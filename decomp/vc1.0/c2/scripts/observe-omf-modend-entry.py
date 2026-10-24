"""Observe no-formal fixup writer count and typed pool identity, no field dump."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-modend-abi');receipt.mkdir();name='_omf_write_module_end';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='modend-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6aad0;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 0 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='modend-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="import gdb,json;print('C2_MODEND '+json.dumps(dict(caller=gdb.selected_frame().older().name())))"
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_write_module_end','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_MODEND (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name]and len(lines[0].split())==5
observed=dict(return_address=int(next(v.split()[2]for v in lines if v.startswith('omf_modend ')),16));assert observed['return_address']==0x6aaac and native['caller']=='omf_finish_module'
r=dict(matched=True,function=name,original_va='0x6aad0',formals=[],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6aad0..6aaea sets EDX0/ECX8a before begin_record, then uses only returned EAXrecord/cursor/count and returns RET0. No incoming register/stack/flags/x87/global/output pointer data consumed. Actual original caller return6aaac/native omf_finish_module observed; record is future child result, not an incoming snapshot or fabricated pointer.',original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual no-formal boundary/caller only. No globals/raw record/destination or arbitrary source data captured/seeded. Whole26-byte body/canonical/controls/fresh case completion still pending.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual MODEND no-formal/caller boundary matches')
