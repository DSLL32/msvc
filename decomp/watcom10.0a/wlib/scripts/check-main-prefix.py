#!/usr/bin/env python3
"""Native-only debugger replay of the first main-to-hook frontier."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

setup_started = time.monotonic()
root = Path(__file__).resolve().parents[1]
hook = len(sys.argv)>1 and sys.argv[1]=='--hook'
case_path='cases/help/hook-frontier' if hook else 'cases/help/main-frontier'
reference = root / case_path / 'original-prefix-01/events.txt'
expected_raw='4fd99a68e557a6729eefa78b34d6ceb6dac0e5a39c4999dbe5135df4a2d195c6' if hook else 'a4f641651dfb1c1d4181a33042ad29eb3f00cdad45d291db02c128f94faae6d7'
assert hashlib.sha256(reference.read_bytes()).hexdigest() == expected_raw
closure_path = root / case_path / 'heap-bootstrap-reference.json'
closure_sha = hashlib.sha256(closure_path.read_bytes()).hexdigest()
closure = json.loads(closure_path.read_text())
for relative, expected in closure['bindings'].items():
    assert hashlib.sha256((root / relative).read_bytes()).hexdigest() == expected, relative
assert closure_sha in (root / 'scripts/native-host.c').read_text()
scratch = Path('/root/.ai-tmp/msvc2/watcom10.0a/wlib/native-main-prefix')
scratch.mkdir(parents=True, exist_ok=True)
out = Path(tempfile.mkdtemp(prefix='run-', dir=scratch))
child_cwd=out/'child'
child_cwd.mkdir()
script = '''set pagination off
set confirm off
set exec-wrapper /usr/bin/python3 WRAPPER
python
import gdb,json,os,stat
from pathlib import Path
out=Path(OUTPUT)
records=[]
def value(text): return int(gdb.parse_and_eval(text))
def record(label,frame):
    frame.select()
    argc=value('argc');argv=gdb.parse_and_eval('argv')
    strings=[argv[i].string() for i in range(argc)]
    env=bytes(gdb.selected_inferior().read_memory(value('&Env'),52)).hex()
    flags=value('ToolFlags')
    fp=dict(cw=value('$fctrl')&65535,sw=value('$fstat')&65535,tag=value('$ftag')&65535)
    errors=dict(errno=value('(int)errno'),doserrno=value('recon_doserrno'))
    heap=gdb.parse_and_eval('recon_watcom_first_main_heap')
    heap_state={name:int(heap[name]) for name in ['bound','clean','calls','target_activity']}
    heap_state['reference_sha256']=heap['reference_sha256'].string()
    records.append(dict(place=label,argc=argc,argv=strings,Env=env,ToolFlags=flags,fp=fp,errors=errors,heap=heap_state))
    expected_calls=0 if label in ['main-entry','heap-entry'] else 1
    assert heap_state==dict(bound=1,clean=expected_calls,calls=expected_calls,target_activity=0,reference_sha256=CLOSURE),heap_state
    assert argc==2 and strings==[r'R:\\toolchains\\watcom10.0a\\.patched\\WLIB.EXE','?']
    assert env=='00'*52 and flags==0
    assert errors==dict(errno=0,doserrno=0),errors
    assert fp==dict(cw=0x127f,sw=0,tag=0xffff),fp
try:
    gdb.execute('break main')
    gdb.execute('run ? >'+str(out/'stdout.bin')+' 2>'+str(out/'stderr.bin'))
    assert gdb.selected_frame().name()=='main'
    record('main-entry',gdb.selected_frame())
    pid=gdb.selected_inferior().pid
    kinds=[]
    for fd in range(3):
        mode=os.stat('/proc/'+str(pid)+'/fd/'+str(fd)).st_mode
        kinds.append('pipe' if stat.S_ISFIFO(mode) else 'file' if stat.S_ISREG(mode) else 'other')
    assert kinds==['pipe','file','file'],kinds
    gdb.execute('break recon_watcom_heapchk')
    gdb.execute('continue')
    assert gdb.selected_frame().name()=='recon_watcom_heapchk'
    record('heap-entry',gdb.selected_frame().older())
    gdb.newest_frame().select()
    gdb.execute('finish')
    assert gdb.selected_frame().name()=='main'
    assert value('$eax')==0
    record('heap-return',gdb.selected_frame())
    if HOOK:
        gdb.execute('break CheckMemoryHook')
        gdb.execute('continue')
        assert gdb.selected_frame().name()=='CheckMemoryHook'
        record('CheckMemoryHook-entry',gdb.selected_frame().older())
        gdb.newest_frame().select()
        gdb.execute('finish')
        assert gdb.selected_frame().name()=='main'
        record('CheckMemoryHook-return',gdb.selected_frame())
    gdb.execute('continue')
    expected='InitMsg' if HOOK else 'CheckMemoryHook'
    assert gdb.selected_frame().name()==expected
    assert gdb.execute('info program',to_string=True).find('SIGTRAP')>=0
    if HOOK:
        image=gdb.selected_frame().read_var('image_name').string()
        assert image==records[0]['argv'][0]
    frame=gdb.selected_frame().older()
    assert frame.name()=='main'
    record(expected+'-before-body',frame)
    (out/'native-state.json').write_text(json.dumps(dict(status='matching logical application frontier',records=records,stdio=kinds),indent=2)+'\\n')
except BaseException as error:
    (out/'native-state.json').write_text(json.dumps(dict(status='HOLD',error=str(error),records=records),indent=2)+'\\n')
    gdb.execute('quit 125')
end
quit 0
'''.replace('WRAPPER',str(root/'scripts/native-case-wrapper.py')).replace('OUTPUT',repr(str(out))).replace('CLOSURE',repr(closure_sha)).replace('HOOK',repr(hook))
(out/'commands.gdb').write_text(script)
command = ['gdb','-q','-batch',str(root/'build/native/WLIB'),'-x',str(out/'commands.gdb')]
setup_elapsed=time.monotonic()-setup_started
start=time.monotonic()
p=subprocess.run(command,input=b'\n'*64,capture_output=True,cwd=child_cwd,timeout=15)
status=p.returncode
artifacts=[str(path.relative_to(child_cwd)) for path in child_cwd.rglob('*')]
if artifacts or any((out/name).stat().st_size for name in ['stdout.bin','stderr.bin']):
    status=125
(out/'gdb.stdout').write_bytes(p.stdout)
(out/'gdb.stderr').write_bytes(p.stderr)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
receipt=dict(argv=command,status=status,artifacts=artifacts,cwd=str(child_cwd),cwd_initially_empty=True,elapsed_seconds=time.monotonic()-start,
             setup_elapsed_seconds=setup_elapsed,execution_including_debugger_checks='elapsed_seconds; native build is a Make dependency outside this observer',
             binary_sha256=sha(root/'build/native/WLIB'),reference_sha256=sha(reference),closure_sha256=closure_sha,
             source_sha256={str(path.relative_to(root)):sha(path) for path in sorted((root/'src').glob('*.c'))+sorted((root/'src').glob('*.h'))+[root/'scripts/native-host.c',root/'Makefile',Path(__file__)]},
             native_abi='GCC i386 semantic values; no raw Watcom register identity claim',
             termination='GDB owns and kills native inferior at logical INT3 frontier; no normal case exit claim')
if (out/'native-state.json').exists():receipt['native_state']=json.loads((out/'native-state.json').read_text())
receipt['files']={path.name:dict(size=path.stat().st_size,sha256=sha(path)) for path in out.iterdir() if path.is_file() and path.name!='receipt.json'}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(receipt=str(out/'receipt.json'),status=status,elapsed_seconds=receipt['elapsed_seconds'],state=receipt.get('native_state',{}).get('status')),indent=2))
if status:print(p.stdout.decode(errors='replace'));print(p.stderr.decode(errors='replace'))
raise SystemExit(status)
