"""Read-only original/native actual module-directive ABI before reconstruction."""
from pathlib import Path
import hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
start=time.perf_counter();receipt=Path('cases/omf-output-record-abi');receipt.mkdir();name='@omf_output_record@4';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='directive-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,rebuilt,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6b860;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
 t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='directive-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json,struct
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval('directive'));d=gdb.parse_and_eval('*directive');kind=int(d['kind']);text=int(d['text']);raw=b''
if kind!=1 and text:
 if kind==7:
  n=struct.unpack('b',read(text,1))[0];assert 0<=n<=127;raw=read(text,n+1)
 else:
  for i in range(512):
   c=read(text+i,1);raw+=c
   if c==b'\\0':break
  assert raw[-1:]==b'\\0'
s=gdb.parse_and_eval('recon_iob[1]');used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_DIRECTIVE '+json.dumps(dict(pointer=p,head=int(gdb.parse_and_eval('module_directives')),next=int(d['next']),kind=kind,text=text,raw_hex=read(p,12).hex(),text_hex=raw.hex(),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=read(int(s['_base']),used).hex()if used else ''))))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_output_record','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_DIRECTIVE (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert len(lines)==8 and lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==1
parts=lines[4].split();observed={parts[n]:int(parts[n+1],10 if parts[n]=='kind'else 16)for n in range(1,len(parts),2)};observed['raw_hex']=lines[5].removeprefix('omf_directive_hex ');observed['text_hex']=lines[6].removeprefix('omf_directive_text_hex ')
for d in [observed,native]:
 assert d['pointer']==d['head']and d['pointer']and d['next']!=d['pointer'];assert struct.unpack('<3I',bytes.fromhex(d['raw_hex']))==(d['next'],d['kind'],d['text'])
assert arguments[0]==observed['pointer']and observed['kind']==native['kind']and bool(observed['next'])==bool(native['next'])and bool(observed['text'])==bool(native['text'])and observed['text_hex']==native['text_hex']
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in ['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ');parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}and {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,original_va='0x6b860',formals=[dict(name='directive',location='ECX',type='ModuleDirective pointer',original=arguments[0],native=native['pointer'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6b860..6b912 +11-way table6b914..6b940 consumes ECX mutable12-byte ModuleDirective*, RET0; savedESI assigned internally, no other incoming register/stack/x87/flags/hidden pointer. Local stack word is only read after unsupported-kind diagnostic were to return; actual supported kind/input observed, invalid control stops at diagnostic, no assumed initialization.',original=observed,native=native,binding=binding,original_state=state,original_stdout=stdout,native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-start,scope='Actual current directive pointer aliases module_directives head;12-byte descriptor fields and next/text pointer null/identity relations plus consumed text bytes are observed. Kind1 original text is overwritten before any read, so no pointee dump for that unused input. Other kinds not claimed executed.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual OMF directive',observed['kind'],bytes.fromhex(observed['text_hex']))
