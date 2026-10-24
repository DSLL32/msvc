#!/usr/bin/env python3
"""Explicit native GDB log/FILE/public-CRT observations, never a fixture."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-debug-log'
def main():
    p=argparse.ArgumentParser();p.add_argument('--trial',action='append',default=[]);p.add_argument('--support',action='append',default=[]);a=p.parse_args();cfg=json.loads((BINARY/'functions.json').read_text());allowed={n[1:]for n in cfg['accepted']}|set(a.trial);support=set(cfg['accepted'])|set(a.support)
    d=Path(tempfile.mkdtemp(prefix='debug-log-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,'S:\\vc1.0\\link\\direct-debug-log')
    source=r'''
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];frames=[];returns={};unavailable=[];out=Path(OUTPUT)
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))if n else []
def string(p):
 b=b''
 for i in range(128):
  c=mem(p+i,1);b+=c
  if c==b'\0':return b
 raise AssertionError('unterminated string')
def caller(address):
 block=gdb.block_for_pc(address-1)
 while block and not block.function:block=block.superblock
 return str(block.function.name)if block else None
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal,unavailable=unavailable),indent=2)+'\n')
COUNTS=dict(debug_initialize=2,open_debug_log=1,debug_printf=1,set_debug_flag=2,fopen=2,fclose=1,setbuf=2,perror=1)
def snap(name,event,before=None):
 esp=val('$esp');phase=val('log_test_phase');args=before['arguments']if before else words(esp+4,COUNTS[name]);streams=[]
 for i in range(1,4):
  raw=words(val('&recon_iob[%d]'%i),8);used=raw[0]-raw[2]if raw[3]&0x83 and not raw[3]&4 else 0;assert 0<=used<=4096
  streams.append(dict(index=i,raw=raw,pending=mem(raw[2],used).hex()if used else ''))
 e=dict(event=event,phase=phase,name=name,esp=esp,eip=val('$eip'),eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),caller=words(esp,1)[0],actual_caller=before['actual_caller']if before else caller(words(esp,1)[0]),arguments=args,stack_argument_words=words(esp if before else esp+4,COUNTS[name]),errors=[val('*log_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],names_address=val('&log_test_names'),names=mem(val('&log_test_names'),272).hex(),debug_flags_address=val('&debug_flags'),debug_flags=mem(val('&debug_flags'),100).hex(),debug_log=val('debug_log'),iob_address=val('&recon_iob'),streams=streams,fp_control=val('$fctrl'),fp_status=val('$fstat'),fp_tag=val('$ftag'),fp_raw=gdb.execute('info float',to_string=True))
 if name=='debug_printf':assert b'%'not in string(args[0]);e['variadic_words_consumed']=0
 if name in ['debug_initialize','open_debug_log','debug_printf','fopen','perror']:
  e['argument_strings']=[string(p).hex()if p else None for p in args[:2 if name in ['debug_initialize','fopen']else 1]]
 return e
class Return(gdb.Breakpoint):
 def stop(self):
  assert frames and frames[-1]['caller']==val('$eip');before=frames.pop();after=snap(before['name'],'return',before);events.append(after);persist();assert after['esp']==before['esp']+4 and all(after[r]==before[r]for r in ['ebx','esi','edi','ebp']);self.enabled=any(f['caller']==after['eip']for f in frames);return False
class Entry(gdb.Breakpoint):
 def __init__(self,name,symbol):self.name=name;self.symbol=symbol;super().__init__('*'+symbol,internal=True)
 def stop(self):
  e=snap(self.name,'entry');e['native_symbol']=self.symbol
  if self.name in ['fopen','fclose','setbuf','perror']:assert e['actual_caller']=='open_debug_log',('unexpected public caller',self.name,e['actual_caller'])
  ok=('_'+self.name in SUPPORT)if self.name in ['fopen','fclose','setbuf','perror']else self.name in ALLOWED
  if not ok:e['event']='frontier'
  events.append(e);persist()
  if not ok:return True
  frames.append(e);address=e['caller']
  if address not in returns:returns[address]=Return('*'+str(address),internal=True)
  else:returns[address].enabled=True
  return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off')
for name,symbol in [('debug_initialize','debug_initialize'),('open_debug_log','open_debug_log'),('debug_printf','debug_printf'),('set_debug_flag','set_debug_flag'),('fopen','recon_fopen'),('fclose','recon_fclose'),('setbuf','recon_setbuf')]:Entry(name,symbol)
for symbol in ['recon_perror',"'perror@plt'"]:
 try:val('(unsigned)&'+symbol)
 except gdb.error:continue
 Entry('perror',symbol);break
else:unavailable.append('perror (unreferenced until open_debug_log candidate)')
gdb.execute('run '+INPUT+' '+REPORT+' >'+STDOUT+' 2>'+STDERR);persist();assert events
'''
    for k,v in dict(OUTPUT=str(d/'state.json'),ALLOWED=allowed,SUPPORT=support,INPUT=str(CASE/'inputs/names.bin'),REPORT=str(d/'results.bin'),STDOUT=str(d/'stdout.bin'),STDERR=str(d/'stderr.bin')).items():source=source.replace(k,repr(v))
    (d/'observe.py').write_text(source);env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build/native/debug-log';command=['gdb','-q','-nx','-batch','-ex','file '+str(exe),'-ex','source '+str(d/'observe.py')];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=20);elapsed=time.perf_counter()-t
    (d/'gdb.stdout').write_bytes(r.stdout);(d/'gdb.stderr').write_bytes(r.stderr);(d/'receipt.json').write_text(json.dumps(dict(command=command,seconds=elapsed,status=r.returncode,trial=a.trial,support=a.support,elf_sha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(d,flush=True);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
