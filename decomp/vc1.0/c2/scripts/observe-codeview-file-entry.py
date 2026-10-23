"""Actual CodeView function-pointer formal, with read-only parent input proof."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--function',default='@read_codeview_initializers@4',choices=['@read_codeview_initializers@4']);parser.add_argument('--receipt',type=Path,default=Path('cases/dbglist-codeview-file-abi'));args=parser.parse_args()
started=time.perf_counter();receipt=args.receipt;receipt.mkdir();fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function;pointer_slot=1 if name=='_coff_output_size'else 0
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='function-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for r in phase['inputs']:
        source=fixture/r['path'];assert sha(source)==r['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name);expected_va={'@read_codeview_initializers@4':0x69ba0}[name];assert row['start']==expected_va
    plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],257+pointer_slot,name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment()
    for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
    env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
    begin=time.perf_counter();result=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
    (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='codeview-file-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="""import gdb,json,os,stat
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval('file'));s=gdb.parse_and_eval('file').dereference();base=int(s['_base']);ptr=int(s['_ptr']);used=ptr-base if ptr and base else 0;valid=used+max(int(s['_cnt']),0);fd=int(s['_file']);host=int(gdb.parse_and_eval('recon_osfhnd[%d]'%fd))-1;pid=gdb.selected_inferior().pid
assert valid<=4096 and fd<64
info=open('/proc/%d/fdinfo/%d'%(pid,host)).read();position=int(next(x.split()[1]for x in info.splitlines()if x.startswith('pos:')));st=os.stat('/proc/%d/fd/%d'%(pid,host));path=os.readlink('/proc/%d/fd/%d'%(pid,host));assert path.endswith('/irdb')
f=dict(owner=int(gdb.parse_and_eval('codeview_type_file')),stream=p,flags=int(s['_flag']),count=int(s['_cnt']),fd=fd,size=int(s['_bufsiz']),null=int(not ptr and not base),used=used,valid=valid,position=position,osfile=int(gdb.parse_and_eval('recon_osfile[%d]'%fd)),file_bytes=st.st_size,file_type=1 if stat.S_ISREG(st.st_mode)else 0,temporary_null=int(not int(s['_tmpfname'])),file_hex=read(p,32).hex(),buffer_hex=read(base,valid).hex()if valid else'',host_path=path)
s=gdb.parse_and_eval('recon_iob[1]');used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_CODEVIEW_FILE '+json.dumps(dict(file=f,column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=read(int(s['_base']),used).hex()if used else ''))))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break read_codeview_initializers','-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin;(receipt/'native.txt').write_bytes(result.stdout);assert result.returncode==0,result.stdout
native=json.loads(re.search(rb'C2_CODEVIEW_FILE (.*)',result.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name];arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==1
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};parts=lines[4].split();original_file={parts[n]:int(parts[n+1],16 if parts[n]in ['owner','stream']else 10)for n in range(1,len(parts),2)};original_file.update(file_hex=lines[5].removeprefix('codeview_file_hex '),buffer_hex=lines[6].removeprefix('codeview_buffer_hex '));assert lines[7]=='entry_end'
assert arguments[0]==original_file['owner']==original_file['stream']and native['file']['owner']==native['file']['stream']and arguments[0]and native['file']['stream']
fields=['flags','count','fd','size','null','used','valid','position','osfile','file_bytes','file_type','temporary_null','buffer_hex'];assert {k:original_file[k]for k in fields}=={k:native['file'][k]for k in fields},(original_file,native['file'])
assert original_file['file_bytes']==55 and original_file['position']==0
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,original_va=hex(expected_va),formals=[dict(name='file',location='ECX',type='input FILE pointer',original=arguments[0],native=native['file']['stream'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original69ba0..69bd0 consumes ECX FILE pointer, savesESI then assigns it; incoming EDX/stack/flags/x87/other GPRs not inputs, RET0. FILE pointee is mutable input stream, not a hidden output pointer.',original_file=original_file,native=native,original_stdout=stdout,original_state=state,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole FILE pointer aliases distinct codeview_type_file owner. Defined FILE flags/count/fd/bufsize/ptr-base topology/initialized buffer, temporary-name-null, lowio flags, descriptor position0/file size55/typeDISK compare. Both raw32-byte FILEs retained; unused char-buffer contents and raw host pointers/handles not normalized/asserted equal. Native OS path endsirdb; phase input55-byte hash mandatory. No injected target calls: original uses read-only DuplicateHandle/position/size/type queries, native reads procfs. No wrapper/parser/CRT-failure behavior accepted by this input proof.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual db FILE owner and defined stream state match',{k:original_file[k]for k in fields if k!='buffer_hex'})
