"""Observe marked Symbol ownership and only consumed descriptor input ranges."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser();parser.add_argument('--native-only',action='store_true');options=parser.parse_args();started=time.perf_counter();original_receipt=Path('cases/omf-mark-abi');receipt=Path('cases/omf-mark-native-abi')if options.native_only else original_receipt;receipt.mkdir();name='@omf_mark_symbol@4';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
original_seconds=None
if options.native_only:
 binding=json.loads((original_receipt/'original-binding.json').read_text());assert all(sha(original_receipt/n)==h for n,h in binding['files'].items())and sha('functions.json')==binding['config_sha256']and sha('src/reference-stdio-observer.c')==binding['observer_source_sha256']and sha('build/reference-stdio-observer.exe')==binding['observer_sha256']
else:
 with capture_lock():
  scratch=Path(tempfile.mkdtemp(prefix='mark-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
  for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
  for r in phase['inputs']:
   p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
  original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']=='_omf_mark_symbol');assert row['start']==0x6ab20;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
  for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
  env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
 binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='mark-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval('$ecx'))&0xffffffff;b=gdb.Value(p).cast(gdb.lookup_type('BackendSymbol').pointer());identifier=int(b['symbol']['identifier']);bucket=identifier&63;member=gdb.parse_and_eval('global_scope')['buckets'][bucket];index=0
while int(member)and int(member)!=p and index<1024:member=member['hash_next'];index+=1
assert int(member)==p and index<1024
flags=int(b['output_flags']);kind=int(b['symbol']['kind']);extent=6 if flags&5==4 and kind in [1,4]else 2
print('C2_MARK '+json.dumps(dict(pointer=p,bucket=bucket,index=index,member=int(member),header_hex=read(p+12,4).hex(),identity_hex=read(p+24,8).hex(),flags_hex=read(p+34,extent).hex(),name_hex=(b['symbol']['name'].string(encoding='latin1').encode('latin1')+bytes([0])).hex())))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_mark_symbol','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_MARK (.*)',r.stdout)[1]);lines=(original_receipt/'original.txt').read_text().splitlines();assert len(lines)==10 and lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==1
parts=lines[4].split();observed={parts[n]:int(parts[n+1],10 if parts[n]in ['bucket','index']else 16)for n in range(1,len(parts),2)}
for i,key in enumerate(['header','identity','flags','name'],5):observed[key+'_hex']=lines[i].removeprefix('omf_mark_'+key+'_hex ')
assert args[0]==observed['pointer']==observed['member']and native['pointer']==native['member']and args[0]and native['pointer']and observed['bucket']==native['bucket']and observed['index']==native['index']
def projection(d):
 h=bytes.fromhex(d['header_hex']);i=bytes.fromhex(d['identity_hex']);f=bytes.fromhex(d['flags_hex']);assert len(h)==4 and len(i)==8 and len(f)in [2,6]
 return dict(kind=struct.unpack('<b',h[:1])[0],symbol_flags=struct.unpack('<H',h[2:])[0],segment_identifier=struct.unpack('<I',i[:4])[0],identifier=struct.unpack('<I',i[4:])[0],output_flags=struct.unpack('<H',f[:2])[0],attributes=struct.unpack('<I',f[2:])[0]if len(f)==6 else None,name_hex=d['name_hex'])
a=projection(observed);b=projection(native);assert a==b and observed['bucket']==a['identifier']&63
assert not a['symbol_flags']&0x20,'COMDAT return-descriptor inputs require additional actual observation before acceptance'
r=dict(original_reference=dict(directory=str(original_receipt),binding_sha256=sha(original_receipt/'original-binding.json')),matched=True,function=name,original_name_before_abi='_omf_mark_symbol',original_va='0x6ab20',formals=[dict(name='symbol',location='ECX',type='SymbolHeader pointer',original=observed['pointer'],native=native['pointer'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6ab20..6acb7 receives sole ECX descriptor, RET0. Consumes output_flags WORD34, kind byte12, flagsWORD14, attributesDWORD36, and segment identifierDWORD24 only on lookup paths. All actual projected values/name/closed global_scope bucket(identifier&63)/chain owner observed; no hidden incoming stack/register/flags/x87/output pointer. Other descriptor words/uninitialized tails not dumped. Actual non-COMDAT branch has no returned Segment input; cold COMDAT shape is controlled separately.',original=observed,native=native,consumed_fields=a,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole pointer and consumed field ranges12..16/24..32/34..40 (attributes omitted if not consumed), initialized owned name and global hash ownership. Padding byte13 retained raw but not equated; identity used for owner, no entire descriptor/adjacent storage snapshot. Non-COMDAT actual path only; cold branch child behavior not accepted by this observation.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual marked symbol inputs match',a)
