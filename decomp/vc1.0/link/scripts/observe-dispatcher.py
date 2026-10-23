#!/usr/bin/env python3
"""Native actual dispatcher entry/return ABI observations, not a fixture."""
import argparse,hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--phase',choices=['helper','direct'],required=True);ap.add_argument('--trial',action='store_true');args=ap.parse_args()
    logical='helper_main' if args.phase=='helper' else 'driver_usage';config=json.loads((BINARY/'functions.json').read_text());allowed='_'+logical in config['accepted'] or args.trial
    directory=Path(tempfile.mkdtemp(prefix='dispatcher-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(directory,'S:\\vc1.0\\link\\dispatcher-assistance-'+args.phase)
    source=r"""
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];before=None
out=Path(OUTPUT)
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def string(p):
 result=b''
 for i in range(512):
  c=mem(p+i,1);result+=c
  if c==b'\0':return result.hex()
 raise AssertionError('unterminated actual string')
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snap(event):
 esp=val('$esp');argc=0;argv=0;formals=[];values=[]
 if event in ('entry','frontier') and LOGICAL=='helper_main':
  args=words(esp,3);argc=args[1];argv=args[2];assert argc<=32;formals=args[1:]
  values=[string(p) for p in words(argv,argc)];assert words(argv+argc*4,1)==[0]
 tools=[]
 for i,size in enumerate([7,16,7,7,7,1]):
  item=gdb.parse_and_eval('tools[%d]'%i)
  tools.append(dict(name_pointer=int(item['name']),length=int(item['length']),padding=int(item['padding']),kind=int(item['kind']),run=int(item['run']),bytes=mem(int(item['name']),size).hex()))
 stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2];assert 0<=used<=4096
 maps=[line.split() for line in Path('/proc/'+str(gdb.selected_inferior().pid)+'/maps').read_text().splitlines()]
 writable=[]
 for item,size in zip(tools[:5],[7,16,7,7,7]):
  writable.append(any(int(line[0].split('-')[0],16)<=item['name_pointer'] and item['name_pointer']+size<=int(line[0].split('-')[1],16) and line[1]=='rw-p' for line in maps))
 return dict(writable_names=writable,run_owners=[tools[i]['run']==(val('&'+name) if name else 0) for i,name in enumerate(['linker_main','library_main','dumper_main','editor_main','helper_main',None])],event=event,name=LOGICAL,eip=val('$eip'),esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),caller=words(esp,1)[0],formal_words=formals,argv=values,program=string(val('program_name')),tools=tools,stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',errors=[val('*dispatcher_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],ctype=mem(val('recon_pctype')-2,514).hex(),mb_cur_max=val('recon_mb_cur_max'))
class Return(gdb.Breakpoint):
 def stop(self):
  after=snap('return');events.append(after);persist()
  assert after['esp']==before['esp']+4 and after['eax']==3
  assert all(before[r]==after[r] for r in ['ebx','esi','edi','ebp'])
  self.enabled=False;return False
class Entry(gdb.Breakpoint):
 def stop(self):
  global before
  before=snap('entry' if ALLOWED else 'frontier');events.append(before);persist()
  if not ALLOWED:return True
  caller=gdb.newest_frame().older();assert caller
  before['actual_caller']=caller.name();persist();Return('*'+str(before['caller']),internal=True);return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited)
gdb.execute('set pagination off');gdb.execute('set confirm off');Entry('*'+LOGICAL,internal=True)
gdb.execute('run '+ARGUMENT+' >'+STDOUT+' 2>'+STDERR)
assert events and (terminal or events[-1]['event']=='frontier')
if terminal:assert terminal==[dict(event='exit',code=3)] and len(events)==2
persist()
"""
    for key,value in dict(OUTPUT=str(directory/'state.json'),LOGICAL=logical,ALLOWED=allowed,ARGUMENT='-helper' if args.phase=='helper' else '--direct',STDOUT=str(directory/'stdout.bin'),STDERR=str(directory/'stderr.bin')).items():source=source.replace(key,repr(value))
    (directory/'observe.py').write_text(source);env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(key,None)
    command=['gdb','-q','-nx','-batch','-ex','file '+str(BINARY/'build/native/dispatcher'),'-ex','source '+str(directory/'observe.py')];t=time.perf_counter();p=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=15);elapsed=time.perf_counter()-t
    (directory/'gdb.stdout').write_bytes(p.stdout);(directory/'gdb.stderr').write_bytes(p.stderr);(directory/'receipt.json').write_text(json.dumps(dict(phase=args.phase,trial=args.trial,seconds=elapsed,exit=p.returncode,command=command,elf_sha256=hashlib.sha256((BINARY/'build/native/dispatcher').read_bytes()).hexdigest(),source_sha256={str(p.relative_to(BINARY)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [BINARY/'src/link.c',BINARY/'scripts/native-dispatcher.c',BINARY/'scripts/observe-dispatcher.py']}),indent=2)+'\n');print(directory,flush=True);assert p.returncode==0,(p.stdout,p.stderr)
if __name__=='__main__':main()
