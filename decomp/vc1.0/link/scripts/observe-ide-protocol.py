#!/usr/bin/env python3
"""Native GDB entry/callback observer for the explicit IDE minimal test driver."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import ROOT,SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/ide-callback-protocol'
FUNCTIONS=['initialize_ide_api','emit_ide_message','read_ide_flags']
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--function',choices=FUNCTIONS,required=True);ap.add_argument('--complete',action='store_true');options=ap.parse_args()
    directory=Path(tempfile.mkdtemp(prefix=options.function+'-',dir=SCRATCH/'vc1.0/link'))
    work,drives=native_work(directory,r'S:\vc1.0\link\ide-callback-protocol')
    cutoff=FUNCTIONS.index(options.function)+(1 if options.complete else 0)
    allowed=FUNCTIONS[:cutoff]
    source=r'''
import gdb,json,struct
from pathlib import Path
out=Path(OUTPUT);events=[];active=[];terminal=[]
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def snap(event,name,count,phase):
    esp=val('$esp');probe=val('ide_test_probe');stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2]
    assert 0<=used<=4096
    globals=None
    if gdb.lookup_global_symbol('ide_api'):globals=[val('ide_api'),val('ide_channel')]
    environment_pointer=words(val('&ide_test_out[32]'),1)[0]
    environment=None
    if event=='target_return' and phase==10:
        environment=dict(pointer=environment_pointer,owned=environment_pointer==val('ide_test_environment'),value=mem(environment_pointer,19).hex())
    return dict(environment=environment,event=event,name=name,phase=phase,eip=val('$eip'),esp=esp,eax=val('$eax'),
        ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),arguments=words(esp,count+1),
        stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',
        errors=[val('*ide_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],globals=globals,
        mb_cur_max=val('recon_mb_cur_max'),ctype=mem(val('recon_pctype')-2,514).hex(),
        probe_address=probe,probe=mem(probe,1968).hex(),message_inputs=mem(val('&ide_test_inputs[0]'),512).hex(),
        out_slot_address=val('&ide_test_out[32]'),out_slot=mem(val('&ide_test_out[0]'),68).hex(),
        message_values=words(val('&ide_test_values[0]'),7),message_base=val('&ide_test_inputs[0]'),
        expected_environment=val('ide_test_environment'),callback_addresses=[val('&IdeProbeChannel'),val('&IdeProbeWrite'),val('&IdeProbeAssertion')])
def caller_name(address):
    block=gdb.block_for_pc(address-1)
    while block and not block.function:block=block.superblock
    return str(block.function.name) if block else None
returns={}
class Return(gdb.Breakpoint):
    def __init__(self,address):
        self.entry_records=[]
        super().__init__('*'+str(address),internal=True)
    def stop(self):
        assert self.entry_records,'return site without fresh actual entry'
        before,name,callback,phase=self.entry_records.pop()
        event=('channel_return' if name=='IdeProbeChannel' else 'write_return') if callback else 'target_return'
        after=snap(event,name,0,phase)
        after['paired_entry_phase']=before['phase'];after['active_target_phase']=active[-1] if active else None
        events.append(after);persist()
        assert after['eip']==before['arguments'][0] and after['esp']==before['esp']+4,'cdecl return stack'
        for reg in ['ebx','esi','edi','ebp']:assert before[reg]==after[reg],('callee saved',reg,name)
        if callback:
            assert after['eax']==(7 if name=='IdeProbeChannel' else before['arguments'][3]),'callback defined result'
            assert before['errors']==after['errors'],'callback error effect'
        else:assert active.pop()==phase
        return False
def arm_return(before,name,callback,phase):
    address=before['arguments'][0]
    if address not in returns:returns[address]=Return(address)
    returns[address].entry_records.append((before,name,callback,phase))
class Entry(gdb.Breakpoint):
    def __init__(self,name,count,callback=False):
        self.name=name;self.count=count;self.callback=callback
        super().__init__('*'+name,internal=True)
    def stop(self):
        if self.callback:
            assert active,'callback without actual target caller'
            phase=active[-1]
        elif self.name=='initialize_ide_api':phase=2
        elif self.name=='emit_ide_message':phase=3+words(val('$esp'),3)[1]
        else:phase=10
        event=('channel_entry' if self.name=='IdeProbeChannel' else 'write_entry') if self.callback else 'target_entry'
        before=snap(event,self.name,self.count,phase);events.append(before);persist()
        if self.callback:
            expected='initialize_ide_api' if self.name=='IdeProbeChannel' else 'emit_ide_message'
            before['actual_target_caller']=caller_name(before['arguments'][0]);assert before['actual_target_caller']==expected
            if self.name=='IdeProbeChannel':assert before['arguments'][1:]==[2]
            else:assert before['arguments'][1]==7 and before['arguments'][2]==before['probe_address']+56 and before['arguments'][3]<1024
        elif self.name not in ALLOWED:
            events.append(dict(event='frontier',name=self.name,phase=phase));return True
        else:active.append(phase)
        arm_return(before,self.name,self.callback,phase);return False
class Assertion(gdb.Breakpoint):
    def stop(self):raise AssertionError('unexpected external assertion callback')
def exited(event):terminal.append(dict(event='exit',code=getattr(event,'exit_code',None)));persist()
gdb.events.exited.connect(exited)
gdb.execute('set pagination off');gdb.execute('set confirm off')
for name,count in [('initialize_ide_api',1),('emit_ide_message',2),('read_ide_flags',1)]:Entry(name,count)
Entry('IdeProbeChannel',1,True);Entry('IdeProbeWrite',3,True);Assertion('*IdeProbeAssertion',internal=True)
gdb.execute('run '+PROTOCOL+' >'+STDOUT+' 2>'+STDERR)
assert events and (terminal or events[-1]['event']=='frontier'),'missing terminal or frontier'
if terminal:assert terminal==[dict(event='exit',code=0)] and not active
out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
'''
    source=source.replace('OUTPUT',repr(str(directory/'state.json'))).replace('ALLOWED',repr(allowed)).replace('PROTOCOL',repr(str(directory/'protocol.bin'))).replace('STDOUT',repr(str(directory/'stdout.bin'))).replace('STDERR',repr(str(directory/'stderr.bin')))
    (directory/'observe.py').write_text(source)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT',_MSC_IDE_FLAGS='alpha.obj,beta.lib')
    command=['gdb','-q','-nx','-batch','-ex','file '+str(BINARY/'build/native/ide-protocol'),'-ex','source '+str(directory/'observe.py')]
    start=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=20);elapsed=time.perf_counter()-start
    (directory/'gdb.stdout').write_bytes(r.stdout);(directory/'gdb.stderr').write_bytes(r.stderr)
    assert r.returncode==0 and (directory/'state.json').exists(),(r.returncode,r.stdout.decode(),r.stderr.decode())
    state=json.loads((directory/'state.json').read_text())
    if cutoff<3:assert state['events'][-1]['name']==FUNCTIONS[cutoff]
    else:
        assert (directory/'protocol.bin').read_bytes()==(CASE/'baseline/1/protocol.bin').read_bytes()
        assert (directory/'stdout.bin').read_bytes()==(directory/'stderr.bin').read_bytes()==b''
    receipt=dict(status='observed; typed state validation separate',selected=options.function,complete=options.complete,allowed=allowed,seconds=elapsed,command=command,elf_sha256=hashlib.sha256((BINARY/'build/native/ide-protocol').read_bytes()).hexdigest(),source_sha256=hashlib.sha256((BINARY/'src/ide.c').read_bytes()).hexdigest())
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(directory)
if __name__=='__main__':main()
