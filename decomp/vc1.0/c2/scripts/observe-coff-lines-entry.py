"""Actual COFF line-ring inputs and auxiliary output owners, outside fixtures."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--function',default='@coff_write_lines@8',choices=['@coff_write_lines@8']);parser.add_argument('--receipt',type=Path,default=Path('cases/dbglist-coff-write-lines-abi'));args=parser.parse_args()
started=time.perf_counter();receipt=args.receipt;receipt.mkdir();fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function;pointer_slot=1 if name=='_coff_output_size'else 0
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='function-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for r in phase['inputs']:
        source=fixture/r['path'];assert sha(source)==r['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name);expected_va={'@coff_write_lines@8':0x6f040}[name];assert row['start']==expected_va
    plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],258,name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
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
word=lambda p:struct.unpack('<I',read(p,4))[0]
head=int(gdb.parse_and_eval('contribution'));raw=read(head,56);last=word(head+20);block=word(last+4);blocks=[];aux=[];debug=int(gdb.parse_and_eval('coff_debug'));vector=int(gdb.parse_and_eval('coff_symbol_entries'));section=word(head+4)
while True:
 count=word(block);assert count<=50 and len(blocks)<256
 data=read(block+8,count*6);blocks.append(dict(address=block,count=count,next=word(block+4),hex=data.hex()))
 for i in range(count):
  value,line=struct.unpack_from('<IH',data,i*6)
  if debug&1 and not line:
   index=value+2;slot=index-1;capacity=struct.unpack('<H',read(vector+4,2))[0];chunk=word(vector+24);assert capacity and 0<index<=word(vector)
   while slot>=capacity:slot-=capacity;chunk=word(chunk);assert chunk
   entry=word(chunk+4+slot*4);pointer=word(entry+4);aux.append(dict(index=index,entry=entry,auxiliary=pointer,hex=read(pointer,18).hex()))
 if block==last:break
 block=word(block+4)
state=dict(last=last,free_head=int(gdb.parse_and_eval('coff_free_lines')),debug=debug,vector=vector,section=section,owner=word(section),file=int(gdb.parse_and_eval('coff_file')))
s=gdb.parse_and_eval('recon_iob[1]');used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0
print('C2_COFF_LINES '+json.dumps(dict(pointer=head,base=int(gdb.parse_and_eval('base')),contribution_hex=raw.hex(),state=state,blocks=blocks,aux=aux,column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=read(int(s['_base']),used).hex()if used else ''))))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break coff_write_lines','-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin;(receipt/'native.txt').write_bytes(result.stdout);assert result.returncode==0,result.stdout
native=json.loads(re.search(rb'C2_COFF_LINES (.*)',result.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name];arguments=[int(v,16)for v in lines[0].split()[5:]];assert len(arguments)==2
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)}
parts=lines[4].split();original_state={parts[n]:int(parts[n+1],10 if parts[n]=='debug'else 16)for n in range(1,len(parts),2)};original=dict(pointer=arguments[0],base=arguments[1],state=original_state,contribution_hex=lines[5].removeprefix('contribution_hex '),blocks=[],aux=[]);index=6
while lines[index]!='entry_end':
 parts=lines[index].split();isblock=parts[0]=='coff_lines_block';r={parts[n]:int(parts[n+1],10 if parts[n]in ['count','index']else 16)for n in range(1,len(parts),2)};r['hex']=lines[index+1].split(' ',1)[1];original['blocks'if isblock else'aux'].append(r);index+=2
def shape(value):
 state=value['state'];raw=bytes.fromhex(value['contribution_hex']);words=struct.unpack_from('<12I',raw);blocks=value['blocks'];addresses=[b['address']for b in blocks];assert words[1]==state['section']and state['owner']==value['pointer']and words[5]==state['last']==addresses[-1]
 assert len(addresses)==len(set(addresses))and blocks[-1]['next']==addresses[0]
 for i,b in enumerate(blocks[:-1]):assert b['next']==addresses[i+1]
 for b in blocks:assert len(bytes.fromhex(b['hex']))==6*b['count']
 return dict(base=value['base'],debug=state['debug'],lines_offset=words[11],free_nonnull=bool(state['free_head']),line_count=struct.unpack_from('<H',raw,54)[0],blocks=[dict(count=b['count'],hex=b['hex'])for b in blocks],aux=[dict(index=a['index'],hex=a['hex'])for a in value['aux']])
a=shape(original);b=shape(native);assert a==b,(a,b)
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(matched=True,function=name,original_va=hex(expected_va),formals=[dict(name='contribution',location='ECX',type='CoffContribution pointer',original=arguments[0],native=native['pointer']),dict(name='base',location='EDX',type='unsigned32',value=arguments[1])],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6f040..6f108 consumes ECX contribution and EDX base; all saved registers assigned, no incoming flags/stack/x87/other GPR input, RET0. Contribution owns circular line blocks at+20 and initial line file offset+44; typed globals supply debugbit0/vector/file/free-list/callback. Actual auxiliary pointers reached through symbol-vector indexed handle+2 are observed as mutable output objects, not hidden ABI parameters.',original=original,native=native,shape=a,original_stdout=stdout,original_state=state,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual two formals, section.last contribution identity, circular defined line records, lines offset, debug bit and indexed18-byte auxiliary output owners. Raw56-byte contribution/pointers retained; unused scalar/padding words not compared. Observer-only max256 blocks/count50 bounds, no target guards. No injected calls, real writer/child/error behavior or completed-artifact acceptance.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual COFF line ring and auxiliary inputs match',dict(base=a['base'],counts=[b['count']for b in a['blocks']],aux=[r['index']for r in a['aux']]))
