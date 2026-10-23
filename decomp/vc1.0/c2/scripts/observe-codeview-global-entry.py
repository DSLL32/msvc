"""Actual CodeView function-pointer formal, with read-only parent input proof."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--function',default='@write_codeview_typedef@4',choices=['@write_codeview_typedef@4']);parser.add_argument('--receipt',type=Path,default=Path('cases/dbglist-codeview-typedef-abi'));args=parser.parse_args()
started=time.perf_counter();receipt=args.receipt;receipt.mkdir();fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function;pointer_slot=1 if name=='_coff_output_size'else 0
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='function-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for r in phase['inputs']:
        source=fixture/r['path'];assert sha(source)==r['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name);expected_va={'@write_codeview_typedef@4':0x69d10}[name];assert row['start']==expected_va
    plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],257+pointer_slot,name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment()
    for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
    env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
    begin=time.perf_counter();result=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
    (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='codeview-global-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="""import gdb,json
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval('symbol'));identifier=int(gdb.parse_and_eval('symbol->identifier'));bucket=identifier&63;member=gdb.parse_and_eval('global_scope')['buckets'][bucket];index=0
while int(member)and int(member)!=p and index<1024:member=member['hash_next'];index+=1
assert int(member)==p and index<1024
s=gdb.parse_and_eval('recon_iob[1]');used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_GLOBAL '+json.dumps(dict(pointer=p,descriptor_hex=read(p,44).hex(),name_hex=(gdb.parse_and_eval('symbol->name').string()+'\\0').encode('latin1').hex(),bucket=bucket,index=index,member=int(member),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=read(int(s['_base']),used).hex()if used else ''))))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break write_codeview_typedef','-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin;(receipt/'native.txt').write_bytes(result.stdout);assert result.returncode==0,result.stdout
native=json.loads(re.search(rb'C2_GLOBAL (.*)',result.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name];arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==1
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};descriptor=bytes.fromhex(lines[4].removeprefix('descriptor_hex '));symbol_name=lines[5].removeprefix('descriptor_name_hex ')
parts=lines[6].split();owner={parts[n]:int(parts[n+1],16 if parts[n]=='member'else 10)for n in range(1,len(parts),2)};assert lines[7]=='entry_end'
assert arguments[0]==owner['member']and native['pointer']==native['member']and native['index']==owner['index']and native['bucket']==owner['bucket']
word=lambda data,a:struct.unpack_from('<I',data,a)[0]
projection=lambda data:dict(kind=struct.unpack_from('<b',data,12)[0],identifier=word(data,28),type=struct.unpack_from('<H',data,32)[0],named=bool(word(data,16)))
a=projection(descriptor);b=projection(bytes.fromhex(native['descriptor_hex']));assert a==b and a['kind']==11 and owner['bucket']==a['identifier']&63
assert symbol_name==native['name_hex']and symbol_name
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,original_va=hex(expected_va),formals=[dict(name='symbol',location='ECX',type='SymbolHeader pointer',original=arguments[0],native=native['pointer'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original69d10..69d7e consumes ECX descriptor only, saves/assigns other GPRs, no incoming EDX/stack/hidden/output pointer, RET0. UDT writer consumes the typeWORD+20 and name+10; full44-byte descriptor retained but unrelated allocation contents are not input equivalence.',descriptor_projection=a,original_descriptor_hex=descriptor.hex(),original_owner=owner,name_hex=symbol_name,native=native,original_stdout=stdout,original_state=state,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole descriptor pointer in global_scope(7c154) bucket(identifier&63), matching chain index/member on native. Defined identifier/kind/type/name and stdio/x87 compare. Raw44 bytes retained; unused value/offset/attribute/padding/allocation contents are not normalized or asserted zero. Closed global owner proof, no fabricated function-scope-head alias or child behavior acceptance.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual global descriptor/member/type/name match',a,owner)
