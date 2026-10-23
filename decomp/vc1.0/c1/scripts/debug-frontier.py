#!/usr/bin/env python3
"""Native GDB frontier/state observation against separately captured originals."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import time
from case_paths import windows, native_work

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', required=True, type=Path)
    parser.add_argument('--phase', default='c1')
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--reference', type=Path)
    parser.add_argument('--trace-flags',help='Logical function to observe at entry/return')
    parser.add_argument('--trace-timestamps',action='store_true',help='Compare metadata adapter handle/result/output timestamp')
    parser.add_argument('--trace-function-finishes',action='store_true')
    parser.add_argument('--trace-enum-callbacks', action='store_true')
    parser.add_argument('--enum-callback-function', choices=['legacy','pdb'], default='legacy')
    options = parser.parse_args()
    enum_callback = '_compare_pdb_enumerators' if options.enum_callback_function == 'pdb' else '_compare_legacy_enumerators'
    assert options.enum_callback_function == 'legacy' or options.trace_enum_callbacks
    if options.trace_function_finishes:
        assert options.trace_flags is None
        options.trace_flags = '_finish_function_definition'
    if options.trace_timestamps:
        assert options.trace_flags is None and not options.trace_function_finishes
        options.trace_flags = '_get_file_timestamp@8'
    start = time.perf_counter()
    case = options.case.resolve()
    directory = options.directory.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    phase = json.loads((case/'manifest.json').read_text())['phases'][options.phase]
    config = json.loads((BINARY/'functions.json').read_text())
    canonical = phase.get('canonical_working_directory')
    work, path_environment = native_work(directory,canonical)
    for item in phase['inputs']:
        source = case/item['path']
        data = source.read_bytes()
        assert hashlib.sha256(data).hexdigest() == item['sha256']
        target_input = work/source.name
        target_input.write_bytes(data)
        if 'mtime_ns' in item:
            os.utime(target_input,ns=(item['mtime_ns'],item['mtime_ns']))
    env = dict(LC_ALL='C', TZ='PST8PDT',
               RECON_PROGRAM_PATH=windows(ROOT/'toolchains/1.0-nt/bin/C13232.EXE'),
               MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(
                   bin=windows(ROOT/'toolchains/1.0-nt/bin'), include=windows(ROOT/'toolchains/1.0-nt/include')))
    env.update(path_environment)
    # Match the observed external clock input, retaining raw pending-buffer and
    # file equality. This is limited to cases whose database-signature writer
    # and companion field have independent original instruction provenance.
    clock_observation = None
    if options.reference and phase.get('reference_clock'):
        clock = phase['reference_clock']
        assert clock == dict(file='msvc.pdb', offset=48, bytes=4,
                             companion_file='irdb', companion_offset=14)
        reference = options.reference.resolve()
        ref_receipt = json.loads((reference/'receipt.json').read_text())
        ref_work = reference/ref_receipt.get('working_directory_snapshot','work')
        database = ref_work/clock['file']
        if database.exists():
            import struct
            raw = database.read_bytes()
            assert len(raw) >= 52
            signature = struct.unpack_from('<I',raw,48)[0]
            companion = ref_work/clock['companion_file']
            if companion.exists() and companion.stat().st_size >= 18:
                assert struct.unpack_from('<I',companion.read_bytes(),14)[0] == signature
            env['RECON_FIXED_TIME'] = str(signature)
            clock_observation = dict(source=str(database), offset=48,
                                     value=signature, meaning='Observed original database creation time; raw artifact comparison retained.')
    names = [r['name'][1:] for r in config['buffered_streams']]
    expected_record = None
    expected_native = None
    runtime_break = ''
    if options.reference:
        events = (options.reference.resolve()/'events').read_text()
        match = re.search(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+)',events)
        if match:
            address = int(match[1],16)+int(match[2],16)
            expected_record = next((r for r in config['driver'] if r['start']==address),None)
            if expected_record:
                expected_native = {'__unlink':'recon_unlink'}.get(expected_record['name'],
                    re.sub(r'@\d+$','',expected_record['name'][1:]))
                runtime_break = 'break ' + expected_native + '\n'
            else:
                # Recognized original-library frontiers use an explicit native
                # boundary breakpoint, rather than inventing an application stub.
                runtime = json.loads((BINARY/'runtime.json').read_text())
                expected_record = next((r for r in runtime if r['start']==address),None)
                if expected_record is None:
                    from binary import PE
                    from images import imports
                    expected_record = next(r for r in imports(PE(ROOT/phase['target'])) if r['start']==address)
                native_boundaries = {'_rename':'recon_rename','_tolower':'recon_tolower','__strerror':'recon_strerror','__stat':'recon_stat','__fstat':'recon_fstat','__getcwd':'recon_getcwd','_remove':'recon_remove','__dup':'recon_dup','_freopen':'recon_freopen','_qsort':'recon_qsort','_swap':'sort_swap','import:SetEndOfFile':'recon_SetEndOfFile'}
                expected_native = native_boundaries[expected_record['name']]
                runtime_break = 'break ' + expected_native + '\n'

    abi_inputs = expected_record.get('abi_inputs',[]) if expected_record else []
    alias_stderr = phase.get('stdio', {}).get('stderr_to_stdout', False)
    redirection = '>'+shlex.quote(str(directory/'stdout'))
    if alias_stderr:
        redirection += ' 2>&1'
        (directory/'stderr').write_bytes(b'')
    else:
        redirection += ' 2>'+shlex.quote(str(directory/'stderr'))
    callback_commands = ''
    if options.trace_enum_callbacks:
        callback_commands = '''break %s
commands
silent
python
import gdb,json
pointer=gdb.lookup_type('EnumeratorSymbol').pointer().pointer()
a=gdb.parse_and_eval('left').cast(pointer).dereference().dereference()
b=gdb.parse_and_eval('right').cast(pointer).dereference().dereference()
with open(%r,'a') as log: log.write(json.dumps([int(a['index']),int(a['value']),int(b['index']),int(b['value'])])+'\\n')
end
continue
end
''' % (enum_callback[1:],str(directory/'enum-callbacks.jsonl'))
    flag_commands = ''
    if options.trace_flags:
        probe_name = re.sub(r'@\d+$','',options.trace_flags[1:])
        assert options.trace_flags in config['accepted']
        flag_commands = """python
import gdb,json
flag_count = 0
def record_flag_words(count,boundary):
    row=[count,boundary,int(gdb.parse_and_eval('scanner_flags')),int(gdb.parse_and_eval('parser_flags'))]
    with open(%r,'a') as log:log.write(json.dumps(row)+'\\n')
    if %r:
        symbol=gdb.parse_and_eval('function_symbol')
        if not int(symbol):row=[count,boundary,None]
        else:
            value=symbol.dereference()
            row=[count,boundary,int(value['kind']),int(value['flags']),int(value['index']),int(value['attributes']),value['name'].string()]
        with open(%r,'a') as log:log.write(json.dumps(row)+'\\n')
class FlagFinish(gdb.FinishBreakpoint):
    def __init__(self,count):
        super().__init__(gdb.newest_frame(),internal=True)
        self.count=count
        if %r:
            self.handle=int(gdb.parse_and_eval('handle'))
            self.timestamp=gdb.parse_and_eval('timestamp')
    def stop(self):
        if %r:
            row=[self.count,self.handle,int(self.return_value),int(self.timestamp.dereference())]
            with open(%r,'a') as log: log.write(json.dumps(row)+'\\n')
        record_flag_words(self.count,'finish')
        return False
class FlagEntry(gdb.Breakpoint):
    def stop(self):
        global flag_count
        record_flag_words(flag_count,'entry')
        FlagFinish(flag_count)
        flag_count+=1
        return False
FlagEntry(%r,internal=True)
end
""" % (str(directory/'flag-words.jsonl'),options.trace_function_finishes,str(directory/'function-symbols.jsonl'),options.trace_timestamps,options.trace_timestamps,str(directory/'source-timestamps.jsonl'),probe_name)
    script = '''set pagination off
set confirm off
set disable-randomization off
%spython
import gdb
native_exit = None
def record_native_exit(event):
    global native_exit
    native_exit = getattr(event,'exit_code',None)
gdb.events.exited.connect(record_native_exit)
end
run %s
python
import gdb,json,struct
state={'buffers':{},'frames':[],'exit':native_exit}
if gdb.selected_inferior().pid:
    frame=gdb.selected_frame()
    state['frontier']=frame.name()
    state['pc']=int(gdb.parse_and_eval('$pc'))
    if state['frontier']=='recon_qsort':
        state['qsort_contract']={
            'count':int(gdb.parse_and_eval('count')),
            'width':int(gdb.parse_and_eval('width')),
            'array_nonnull':bool(int(gdb.parse_and_eval('array'))),
            'callback_is_legacy_enumerator':int(gdb.parse_and_eval('compare'))==int(gdb.parse_and_eval('&compare_legacy_enumerators'))}

    state['abi_inputs']=[]
    for item in %r:
        slot=item['stack_slot']
        value=struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$ebp'))+4*(slot+1),4)))[0]
        row=dict(name=item['name'],stack_slot=slot,value=value)
        if item.get('formal'):
            row['formal_value']=int(gdb.parse_and_eval(item['formal'])) & 0xffffffff
            assert row['formal_value']==value,('native formal/ABI slot differs',row)
        state['abi_inputs'].append(row)
    while frame and len(state['frames'])<12:
        state['frames'].append(frame.name())
        frame=frame.older()
    for name in %r:
        address=int(gdb.parse_and_eval(name))
        if not address:
            state['buffers'][name]=None
            continue
        ptr,count,base=struct.unpack('<III',bytes(gdb.selected_inferior().read_memory(address,12)))
        assert 0<=ptr-base<=65536,(name,ptr,base)
        state['buffers'][name]=(bytes(gdb.selected_inferior().read_memory(base,ptr-base)).hex() if ptr>base else '')
    state['label_stack_count']=int(gdb.parse_and_eval('label_stack_count'))
    state['scanner_flags']=int(gdb.parse_and_eval('scanner_flags'))
    state['exception_scope_depth']=[int(gdb.parse_and_eval('exception_scope_depth[%%d]'%%i)) for i in range(3)]
else:
    state['frontier']=None
with open(%r,'w') as out:json.dump(state,out,indent=2)
end
''' % (runtime_break+callback_commands+flag_commands,redirection,abi_inputs,names,str(directory/'state.json'))
    (directory/'commands.gdb').write_text(script)
    prepared = time.perf_counter()-start
    run_start = time.perf_counter()
    result = subprocess.run(['/usr/bin/gdb','--nx','--batch','-x',str(directory/'commands.gdb'),
                             str(BINARY/'build/native/C13232.EXE')], cwd=work, env=env,
                            stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    execution = time.perf_counter()-run_start
    (directory/'gdb.log').write_bytes(result.stdout)
    assert result.returncode == 0, result.stdout.decode('latin1')
    state = json.loads((directory/'state.json').read_text())
    if options.reference:
        reference = options.reference.resolve()
        events = (reference/'events').read_text()
        match = re.search(r'exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+)',events)
        if match:
            address = int(match[1],16)+int(match[2],16)
            assert state['frontier'] == expected_native, (expected_record['name'],state['frontier'])
            state['original_logical'] = expected_record['name']
            original_stack={int(slot):int(value,16)for slot,value in re.findall(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
            for item in state['abi_inputs']:
                assert item['value']==original_stack[item['stack_slot']],('original/native ABI input differs',item,original_stack[item['stack_slot']])
            if abi_inputs: state['abi_inputs_original_match']=True
            if expected_record['name']=='_qsort':
                stack = {int(slot):int(value,16) for slot,value in re.findall(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
                assert state['qsort_contract']==dict(count=stack[2],width=stack[3],array_nonnull=bool(stack[1]),callback_is_legacy_enumerator=True)
                assert stack[4]==next(r['start'] for r in config['driver'] if r['name']=='_compare_legacy_enumerators')

            for index,name in enumerate(names):
                line = next(line for line in events.splitlines() if line.startswith('buffer '+str(index)+' '))
                tail = line.split(' ',3)
                expected = None if tail[2]=='unopened' else tail[3]
                assert state['buffers'][name] == expected, ('pending buffer differs',name)
        else:
            terminal_lines = [line for line in events.splitlines() if not ((options.trace_enum_callbacks and line.startswith('enum-callback ')) or (options.trace_flags and line.startswith('flag-words ')) or (options.trace_function_finishes and line.startswith('function-symbol ')) or (options.trace_timestamps and line.startswith('source-timestamp ')))]
            assert terminal_lines == ['exit '+str(phase['expected_exit'])] and state['frontier'] is None,(events,state)
            assert state['exit'] == phase['expected_exit'], ('native terminal status differs',state['exit'],phase['expected_exit'])
        if options.trace_enum_callbacks:
            actual = [json.loads(line) for line in (directory/'enum-callbacks.jsonl').read_text().splitlines()]
            observed = [[int(value,16) for value in row[1:]] for row in re.findall(r'^enum-callback (\d+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',events,re.M)]
            assert actual == observed and actual, ('enum callback operand order differs',observed,actual)
            state['enum_callback_operands'] = actual
        if options.trace_flags:
            actual = [json.loads(line) for line in (directory/'flag-words.jsonl').read_text().splitlines()]
            observed = [[int(n),boundary,int(scanner,16),int(parser,16)] for n,boundary,scanner,parser in re.findall(r'^flag-words (\d+) (entry|finish) ([0-9a-f]+) ([0-9a-f]+)$',events,re.M)]
            assert actual == observed and actual, ('complete flag words differ at boundary',observed,actual)
            assert [r[1] for r in actual] == ['entry','finish']*(len(actual)//2)
            state['flag_word_boundaries'] = dict(function=options.trace_flags,observed=actual)
        if options.trace_timestamps:
            actual = [json.loads(line) for line in (directory/'source-timestamps.jsonl').read_text().splitlines()]
            observed = [[int(v)for v in row]for row in re.findall(r'^source-timestamp (\d+) (\d+) (\d+) (\d+)$',events,re.M)]
            assert actual == observed and actual, ('source metadata differs',observed,actual)
            assert all(row[2:]==[1,1768480496]for row in actual),actual
            state['source_timestamps'] = actual
        if options.trace_function_finishes:
            actual = [json.loads(line) for line in (directory/'function-symbols.jsonl').read_text().splitlines()]
            observed=[]
            for line in events.splitlines():
                if not line.startswith('function-symbol '):continue
                fields=line.split()
                if fields[3]=='absent':observed.append([int(fields[1]),fields[2],None])
                else:observed.append([int(fields[1]),fields[2],*[int(v,16) for v in fields[3:7]],fields[7]])
            assert actual == observed and actual, ('function finish symbol differs',observed,actual)
            assert [r[-1] for r in actual if r[1]=='entry']==phase.get('expected_function_finishes',['broken_initializer','implicit_call','recovered_tail'])
            assert all(r[2] is None for r in actual if r[1]=='finish')
            state['function_finish_symbols']=actual
        original_receipt = json.loads((reference/'receipt.json').read_text())
        reference_work = reference/original_receipt.get('working_directory_snapshot','work')
        originals = {p.name:p.read_bytes() for p in reference_work.iterdir()}
        natives = {p.name:p.read_bytes() for p in work.iterdir()}
        assert originals == natives, 'visible input/output files differ at frontier'
        assert (reference/'stdout').read_bytes() == (directory/'stdout').read_bytes(), 'stdout differs at frontier'
        assert (reference/'stderr').read_bytes() == (directory/'stderr').read_bytes(), 'stderr differs at frontier'
        state['fresh_original_match'] = True
    state['reference_clock_observation'] = clock_observation
    state.update(preparation_seconds=prepared,execution_seconds=execution,total_seconds=time.perf_counter()-start,
                 environment=env,stdio=phase.get('stdio', {'stderr_to_stdout':False}),executable_sha256=hashlib.sha256((BINARY/'build/native/C13232.EXE').read_bytes()).hexdigest())
    (directory/'receipt.json').write_text(json.dumps(state,indent=2)+'\n')
    print(state.get('frontier'),state.get('fresh_original_match'),directory/'receipt.json')


if __name__ == '__main__':
    main()
