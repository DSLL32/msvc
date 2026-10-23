#!/usr/bin/env python3
"""Explicit native GDB ABI/continuation evidence; never a fixture dependency."""
import hashlib,json,os,re,shlex,shutil,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,LIBRARY,native_work
BINARY=Path(__file__).resolve().parents[1]
BASE=BINARY/'cases/resource-identity'
root=Path(tempfile.mkdtemp(prefix='resource-jump-',dir=SCRATCH/'vc1.0/link/frontiers'))
spec=json.loads((BASE/'duplicate-controlled-spec.json').read_text())
work,drives=native_work(root/'native','S:\\vc1.0\\link\\'+spec['case'])
for item in spec['inputs']:
    source=BINARY/item['path'];assert hashlib.sha256(source.read_bytes()).hexdigest()==item['sha256'];shutil.copyfile(source,work/source.name)
env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB']:env.pop(key,None)
env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,TZ='PST8PDT',TMP=str(work),TEMP=str(work))
for key,value in spec.get('environment',{}).items():env[key]=value.replace('{lib}',LIBRARY)
env['RECON_TEMPNAM_PROFILE']='libc100'
state=root/'state.json'
code='''set pagination off
set confirm off
python
import gdb,json,struct,re
state={}
path=%r
def save():open(path,'w').write(json.dumps(state,indent=2)+'\\n')
def error_value():return int(gdb.parse_and_eval('(int)errno'))
def errors():return dict(errno=error_value(),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
class TempnamReturn(gdb.FinishBreakpoint):
    def stop(self):
        pointer=int(self.return_value)
        state['tempnam_after']=dict(result=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(),counter=int(gdb.parse_and_eval('recon_tempnam_counter')),prefix_length=int(gdb.parse_and_eval('recon_tempnam_prefix_length')),**errors())
        save();return False
class TempnamEnter(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)))
        string=lambda p:gdb.Value(p).cast(gdb.lookup_type('char').pointer()).string()
        state['tempnam_before']=dict(directory=string(args[1]),prefix=string(args[2]),caller=gdb.newest_frame().older().name(),counter=int(gdb.parse_and_eval('recon_tempnam_counter')),prefix_length=int(gdb.parse_and_eval('recon_tempnam_prefix_length')),**errors())
        save();TempnamReturn(gdb.newest_frame(),internal=True);return False
TempnamEnter('*recon_tempnam')
class Enter(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        args=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)))
        legacy=int(gdb.parse_and_eval('&resource_abort'))
        context=gdb.parse_and_eval('jump_contexts');found=[]
        while int(context):
            if int(context['legacy'])==legacy:found.append(int(context['opaque_setjmp3']))
            context=context['next']
        state['entry']=dict(buffer_is_resource=args[1]==legacy,buffer_bytes=int(gdb.parse_and_eval('sizeof(resource_abort)')),value=args[2],registered_contexts=found,caller=gdb.newest_frame().older().name(),errno=error_value())
        save();return False
class Resume(gdb.Breakpoint):
    def stop(self):
        value=int(gdb.parse_and_eval('$eax'))&0xffffffff
        state['resume']=dict(value=value,signed_value=struct.unpack('<i',struct.pack('<I',value))[0],function=gdb.newest_frame().name(),errno=error_value())
        save();return False
Enter('*recon_longjmp')
lines=gdb.execute('disassemble convert_resource_input',to_string=True).splitlines()
indexes=[i for i,line in enumerate(lines) if 'call' in line and '_setjmp@plt' in line]
assert len(indexes)==1,lines
match=re.search(r'<\\+(\\d+)>',lines[indexes[0]+1]);assert match
point=Resume('*convert_resource_input+'+match.group(1));point.condition='$eax != 0'
class Fatal(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(esp,16)))
        text=lambda p:gdb.Value(p).cast(gdb.lookup_type('char').pointer()).string()
        state['fatal']=dict(file=text(args[1]),message=args[2],first_variadic_string=text(args[3]),caller=gdb.newest_frame().older().name(),**errors());save();return False
class UnlinkReturn(gdb.FinishBreakpoint):
    def stop(self):
        state['unlink_after']=dict(result=int(self.return_value)&0xffffffff,**errors());save();return False
class Unlink(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<2I',bytes(gdb.selected_inferior().read_memory(esp,8)))
        state['unlink_before']=dict(file=gdb.Value(args[1]).cast(gdb.lookup_type('char').pointer()).string(),caller=gdb.newest_frame().older().name(),**errors());save();UnlinkReturn(gdb.newest_frame(),internal=True);return False
Fatal('*fatal_message');Unlink('*recon_unlink')
class LateErrorReader(gdb.Breakpoint):
    def stop(self):
        if 'unlink_after' in state:
            state.setdefault('late_error_readers',[]).append(gdb.newest_frame().older().name());save()
        return False
LateErrorReader('*recon_GetLastError')
def exited(event):state['exit']=event.exit_code;save()
gdb.events.exited.connect(exited)
end
run %s >%s 2>%s
printf "RESOURCE_RECOVERED_THROUGH_FATAL\\n"
quit
'''%(str(state),shlex.join(spec['arguments']),str(root/'stdout.bin'),str(root/'stderr.bin'))
(root/'jump.gdb').write_text(code)
t=time.perf_counter();result=subprocess.run(['gdb','-q','-batch','-x',str(root/'jump.gdb'),'--args',str(BINARY/'build/native/LINK.EXE')],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);elapsed=time.perf_counter()-t
(root/'gdb.log').write_bytes(result.stdout)
assert result.returncode==0 and state.exists(),result.stdout.decode()
actual=json.loads(state.read_text());expected=[json.loads(s) for s in (BASE/'longjmp/controlled-fullpath-original/events.jsonl').read_text().splitlines() if json.loads(s)['event']=='longjmp_boundary'];assert len(expected)==1;expected=expected[0]
assert expected['buffer']==0x5c590 and expected['caller']==0x4320b and expected['saved_eip']==0x21085
assert expected['registration_is_current']==1 and expected['saved_scope']==expected['registration_scope']==0
assert expected['value']==actual['entry']['value']==actual['resume']['value']==0xffffffff
assert actual['entry']['buffer_is_resource'] and actual['entry']['buffer_bytes']==32 and actual['entry']['registered_contexts']==[0]
assert actual['entry']['caller']=='insert_resource' and actual['resume']['function']=='convert_resource_input' and actual['resume']['signed_value']==-1
assert actual['entry']['errno']==actual['resume']['errno']==2
assert actual['tempnam_before']==dict(directory='\\',prefix='tmp',caller='convert_resource_input',counter=1,prefix_length=0,errno=0,doserrno=0,last_error=0)
assert actual['tempnam_after']==dict(result='S:\\vc1.0\\link\\'+spec['case']+'\\tmp2',counter=2,prefix_length=3,errno=2,doserrno=2,last_error=2)
assert actual['fatal']['file']=='identity.res' and actual['fatal']['message']==26 and actual['fatal']['first_variadic_string']=='' and actual['fatal']['caller']=='convert_resource_input'
returns=[json.loads(s) for s in (BASE/'longjmp/controlled-fullpath-original/events.jsonl').read_text().splitlines() if json.loads(s)['event']=='resource_unlink_return']
assert len(returns)==1
assert {k:v for k,v in actual['unlink_after'].items() if k!='last_error'}=={k:v for k,v in returns[0].items() if k not in ('event','last_error')}
assert not actual.get('late_error_readers',[])
late_guard=json.loads((BASE/'longjmp/no-late-error-original/receipt.json').read_text())
assert late_guard['summary']['failed']==0 and late_guard['summary']['root_exit']==26
guard_events=[json.loads(s) for s in (BASE/'longjmp/no-late-error-original/events.jsonl').read_text().splitlines()]
assert len([e for e in guard_events if e['event']=='no-late-lasterror-guard'])==1
assert actual['unlink_before']['file']=='identity.exe' and actual['unlink_before']['caller']=='fatal_message'
assert actual['exit']==26
assert (root/'stdout.bin').read_bytes()==(BASE/'controlled-reference/duplicate/1/stdout.bin').read_bytes()
assert (root/'stderr.bin').read_bytes()==b''
assert sorted(p.name for p in work.iterdir() if p.is_file())==['identity.res','test.obj','tmp2']
assert (work/'tmp2').read_bytes()==b''

ambient=dict(original_last_error=returns[0]['last_error'],native_last_error=actual['unlink_after']['last_error'],live_result_errno_doserrno_exact=True,original_late_GetLastError_trap_unreached=True,native_late_GetLastError_readers=[],limit='Root-approved def-use limit: successful unlink then actual exit/callback path never reads this stale LastError. Failed full-ambient assertion retained separately; no equivalence claimed for ambient LastError.')
receipt=dict(status='pass',ambient_last_error=ambient,original=expected,native=actual,seconds=elapsed,directory=str(root),scope='Public no-SEH-unwind resource longjmp, actual value-1,32-byte buffer/namedcaller/registeredcontext/continuation/errors; no internal native SEH ABI claim')
(root/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(root/'receipt.json')
