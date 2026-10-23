"""Focused native missing-query rejection and successful-owner control.

Synthetic process-local base/group records exercise the two query outcomes.
Unaccepted padding is supplied as zero only in this controlled guard test.
This is not a public input, original equivalence claim or reconstruction case.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from case_paths import BINARY, ROOT, CAPTURE_ROOT, native_work, windows


def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run(success):
    started = time.perf_counter()
    directory = Path(tempfile.mkdtemp(prefix='debug-base-query-native-policy-', dir=CAPTURE_ROOT))
    fixture = BINARY.parent/'fixtures/c1xx-state-machine-debug'
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    executable = BINARY/'build/native-trace/C1XX3232.EXE'
    frozen = {str(p):sha(p) for p in [executable,fixture/'manifest.json',BINARY/'functions.json',BINARY/'cases/native-traces.json']}
    padding_emulated = '_pad_debug_field' not in json.loads((BINARY/'functions.json').read_text())['accepted']
    work, drives = native_work(directory, phase['canonical_working_directory'])
    for item in phase['inputs']:
        path = fixture/item['path']; raw = path.read_bytes()
        assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256']
        frozen[str(path)] = sha(path); (work/Path(item['path']).name).write_bytes(raw)
    commands = ['set pagination off','break emit_debug_base_fields',
        'run > '+str(directory/'stdout.bin')+' 2> '+str(directory/'stderr.bin'),
        'python','import gdb,json',
        'assert gdb.newest_frame().name()=="emit_debug_base_fields"',
        'assert int(gdb.parse_and_eval("*member_index"))==0',
        'assert int(gdb.parse_and_eval("debug_field_bytes"))==0','end',
        'set $owner = tag','set $counter = member_index',
        'set $base = (BaseClassLink *)calloc(1,sizeof(BaseClassLink))',
        'set $base->kind = 15','set $base->flags = 1','set $base->state = 1',
        'set $base->context_index = tag->type->field08',
        'set tag->auxiliary.scope_members = (CppSymbol *)$base',
        'set $groups = (Vector *)calloc(1,sizeof(Vector))',
        'set tag->detail.class_info->virtual_table_groups = $groups']
    if success:
        commands += ['set $group = (VirtualTableGroup *)calloc(1,sizeof(VirtualTableGroup))',
            'set $group->tag = $owner','set $group->virtual_pointer_offset = 4',
            'set $group->virtual_base_table_offset = 8',
            'set $pointer = (CppSymbol *)calloc(1,sizeof(CppSymbol))',
            'set $ptype = (CType *)calloc(1,sizeof(CType))',
            'set $ptype->flags = 4','set $ptype->debug_index = 0x1234',
            'set $pointer->type = $ptype','set $group->virtual_base_pointer = $pointer',
            'set $groups->items = (void **)calloc(1,sizeof(void *))',
            'set $groups->items[0] = $group','set $groups->count = 1']
    commands += ['disable 1','break pad_debug_field','continue',
        'python','assert gdb.newest_frame().name()=="pad_debug_field"','end',
        'return (unsigned)0' if padding_emulated else 'finish','disable 2','finish' if success else 'continue','python',
        'frame=gdb.newest_frame()',
        'assert int(gdb.parse_and_eval("*$counter"))==1']
    if success:
        commands += ['assert frame.name()=="emit_debug_field_list"',
            'assert int(gdb.parse_and_eval("$eax"))==1',
            'assert int(gdb.parse_and_eval("debug_field_bytes"))==12',
            'assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+4)"))==0x401',
            'assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+8)"))==0x1234',
            'assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+12)"))==4',
            'assert int(gdb.parse_and_eval("*(unsigned short *)(debug_record_buffer+14)"))==2']
    else:
        commands += ['assert frame.name()=="emit_debug_base_fields"',
            'assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5',
            'pc=frame.pc(); assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\\xcc"',
            'assert int(gdb.parse_and_eval("$groups->count"))==0']
    result = dict(status='pass',mode='successful actual owner control' if success else 'missing group rejected before output reads',synthetic_native_only=True,padding='forced zero stub return for focused control' if padding_emulated else 'actual accepted padding helper',public_reachability='UNKNOWN')
    commands += ['open('+repr(str(directory/'observation.json'))+',"w").write(json.dumps('+repr(result)+',indent=2)+"\\n")','end']
    (directory/'commands.gdb').write_text('\n'.join(commands)+'\n')
    env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for key in ('CL','_CL_','LINK','_LINK_'): env.pop(key,None)
    env.update(drives,TZ='PST8PDT',
        MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')),
        RECON_PROGRAM_PATH=phase['command_line_template'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin')))
    prepared = time.perf_counter()
    proc = subprocess.run(['gdb','-q','-batch','-x',str(directory/'commands.gdb'),'--args',str(executable)],cwd=work,env=env,capture_output=True,text=True,timeout=60)
    ran = time.perf_counter(); (directory/'gdb.log').write_text(proc.stdout+proc.stderr)
    assert proc.returncode==0 and (directory/'observation.json').exists(), proc.stdout+proc.stderr
    assert all(sha(p)==v for p,v in frozen.items()), 'native guard inputs changed'
    receipt = dict(status='pass',frozen=frozen,observation=result,setup_seconds=prepared-started,debugger_seconds=ran-prepared,total_seconds=time.perf_counter()-started)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':
    run(False)
    run(True)
