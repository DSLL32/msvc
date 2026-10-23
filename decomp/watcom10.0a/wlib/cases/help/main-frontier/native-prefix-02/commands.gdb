set pagination off
set confirm off
set exec-wrapper /usr/bin/python3 /root/msvc2/decomp/watcom10.0a/wlib/scripts/native-case-wrapper.py
python
import gdb,json,os,stat
from pathlib import Path
out=Path('/root/.ai-tmp/msvc2/watcom10.0a/wlib/native-main-prefix/run-2cxer5nk')
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
    assert heap_state==dict(bound=1,clean=expected_calls,calls=expected_calls,target_activity=0,reference_sha256='1b1f029451106e1e055c63ec21a78dac740984dcebe66bc212b73fbbaff1b625'),heap_state
    assert argc==2 and strings==[r'R:\toolchains\watcom10.0a\.patched\WLIB.EXE','?']
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
    gdb.execute('continue')
    assert gdb.selected_frame().name()=='CheckMemoryHook'
    assert gdb.execute('info program',to_string=True).find('SIGTRAP')>=0
    frame=gdb.selected_frame().older()
    assert frame.name()=='main'
    record('CheckMemoryHook-before-body',frame)
    (out/'native-state.json').write_text(json.dumps(dict(status='matching logical main-to-hook frontier',records=records,stdio=kinds),indent=2)+'\n')
except BaseException as error:
    (out/'native-state.json').write_text(json.dumps(dict(status='HOLD',error=str(error),records=records),indent=2)+'\n')
    gdb.execute('quit 125')
end
quit 0
