"""Observe one typed fixup pointer and only consumed entry fields/owned slot."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-fixup-entry-abi');receipt.mkdir();name='@omf_write_fixup@4';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='fixup-entry-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6c680;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='fixup-entry-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
p=int(gdb.parse_and_eval('fixup'));pool=int(gdb.parse_and_eval('&omf_fixups[0]'));f=gdb.parse_and_eval('fixup');flags=int(f['flags']);frame=int(f['frame_code']);slot=(p-pool)//24;assert p==pool+slot*24 and 0<=slot<250
state=dict(pointer=p,pool=pool,slot=slot,count=int(gdb.parse_and_eval('omf_fixup_count')),type=int(f['type']),location=int(f['location']),flags=flags,frame=frame,target=int(f['target_code']),caller=gdb.selected_frame().older().name());payload={}
if not flags&8:payload['addend']=int(f['addend'])
if not flags&2 and frame<=2:payload['frame_datum']=int(f['frame_datum'])
if not flags&4:payload['target_datum']=int(f['target_datum'])
caller=gdb.selected_frame().older();caller.select();state['owner']=int(gdb.parse_and_eval('fixup'));state['loop_index']=int(gdb.parse_and_eval('index'));print('C2_FIXUP_ENTRY '+json.dumps(dict(state=state,payload=payload)))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_write_fixup','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_FIXUP_ENTRY (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name]and len(lines[0].split())==6
parts=next(v for v in lines if v.startswith('omf_fixup_entry ')).split()[1:];state={parts[n]:int(parts[n+1],16 if parts[n]in['pointer','pool','flags','return_address']else 10)for n in range(0,len(parts),2)};payload={}
for v in lines:
 if v.startswith('omf_fixup_addend '):payload['addend']=int(v.split()[1],16)
 elif v.startswith('omf_fixup_frame_datum '):payload['frame_datum']=int(v.split()[1])
 elif v.startswith('omf_fixup_target_datum '):payload['target_datum']=int(v.split()[1])
observed=dict(state=state,payload=payload);assert int(lines[0].split()[5],16)==state['pointer']and state['pointer']==state['pool']+24*state['slot']and state['pool']==0x79154 and state['return_address']==0x6c41e and native['state']['caller']=='omf_write_fixups'and native['state']['owner']==native['state']['pointer']and native['state']['loop_index']==state['slot']
def shape(d):return dict(**{k:d['state'][k]for k in ['slot','count','type','location','flags','frame','target']},payload=d['payload'])
assert shape(observed)==shape(native)
r=dict(matched=True,function=name,original_va='0x6c680',formals=[dict(name='fixup',location='ECX',type='OmfFixup pointer',original=state['pointer'],native=native['state']['pointer'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6c680..6c7b2 receives sole ECX fixup, RET0. No EDX/stack/flags/x87/out pointer. Actual original fixed pool slot/counter and native caller fixup/index aliases prove owner. Defined type/location/flags/frame/target and only conditional addend/datum fields observed, no reserved/padding bytes. Child begin result is future output, not fabricated input; actual post-child reload ordering must remain in full body.',shape=shape(observed),original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole formal/closed250x24 slot and contextual consumed fields, no raw entry/record destination/irrelevant tail dump or seed. Full body/control/canonical/fresh matching pending.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual fixup serializer sole formal/typed fields match',r['shape'])
