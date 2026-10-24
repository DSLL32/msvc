"""Observe size-reference forwarding inputs and lookup-result identity."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-codeview-global-abi');receipt.mkdir();name='@write_codeview_global_data@4';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='codeview-data-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['start']==0x69f30);assert row['start']==0x69f30;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='codeview-data-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
s=gdb.parse_and_eval('symbol');p=int(s);identifier=int(s['identifier']);bucket=identifier&63;member=gdb.parse_and_eval('global_scope')['buckets'][bucket];index=0
while int(member)and int(member)!=p and index<1024:member=member['hash_next'];index+=1
assert int(member)==p and index<1024
coff=int(gdb.parse_and_eval('coff_object'));first=int(gdb.parse_and_eval('byte_block_first'));debug=gdb.parse_and_eval('debug_symbols_segment');state=dict(symbol=p,identifier=identifier,bucket=bucket,index=index,member=int(member),kind=int(s['kind']),flags=int(s['field0e']),type=int(gdb.Value(p+32).cast(gdb.lookup_type('unsigned short').pointer()).dereference()),coff=coff,byte_first=first,debug=int(debug),debug_offset=int(debug['symbol']['field04']),name_hex=(s['name'].string(encoding='latin1').encode('latin1')+bytes([0])).hex());payload={}
if coff and not first:payload['offset']=int(s['field04'])
caller=gdb.selected_frame().older();caller.select();state.update(caller=caller.name(),caller_symbol=int(gdb.parse_and_eval('symbol')));print('C2_GLOBAL_DATA '+json.dumps(dict(state=state,payload=payload)))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break write_codeview_global_data','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_GLOBAL_DATA (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==1
parts=next(v for v in lines if v.startswith('codeview_global ')).split()[1:];state={parts[n]:int(parts[n+1],16 if parts[n]in['symbol','member','debug','debug_offset','return_address']else 10)for n in range(0,len(parts),2)};state['name_hex']=next(v.split()[1]for v in lines if v.startswith('codeview_global_name_hex '));payload={}
for v in lines:
 if v.startswith('codeview_global_offset '):payload['offset']=int(v.split()[1],16)
observed=dict(state=state,payload=payload);assert args==[state['symbol']]and native['state']['symbol']==native['state']['caller_symbol']and native['state']['caller']=='write_debug_symbol'
def shape(d):
 s=d['state'];assert s['symbol']==s['member']and s['symbol']and s['debug']and s['bucket']==s['identifier']&63
 return dict(**{k:s[k]for k in ['identifier','bucket','index','kind','flags','type','coff','byte_first','debug_offset','name_hex']},payload=d['payload'])
assert shape(observed)==shape(native)
r=dict(matched=True,function=name,original_va='0x69f30',formals=[dict(name='symbol',location='ECX',type='SymbolHeader pointer',original=state['symbol'],native=native['state']['symbol'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original69f30..69fef sole ECXsymbol, RET0; no other incoming register/stack/flags/x87/out pointer. Actual closed global hash and native write_debug_symbol formal alias. Kind/flags/full name/typeWORD and COFF/byte-block conditional offset, debug segment global/defined offset observed. Name provider uses only name/kind/convention flags on actual non-C++ input; no opaque descriptor or output buffer.',shape=shape(observed),original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual pointer/owner and active scalar/name/context only, no unused offset under nonCOFF/descriptor tail/stack record dump or seed. Child format/select/copy may change later state; preserve original reload/order. Full body/control/canonical/fresh matching pending.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual global CodeView data input matches',r['shape'],hex(state['return_address']))
