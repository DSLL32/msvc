"""Actual initializer-chain input and free-list identity, outside fixtures."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--function',default='@release_initializer@4',choices=['@release_initializer@4']);parser.add_argument('--receipt',type=Path,default=Path('cases/dbglist-release-initializer-abi'));args=parser.parse_args()
started=time.perf_counter();receipt=args.receipt;receipt.mkdir();fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function;pointer_slot=1 if name=='_coff_output_size'else 0
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='function-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for r in phase['inputs']:
        source=fixture/r['path'];assert sha(source)==r['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name);expected_va={'@release_initializer@4':0x4e8e0}[name];assert row['start']==expected_va
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
snapshot="""import gdb,json,struct
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
head=int(gdb.parse_and_eval('initializer'));free_head=int(gdb.parse_and_eval('free_initializers'));node=head;nodes=[]
while node and len(nodes)<256:
 raw=read(node,28);words=struct.unpack('<7I',raw);nodes.append(dict(address=node,next=words[1],link=words[2],hex=raw.hex()));node=words[1]
assert not node
s=gdb.parse_and_eval('recon_iob[1]');used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_INITIALIZERS '+json.dumps(dict(head=head,free_head=free_head,nodes=nodes,free_hex=read(free_head,28).hex()if free_head else'',column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=read(int(s['_base']),used).hex()if used else ''))))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break release_initializer','-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin;(receipt/'native.txt').write_bytes(result.stdout);assert result.returncode==0,result.stdout
native=json.loads(re.search(rb'C2_INITIALIZERS (.*)',result.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name];arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==1
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)}
parts=lines[4].split();original_list={parts[n]:int(parts[n+1],16)for n in range(1,len(parts),2)};original_list['nodes']=[];index=5
while lines[index].startswith('initializer_node '):
 parts=lines[index].split();node={parts[n]:int(parts[n+1],16)for n in range(1,len(parts),2)};node['hex']=lines[index+1].removeprefix('initializer_hex ');original_list['nodes'].append(node);index+=2
original_list['free_hex']=''
if lines[index].startswith('free_initializer_hex '):original_list['free_hex']=lines[index].removeprefix('free_initializer_hex ');index+=1
assert lines[index]=='entry_end'
def shape(value):
 nodes=value['nodes'];addresses=[n['address']for n in nodes];assert len(addresses)==len(set(addresses))and len(nodes)<=256
 assert value['head']==(addresses[0]if addresses else 0)
 for i,node in enumerate(nodes):
  words=struct.unpack('<7I',bytes.fromhex(node['hex']));assert words[1]==node['next']and words[2]==node['link'];assert node['next']==(addresses[i+1]if i+1<len(nodes)else 0)
 return dict(kinds=[struct.unpack('<i',bytes.fromhex(n['hex'])[:4])[0]for n in nodes],free_nonnull=bool(value['free_head']),free_alias_index=addresses.index(value['free_head'])if value['free_head']in addresses else None)
assert arguments[0]==original_list['head']and native['head']and arguments[0]
a=shape(original_list);b=shape(native);assert a==b,(a,b)
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,original_va=hex(expected_va),formals=[dict(name='initializer',location='ECX',type='Initializer pointer',original=arguments[0],native=native['head'])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original4e8e0..4e8fd consumes ECX next-chain and free-list head7c144; EAX/EDX assigned before use, no incoming flags/stack/x87/other GPR inputs, RET0. Only pointee next links are read; free-head identity is assigned to overwritten+8 link, not dereferenced.',original_list=original_list,native=native,shape=a,original_stdout=stdout,original_state=state,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Read-only actual ECX chain with raw28-byte nodes and free-head identity; bounded host observer rejects cyclic/over256 chains. Compare defined next topology/kinds and free-head null/chain alias relation; raw payload pointers/other unused data words not equated. The bound is observer-only, never a reconstructed-function guard. No injected target calls, transformation/return behavior or artifact completion claimed.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual initializer chain/free-head inputs match',a)
