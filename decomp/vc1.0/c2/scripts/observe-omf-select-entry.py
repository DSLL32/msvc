"""Observe actual OMF segment-selection pointer and combine mode."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser();parser.add_argument('--parse-only',action='store_true');options=parser.parse_args();started=time.perf_counter();receipt=Path('cases/omf-select-abi');receipt.mkdir(exist_ok=options.parse_only);name='@omf_select_segment@4';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
original_seconds=native_seconds=None
if options.parse_only:
 binding=json.loads((receipt/'original-binding.json').read_text());failure=json.loads((receipt/'parser-failure.json').read_text())
 assert all(sha(receipt/n)==h for n,h in binding['files'].items())and sha('functions.json')==binding['config_sha256']and sha('src/reference-stdio-observer.c')==binding['observer_source_sha256']and sha('build/native/C23232.EXE')==failure['native_executable_sha256']and sha(receipt/'native.txt')==failure['native_log_sha256']
else:
 with capture_lock():
  scratch=Path(tempfile.mkdtemp(prefix='select-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
  for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
  for r in phase['inputs']:
   p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
  original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6b6a0;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
  for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
  env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
 binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
 scratch=Path(tempfile.mkdtemp(prefix='select-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
 for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
 env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
 script="import gdb,json; read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n)); p=int(gdb.parse_and_eval('segment')); print('C2_SELECT '+json.dumps(dict(segment=p,owner=int(gdb.parse_and_eval('debug_symbols_segment')),segment_hex=read(p,72).hex(),segment_size=int(gdb.parse_and_eval('sizeof(*segment)')))))"
 command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_select_segment','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout
native=json.loads(re.search(rb'C2_SELECT (.*)',(receipt/'native.txt').read_bytes())[1]);lines=(receipt/'original.txt').read_text().splitlines();assert len(lines)==7 and lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==1
parts=lines[4].split();observed={parts[n]:int(parts[n+1],16)for n in range(1,len(parts),2)};observed['segment_hex']=lines[5].removeprefix('omf_select_segment_hex ')
assert args[0]==observed['segment']==observed['owner']and native['segment']==native['owner']and args[0]and native['segment']and native['segment_size']==72
raw=bytes.fromhex(observed['segment_hex']);other=bytes.fromhex(native['segment_hex']);assert len(raw)==len(other)==72 and raw[53]==other[53]==0
r=dict(reused_raw_observations=options.parse_only,raw_parser_failure="parser-failure.json"if options.parse_only else None,matched=True,function=name,formals=[dict(name='segment',location='ECX',type='Segment pointer',original=observed['segment'],native=native['segment'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6b6a0..6b6bf consumes ECX Segment pointer, reads only combine byte53 and passes context to begin_record, stores returned pointer in omf_current_record. RET0, no other incoming register/stack/flags/x87 or hidden pointer. Actual debug_symbols_segment owner/full72-byte raw descriptor/combine0 observed; unused pointer bytes not equated.',original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual debug-symbol Segment alias and combine0 -> LEDATA32 a1. Original byte subtract7/compare1 is exactly combine==7 -> COMDAT32 c3; cold other values require controlled proof. Context fields are passed to already reconstructed child, no arbitrary initialized Segment or final object claim.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual selection Segment/debug owner and combine0 match')
