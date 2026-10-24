"""Observe public-symbol callback current-function alias and active fields."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-symbol-callback-abi');receipt.mkdir();name='@omf_output_symbol@4';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='symbol-callback-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['start']==0x6b710);assert row['start']==0x6b710;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='symbol-callback-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
p=int(gdb.parse_and_eval('$ecx'))&0xffffffff;b=gdb.Value(p).cast(gdb.lookup_type('FunctionSymbol').pointer());state=dict(pointer=p,current=int(gdb.parse_and_eval('current_symbol')),flags=int(b['symbol']['field0e']),caller=gdb.selected_frame().older().name());payload={}
if not state['flags']&32:payload['segment_id']=int(b['symbol']['field18']['identifier'])
print('C2_SYMBOL '+json.dumps(dict(state=state,payload=payload)))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_output_symbol','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_SYMBOL (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name]and len(lines[0].split())==6
parts=next(v for v in lines if v.startswith('omf_symbol ')).split()[1:];state={parts[n]:int(parts[n+1],10 if parts[n]=='flags'else 16)for n in range(0,len(parts),2)};payload={}
for v in lines:
 if v.startswith('omf_symbol_segment_id '):payload['segment_id']=int(v.split()[1])
observed=dict(state=state,payload=payload);assert int(lines[0].split()[5],16)==state['pointer']and state['pointer']==state['current']and native['state']['pointer']==native['state']['current']and native['state']['caller']=='emit_machine_instructions'
def shape(d):return dict(flags=d['state']['flags'],payload=d['payload'])
assert shape(observed)==shape(native)
r=dict(matched=True,function=name,previous_provisional_name='_omf_output_symbol',original_va='0x6b710',formals=[dict(name='symbol',location='ECX',type='FunctionSymbol pointer',original=state['pointer'],native=native['state']['pointer'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6b710..6b72c receives sole ECX symbol, RET0. No EDX/stack/flags/x87/out pointer. Actual original831ac current_symbol and native current_symbol pointer aliases and caller observed. Only flags16 at14 and conditional segment identifier24 consumed directly; child return segment is future lookup output, not invented input.',shape=shape(observed),original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole formal/current function and directly consumed conditional fields; no object tail/name/child destination dump or seed. Full body/control/canonical/fresh matching pending.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual public symbol callback sole formal/current alias matches',r['shape'])
