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

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', required=True, type=Path)
    parser.add_argument('--phase', default='c1')
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--reference', type=Path)
    options = parser.parse_args()
    start = time.perf_counter()
    case = options.case.resolve()
    directory = options.directory.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    phase = json.loads((case/'manifest.json').read_text())['phases'][options.phase]
    config = json.loads((BINARY/'functions.json').read_text())
    canonical = phase.get('canonical_working_directory')
    if canonical:
        assert canonical.startswith('N:\\') and len(canonical.split('\\')) == 2
    work = directory/(canonical.split('\\')[-1] if canonical else 'work')
    work.mkdir()
    for item in phase['inputs']:
        source = case/item['path']
        data = source.read_bytes()
        assert hashlib.sha256(data).hexdigest() == item['sha256']
        (work/source.name).write_bytes(data)
    windows = lambda p:'Z:'+str(p).replace('/', '\\')
    env = dict(LC_ALL='C', TZ='PST8PDT',
               RECON_PROGRAM_PATH=windows(ROOT/'toolchains/1.0-nt/bin/C13232.EXE'),
               MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(
                   bin=windows(ROOT/'toolchains/1.0-nt/bin'), include=windows(ROOT/'toolchains/1.0-nt/include')))
    if canonical:
        env.update(RECON_DRIVE_N=str(directory),RECON_CANONICAL_CWD=canonical)
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
            else:
                # Recognized original-library frontiers use an explicit native
                # boundary breakpoint, rather than inventing an application stub.
                runtime = json.loads((BINARY/'runtime.json').read_text())
                expected_record = next(r for r in runtime if r['start']==address)
                native_boundaries = {'_tolower':'recon_tolower','__getcwd':'recon_getcwd','_remove':'recon_remove'}
                expected_native = native_boundaries[expected_record['name']]
                runtime_break = 'break ' + expected_native + '\n'

    script = '''set pagination off
set confirm off
set disable-randomization off
%srun >%s 2>%s
python
import gdb,json,struct
state={'buffers':{},'frames':[]}
if gdb.selected_inferior().pid:
    frame=gdb.selected_frame()
    state['frontier']=frame.name()
    state['pc']=int(gdb.parse_and_eval('$pc'))
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
''' % (runtime_break,shlex.quote(str(directory/'stdout')),shlex.quote(str(directory/'stderr')),names,str(directory/'state.json'))
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
            for index,name in enumerate(names):
                line = next(line for line in events.splitlines() if line.startswith('buffer '+str(index)+' '))
                tail = line.split(' ',3)
                expected = None if tail[2]=='unopened' else tail[3]
                assert state['buffers'][name] == expected, ('pending buffer differs',name)
        else:
            assert events.strip() == 'exit 0' and state['frontier'] is None,(events,state)
        reference_work = reference/(canonical.split('\\')[-1] if canonical else 'work')
        originals = {p.name:p.read_bytes() for p in reference_work.iterdir()}
        natives = {p.name:p.read_bytes() for p in work.iterdir()}
        assert originals == natives, 'visible input/output files differ at frontier'
        assert (reference/'stdout').read_bytes() == (directory/'stdout').read_bytes(), 'stdout differs at frontier'
        assert (reference/'stderr').read_bytes() == (directory/'stderr').read_bytes(), 'stderr differs at frontier'
        state['fresh_original_match'] = True
    state.update(preparation_seconds=prepared,execution_seconds=execution,total_seconds=time.perf_counter()-start,
                 environment=env,executable_sha256=hashlib.sha256((BINARY/'build/native/C13232.EXE').read_bytes()).hexdigest())
    (directory/'receipt.json').write_text(json.dumps(state,indent=2)+'\n')
    print(state.get('frontier'),state.get('fresh_original_match'),directory/'receipt.json')


if __name__ == '__main__':
    main()
