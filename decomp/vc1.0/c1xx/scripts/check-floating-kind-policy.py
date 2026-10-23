"""Minimal native-only injected type fallback check; not an original public case.

Inject the actual caller's defined multiply result (3.0) at that callee return,
then test the precise type classification with genuine-frame locals and owners.
"""
import hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import BINARY,ROOT,CAPTURE_ROOT,native_work,windows
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 started=time.perf_counter();directory=Path(tempfile.mkdtemp(prefix='floating-kind-policy-',dir=CAPTURE_ROOT));fixture=BINARY.parent/'fixtures/c1xx-state-machine-debug';phase=json.load(open(fixture/'manifest.json'))['phases']['c1'];executable=BINARY/'build/native-trace/C1XX3232.EXE'
 frozen={str(p):sha(p) for p in [executable,fixture/'manifest.json',BINARY/'functions.json',BINARY/'cases/native-traces.json']};observations=[]
 for flags in [0x108,0x100]:
  run=directory/hex(flags);run.mkdir();work,drives=native_work(run,phase['canonical_working_directory'])
  for item in phase['inputs']:
   source=fixture/item['path'];raw=source.read_bytes();assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256'];frozen[str(source)]=sha(source);(work/Path(item['path']).name).write_bytes(raw)
  commands=['set pagination off','break fold_floating_binary','run > '+str(run/'stdout.bin')+' 2> '+str(run/'stderr.bin'),'python','import gdb,json','assert gdb.newest_frame().name()=="fold_floating_binary"','assert int(gdb.parse_and_eval("expression->kind"))==0x53','assert int(gdb.parse_and_eval("expression->type->flags"))&0xefff==0x108','end','disable 1','break multiply_extended','continue','python','assert gdb.newest_frame().name()=="multiply_extended"','output=gdb.parse_and_eval("result")','output["low"].assign(0)','output["high"].assign(0xc0000000)','output["exponent"].assign(0x4000)','output["format"].assign(8)','end','return (int)0','python','assert gdb.newest_frame().name()=="fold_floating_binary"','gdb.parse_and_eval("expression")["type"]["flags"].assign('+str(flags)+')','end','disable 2']
  if flags==0x108:commands+=['break new_literal_constant','continue','python','assert gdb.newest_frame().name()=="new_literal_constant"','assert int(gdb.parse_and_eval("kind"))==5']
  else:commands+=['continue','python','frame=gdb.newest_frame()','assert frame.name()=="fold_floating_binary"','assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5','assert bytes(gdb.selected_inferior().read_memory(frame.pc()-1,1))==b"\\xcc"','assert int(gdb.parse_and_eval("type"))==0x100']
  commands+=['open('+repr(str(run/'observation.json'))+',"w").write(json.dumps(dict(injected_type='+str(flags)+',stopped_function=gdb.newest_frame().name(),status="pass"),indent=2)+"\\n")','end'];(run/'commands.gdb').write_text('\n'.join(commands)+'\n')
  environment={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
  for key in ('CL','_CL_','LINK','_LINK_'):environment.pop(key,None)
  environment.update(drives,TZ='PST8PDT',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')),RECON_PROGRAM_PATH=phase['command_line_template'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin')))
  before=time.perf_counter();result=subprocess.run(['gdb','-q','-batch','-x',str(run/'commands.gdb'),'--args',str(executable)],cwd=work,env=environment,capture_output=True,text=True,timeout=60);(run/'gdb.log').write_text(result.stdout+result.stderr);assert result.returncode==0 and (run/'observation.json').exists(),result.stdout+result.stderr
  observation=json.load(open(run/'observation.json'));observation['debugger_seconds']=time.perf_counter()-before;observations.append(observation)
 assert all(sha(Path(p))==digest for p,digest in frozen.items()),'frozen inputs changed'
 report=dict(status='pass',scope='Native-only exact fallback/control, injected defined multiplication output and type. No original invalid-type equivalence, public reachability or appcredit claim.',frozen=frozen,observations=observations,total_seconds=time.perf_counter()-started);(directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n');print(directory/'receipt.json')
if __name__=='__main__':main()
