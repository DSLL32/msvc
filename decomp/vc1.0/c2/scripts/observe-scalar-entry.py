"""Explicit read-only fastcall scalar-entry observations, outside fixtures."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-only',action='store_true');parser.add_argument('--original-receipt',type=Path);parser.add_argument('--profile',default='dbglist');parser.add_argument('--function',required=True);parser.add_argument('--parameter',action='append',default=[]);parser.add_argument('--no-hidden-inputs-proof',required=True);parser.add_argument('--receipt',required=True,type=Path);parser.add_argument('--stack-cleanup',type=int,default=0);args=parser.parse_args()
parameters=[v.split(':')for v in args.parameter];assert 0<=len(parameters)<=8 and all(len(v)==2 and v[1]in ['signed32','unsigned32','signed16','unsigned16','signed8','unsigned8','null32','omf_record_slot32','omf_record_current32']for v in parameters)
assert args.stack_cleanup==max(len(parameters)-2,0)*4
started=time.perf_counter();receipt=args.receipt;original_receipt=args.original_receipt or receipt;assert args.original_receipt is None or args.native_only
if args.original_receipt is not None:assert receipt!=original_receipt and not(receipt/'native.txt').exists(), 'Preserve prior raw native evidence; choose a new receipt directory'
receipt.mkdir(exist_ok=args.native_only);config=json.loads(Path('functions.json').read_text());fixture=Path(config['cases'][args.profile]['fixture_manifest']).parent;phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2'];name=args.function
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
original_seconds=None
if args.native_only:
    binding=json.loads((original_receipt/'original-binding.json').read_text())
    for key,path in [('target_sha256',idx.wt.TC/'bin/C23232.EXE'),('manifest_sha256',fixture/'manifest.json'),('config_sha256','functions.json'),('observer_source_sha256','src/reference-stdio-observer.c'),('observer_sha256','build/reference-stdio-observer.exe')]:
        assert binding[key]==sha(path),(key,'immutable original binding changed')
    assert all(sha(original_receipt/file)==value for file,value in binding['files'].items())
else:
    with capture_lock():
        scratch=Path(tempfile.mkdtemp(prefix='scalar-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/phase['canonical_working_directory'].rsplit('\\',1)[1]
        for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
        for row in phase['inputs']:
            source=fixture/row['path'];assert sha(source)==row['sha256'];shutil.copyfile(source,work/source.name)
        original,rebuilt,config,records,unused=idx.indexes();row=next(r for r in config['driver']if r['name']==name)
        plan=receipt/'entry-plan.txt';plan.write_text('%x %02x %d %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],0x100|len(parameters),name));image=scratch/'C23232.EXE';image.write_bytes(original.data)
        env=idx.wt.environment()
        for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(key,None)
        env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name)
        command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+key))for key in ['stdout','stderr','events']],windows(work)]
        begin=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
        (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.stdout,result.stderr)
if not args.native_only:
    binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']})
    (receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='scalar-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="import gdb,json; s=gdb.parse_and_eval('recon_iob[1]'); used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0; values=[int(gdb.parse_and_eval(p))for p in PARAMETERS]; print('C2_SCALARS '+json.dumps(dict(values=values,omf_pool=dict(base=int(gdb.parse_and_eval('&omf_records[0]')),current=int(gdb.parse_and_eval('omf_current_record')),headers=[bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&omf_records[0]'))+1004*i,20)).hex()for i in range(4)]),coff_object=int(gdb.parse_and_eval('coff_object')),column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex()if used else ''))))"
snapshot=snapshot.replace('PARAMETERS',repr([p[0]for p in parameters]));command=['gdb','-q','--batch','-ex','set pagination off','-ex','break '+re.sub(r'@\d+$','',name[1:]),'-ex','run > native-inferior.stdout','-ex','python '+snapshot,str(Path('build/native/C23232.EXE').resolve())]
if name in ['_omf_flush_records','@omf_flush_record@4','@omf_write_record@4','@omf_write_index@8']:
 extra="import gdb,json; p=int(gdb.parse_and_eval('omf_current_record')); read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n)); import struct; end=struct.unpack('<I',read(p+16,4))[0]; assert 20<=end-p<=1004; print('C2_RECORD '+json.dumps(dict(pointer=p,current=p,base=int(gdb.parse_and_eval('&omf_records[0]')),bytes=end-p,hex=read(p,end-p).hex())))"
 command[-1:-1]=['-ex','python '+extra]
 if name=='@omf_write_record@4':
  obj="import gdb,json,os; s=gdb.parse_and_eval('*object_file'); fd=int(s['_file']); host=int(gdb.parse_and_eval('recon_osfhnd[%d]'%fd))-1; path='/proc/%d/fd/%d'%(gdb.selected_inferior().pid,host); assert os.readlink(path).endswith('/omftypes.obj'); info=open('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,host)).read(); pos=int(next(l.split()[1]for l in info.splitlines()if l.startswith('pos:'))); used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0; print('C2_OBJECT '+json.dumps(dict(stream=int(gdb.parse_and_eval('object_file')),flags=int(s['_flag']),count=int(s['_cnt']),fd=fd,size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,position=pos,osfile=int(gdb.parse_and_eval('recon_osfile[%d]'%fd)),file_bytes=os.stat(path).st_size,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex()if used else '')))"
  command[-1:-1]=['-ex','python '+obj]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log;native=json.loads(re.search('C2_SCALARS (.*)',log)[1]);lines=(original_receipt/'original.txt').read_text().splitlines();assert lines[0].split()[:2]==['enter',name]
values=[int(v,16)for v in lines[0].split()[5:]];assert len(values)==len(parameters);raw_values=values[:]
for index,(parameter,kind)in enumerate(parameters):
    bits=8 if kind.endswith('8')else 16 if kind.endswith('16')else 32;values[index]&=(1<<bits)-1
    if kind.startswith('signed')and values[index]>=(1<<(bits-1)):values[index]-=1<<bits
assert all(values[i]==native['values'][i]==0 for i,p in enumerate(parameters)if p[1]=='null32'), 'Null pointer observation requires both actual pointers zero'
native_raw_values=native['values'][:]
parts=lines[1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=lines[2].removeprefix('buffer_hex ')
parts=lines[3].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};assert lines[-1]=='entry_end'
omf_state=None;pool_input=None;record_input=None;object_input=None
if name=='_omf_open_module':
    assert len(lines)==6 and lines[4]=='omf_entry coff_object 0'and native['coff_object']==0;omf_state=dict(coff_object=0)
elif name in ['@omf_flush_record_type@4','@omf_find_record@8','@omf_allocate_record@8','@omf_initialize_record@12']:
    assert len(lines)==10
    parts=lines[4].split();pool={parts[n]:int(parts[n+1],16 if parts[n]in ['current','base']else 10)for n in range(1,len(parts),2)};pool['headers']=[line.removeprefix('omf_pool_header_hex ')for line in lines[5:9]]
    other=native['omf_pool'];assert pool['current']==pool['base']and other['current']==other['base']and pool['stride']==1004 and pool['count']==4
    assert pool['headers']==other['headers']==[bytes(20).hex()]*4
    pool_input=dict(original=pool,native=other,scope='Actual first helper has four BSS-zero20-byte headers and current record aliases slot0; no payload read/dump or arbitrary reuse-state claim.')
elif name in ['_omf_flush_records','@omf_flush_record@4','@omf_write_record@4','@omf_write_index@8']:
    assert len(lines)==(9 if name=='@omf_write_record@4'else 7)
    parts=lines[4].split();record={parts[n]:int(parts[n+1],10 if parts[n]=='bytes'else 16)for n in range(1,len(parts),2)};record['hex']=lines[5].removeprefix('omf_record_hex ');other=json.loads(re.search('C2_RECORD (.*)',log)[1])
    for r in [record,other]:
        expected_bytes=28 if name=='@omf_write_index@8'else 34
        expected_header=(153,0,0,5,r['pointer']+28)if name=='@omf_write_index@8'else(128,0,0,11,r['pointer']+34)
        expected_payload=b'\x99\x55\0\xa9'+bytes(4)if name=='@omf_write_index@8'else b'\x80\0\0\x0aomftypes.c'
        assert r['pointer']==r['current']==r['base']and r['bytes']==expected_bytes
        raw=bytes.fromhex(r['hex']);assert len(raw)==expected_bytes and struct.unpack('<5I',raw[:20])==expected_header and raw[20:]==expected_payload
    record_input=dict(original=record,native=other,scope='Actual current slot0 defined header/used payload only; cursor and owner aliases retained. No uninitialized tail dump or arbitrary record claim.')
    if name=='@omf_write_record@4':
        parts=lines[6].split();obj={parts[n]:int(parts[n+1],16 if parts[n]=='stream'else 10)for n in range(1,len(parts),2)};obj['buffer_hex']=lines[7].removeprefix('omf_object_buffer_hex ');other=json.loads(re.search('C2_OBJECT (.*)',log)[1]);assert {k:v for k,v in obj.items()if k!='stream'}=={k:v for k,v in other.items()if k!='stream'};assert obj['stream']and other['stream']and obj['flags']==2 and obj['count']==obj['used']==obj['position']==obj['file_bytes']==0 and obj['null']==1
        object_input=dict(original=obj,native=other,scope='Actual object_file owner, FILE flags/count/fd/buffer state, lowio flags and independent descriptor position/length; native path ends omftypes.obj. Read-only observer/proc inspection, no injected target calls.')
else:assert len(lines)==5
record_pointer_formals=[]
for i,(parameter,kind)in enumerate(parameters):
    if kind=='omf_record_current32':
        assert record_input is not None
        a=record_input['original'];b=record_input['native'];assert values[i]==a['pointer']and native['values'][i]==b['pointer'];record_pointer_formals.append(dict(name=parameter,slot=0,original_pointer=raw_values[i],native_pointer=native_raw_values[i],defined_pointee='actual%d-byte used record'%a['bytes']));values[i]=native['values'][i]=0
        continue
    if kind!='omf_record_slot32':continue
    assert pool_input is not None
    original_pool=pool_input['original'];native_pool=pool_input['native']
    left=values[i]-original_pool['base'];right=native['values'][i]-native_pool['base']
    assert 0<=left<4016 and left%1004==0 and 0<=right<4016 and right%1004==0
    assert left==right
    values[i]=left//1004;native['values'][i]=right//1004
    record_pointer_formals.append(dict(name=parameter,slot=values[i],original_pointer=raw_values[i],native_pointer=native_raw_values[i],defined_pointee='corresponding observed20-byte record header'))
assert values==native['values'],(values,native['values'])
assert native['stdout']=={k:v for k,v in stdout.items()if k not in ['counter','cursor','value','position']}
assert {k:native[k]for k in ['column','cw','sw']}=={k:state[k]for k in ['column','cw','sw']}
r=dict(original_reference=dict(directory=str(original_receipt),binding_sha256=sha(original_receipt/'original-binding.json'))if args.original_receipt is not None else None,matched=True,function=name,profile=args.profile,omf_state=omf_state,pool_input=pool_input,record_input=record_input,object_input=object_input,record_pointer_formals=record_pointer_formals,native_raw_values=native_raw_values,null_pointer_formals=[p[0]for p in parameters if p[1]=='null32'],formals=[dict(name=p,location=('ECX'if i==0 else'EDX'if i==1 else'entryESP+%d'%(4*(i-1))),type=t,value=values[i])for i,(p,t)in enumerate(parameters)],hidden_inputs=[],no_hidden_inputs_proof=args.no_hidden_inputs_proof,callee_stack_cleanup=args.stack_cleanup,original_raw_values=raw_values,original_values=values,original_state=state,native=native,original_stdout=stdout,target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),native_executable_sha256=sha('build/native/C23232.EXE'),original_execution_seconds=original_seconds,reused_immutable_original=args.native_only,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual scalar fastcall register/stack formals (8/16-bit parameters compare their proved byte/word contract; full original register/slot words retained) and known stdout/column/x87 entry state. No injected calls, return observation, child behavior, arbitrary-input equivalence or completed artifact claim.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual',name,'scalar formals match',values)
