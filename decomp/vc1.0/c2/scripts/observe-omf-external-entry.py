"""Observe actual EXTDEF name context/global counter without old output bytes."""
from pathlib import Path
import hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-external-abi');receipt.mkdir();name='@omf_emit_external@4';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='external-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6ad80;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='external-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval('symbol'));b=gdb.parse_and_eval('symbol');identifier=int(b['identifier']);bucket=identifier&63;member=gdb.parse_and_eval('global_scope')['buckets'][bucket];index=0
while int(member)and int(member)!=p and index<1024:member=member['hash_next'];index+=1
assert int(member)==p and index<1024
print('C2_EXTERNAL '+json.dumps(dict(symbol=p,bucket=bucket,index=index,member=int(member),next_external=int(gdb.parse_and_eval('omf_next_external')),header_hex=read(p+12,4).hex(),identifier_hex=read(p+28,4).hex(),attributes_hex=read(p+36,4).hex(),name_hex=(b['name'].string(encoding='latin1').encode('latin1')+bytes([0])).hex())))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_emit_external','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_EXTERNAL (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert len(lines)==10 and lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==1
parts=lines[4].split();observed={parts[n]:int(parts[n+1],10 if parts[n]in ['bucket','index','next_external']else 16)for n in range(1,len(parts),2)}
for i,key in enumerate(['header','identifier','attributes','name'],5):observed[key+'_hex']=lines[i].removeprefix('omf_external_'+key+'_hex ')
assert args[0]==observed['symbol']==observed['member']and native['symbol']==native['member']and args[0]and native['symbol']and observed['bucket']==native['bucket']and observed['index']==native['index']and observed['next_external']==native['next_external']==1
assert original.read(0x85078,4)==bytes([1,0,0,0])
def projection(d):
 h=bytes.fromhex(d['header_hex']);i=bytes.fromhex(d['identifier_hex']);a=bytes.fromhex(d['attributes_hex']);assert len(h)==len(i)==len(a)==4
 return dict(kind=struct.unpack('<b',h[:1])[0],name_class=h[1],symbol_flags=struct.unpack('<H',h[2:])[0],identifier=struct.unpack('<I',i)[0],attributes=struct.unpack('<I',a)[0],name_hex=d['name_hex'])
a=projection(observed);b=projection(native);assert a==b and observed['bucket']==a['identifier']&63
r=dict(matched=True,function=name,original_va='0x6ad80',formals=[dict(name='symbol',location='ECX',type='SymbolHeader pointer',original=observed['symbol'],native=native['symbol'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6ad80..6adc0 receives sole ECX Symbol pointer and mutable unsigned next-external global85078. It assigns Symbol.field14 after begin8c/NULL then invokes name provider, counted-string append and zero type byte. No hidden incoming stack/register/flags/x87/output pointer, RET0. Actual closed global_scope owner/name-provider kind/field0d/flags/attributes/name and counter1 observed; old Symbol.field14 write-only output not inspected, dumped or seeded.',original=observed,native=native,observed_fields=a,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole Symbol pointer/closed hash owner and name-provider defined context; native typed next-external initializer1 equals original PE85078 and actual entry1. No old output-word read/seed or caller output alias invented, no child semantic or completed artifact claim.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual EXTDEF input/name context and counter1 match',a)
