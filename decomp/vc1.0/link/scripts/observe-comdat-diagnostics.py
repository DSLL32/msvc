#!/usr/bin/env python3
"""Native GDB actual-ABI observations for an explicit direct graph caller."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import ROOT,SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/comdat-graph-diagnostics'
FUNCTIONS=['dump_comdat_graph','debug_printf','print_comdat_node','comdat_usage_label','print_comdat_edge']
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--trial',choices=FUNCTIONS);args=ap.parse_args()
    config=json.loads((BINARY/'functions.json').read_text());allowed=[n for n in FUNCTIONS if '_'+n in config['accepted'] or n==args.trial]
    directory=Path(tempfile.mkdtemp(prefix='comdat-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(directory,r'S:\vc1.0\link\comdat-graph-diagnostics')
    source=r'''
import gdb,json,struct
from pathlib import Path
out=Path(OUTPUT);events=[];terminal=[];frames=[];returns={}
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def string(p):
    result=b''
    for i in range(256):
        c=mem(p+i,1)
        if c==b'\0':return result
        result+=c
    raise AssertionError('unterminated string')
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def caller(address):
    block=gdb.block_for_pc(address-1)
    while block and not block.function:block=block.superblock
    return str(block.function.name) if block else None
def snap(event,name):
    esp=val('$esp');count=1 if event=='entry' else 0;fmt=value=b''
    if name=='debug_printf' and event=='entry':
        fmt=string(words(esp,2)[1])
        if b'%' in fmt:
            assert fmt in [b'%s',b'|sec=%.8s',b'|grp=%.8s',b'|con=0x%p',b'|mod=%s',b'|%s()'];count=2
            if fmt!=b'|con=0x%p':value=string(words(esp,3)[2])
    mapping=[line for line in Path('/proc/'+str(gdb.selected_inferior().pid)+'/maps').read_text().splitlines() if line.startswith('12000000-12001000 ')];assert len(mapping)==1 and mapping[0].split()[1]=='rw-p','owned private region extent/protection'
    stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2];assert 0<=used<=4096
    return dict(mapping=mapping[0],actual_caller=caller(words(esp,1)[0]) if event=='entry' else None,event=event,name=name,eip=val('$eip'),esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),arguments=words(esp,count+1),format=fmt.hex(),string_value=value.hex(),input_base=val('comdat_test_region'),region=mem(val('comdat_test_region'),4096).hex(),stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',errors=[val('*comdat_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],debug_flags=mem(val('&debug_flags[0]'),100).hex(),debug_log=val('debug_log') if gdb.lookup_global_symbol('debug_log') else 0,mb_cur_max=val('recon_mb_cur_max'),ctype=mem(val('recon_pctype')-2,514).hex(),return_text=string(val('$eax')).hex() if name=='comdat_usage_label' and event=='return' else '')
class Return(gdb.Breakpoint):
    def stop(self):
        assert frames and frames[-1]['arguments'][0]==val('$eip'),'return without actual current entry'
        before=frames.pop();after=snap('return',before['name']);events.append(after);persist()
        assert after['esp']==before['esp']+4
        assert all(after[r]==before[r] for r in ['ebx','esi','edi','ebp'])
        self.enabled=any(frame['arguments'][0]==after['eip'] for frame in frames)
        return False
class Entry(gdb.Breakpoint):
    def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
    def stop(self):
        entry=snap('entry',self.name);events.append(entry);persist()
        if self.name not in ALLOWED:events.append(dict(event='frontier',name=self.name,eip=val('$eip')));persist();return True
        frames.append(entry);address=entry['arguments'][0]
        if address not in returns:returns[address]=Return('*'+str(address),internal=True)
        else:returns[address].enabled=True
        return False
def exited(event):terminal.append(dict(event='exit',code=getattr(event,'exit_code',None)));persist()
gdb.events.exited.connect(exited)
gdb.execute('set pagination off');gdb.execute('set confirm off')
for name in FUNCTIONS:Entry(name)
gdb.execute('run '+INPUT+' >'+STDOUT+' 2>'+STDERR)
assert events and (terminal or events[-1]['event']=='frontier')
if terminal:assert terminal==[dict(event='exit',code=0)] and not frames
persist()
'''
    for name,value in dict(OUTPUT=str(directory/'state.json'),ALLOWED=allowed,FUNCTIONS=FUNCTIONS,INPUT=str(CASE/'inputs/graph.bin'),STDOUT=str(directory/'stdout.bin'),STDERR=str(directory/'stderr.bin')).items():source=source.replace(name,repr(value))
    (directory/'observe.py').write_text(source)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    cmd=['gdb','-q','-nx','-batch','-ex','file '+str(BINARY/'build/native/comdat-diagnostics'),'-ex','source '+str(directory/'observe.py')]
    start=time.perf_counter();r=subprocess.run(cmd,cwd=work,env=env,capture_output=True,timeout=20);elapsed=time.perf_counter()-start
    (directory/'gdb.stdout').write_bytes(r.stdout);(directory/'gdb.stderr').write_bytes(r.stderr)
    assert r.returncode==0 and (directory/'state.json').exists(),(r.returncode,r.stdout.decode(),r.stderr.decode())
    state=json.loads((directory/'state.json').read_text());receipt=dict(status='observed; typed validation separate',trial=args.trial,allowed=allowed,seconds=elapsed,command=cmd,elf_sha256=hashlib.sha256((BINARY/'build/native/comdat-diagnostics').read_bytes()).hexdigest(),source_sha256=hashlib.sha256((BINARY/'src/comdat-diagnostics.c').read_bytes()).hexdigest())
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(directory)
if __name__=='__main__':main()
