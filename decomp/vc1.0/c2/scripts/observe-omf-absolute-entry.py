"""Observe absolute-reference forwarding inputs and lookup-result identity."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-absolute-abi');receipt.mkdir();name='@omf_output_bytes@8';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='absolute-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6b2c0;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 258 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='absolute-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
symbol=int(gdb.parse_and_eval('symbol'));displacement=int(gdb.parse_and_eval('displacement'));f=gdb.selected_frame();caller=f.older();assert caller.name()=='omf_output_relocation';caller.select();owner=int(gdb.parse_and_eval('referenced'));identifier=int(gdb.parse_and_eval('initializer->content.relocation.symbol_id'));f.select();bucket=identifier&63;member=gdb.parse_and_eval('global_scope')['buckets'][bucket];index=0
while int(member)and int(member['identifier'])!=identifier and index<1024:member=member['hash_next'];index+=1
assert int(member)and index<1024
print('C2_ABSOLUTE '+json.dumps(dict(displacement=displacement,symbol=symbol,referenced_owner=owner,identifier=identifier,bucket=bucket,index=index,member=int(member),caller=caller.name())))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_output_bytes','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_ABSOLUTE (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==2
parts=next(v for v in lines if v.startswith('omf_absolute ')).split()[1:];observed={parts[n]:int(parts[n+1],10 if parts[n]in['identifier','bucket','index']else 16)for n in range(0,len(parts),2)};assert args==[observed['displacement'],observed['symbol']]and observed['return_address']==0x6b145
def shape(d):
 assert d['symbol']==d['referenced_owner']==d['member']and d['symbol']and d['bucket']==d['identifier']&63;return {k:d[k]for k in['displacement','identifier','bucket','index']}
assert shape(observed)==shape(native)and observed['displacement']==0xfffffffc
r=dict(matched=True,function=name,original_va='0x6b2c0',formals=[dict(name=k,location=l,type=t,original=observed[k],native=native[k])for k,l,t in [('displacement','ECX','unsigned32'),('symbol','EDX','SymbolHeader pointer')]],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6b2c0..6b2d0 forwards ECX displacement/EDX symbol as child ECXsymbol/EDX9/stack displacement/0; RET0, no other incoming registers/stack/flags/x87/out pointer. Actual original EBP referenced pointer at return6b145 aliases lookup member for caller initializer id, native omf_output_relocation referenced/local id and closed global chain match. Wrapper never dereferences symbol; no invented pointee fields.',shape=shape(observed),original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual displacement-4 and proved symbol pointer identity only; child behavior/data not inferred. Identifier/hash traversal is observer ownership proof,1024 bound observer-only.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual absolute-reference forwarding inputs match',r['shape'])
