"""Explicit original/native actual ABI observations before listing promotion."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/dbglist-bytes-abi');receipt.mkdir()
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='listing-abi-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for row in phase['inputs']:
        source=fixture/row['path'];assert sha(source)==row['sha256'];shutil.copyfile(source,work/source.name)
    original,rebuilt,config,records,unused=idx.indexes();names=[('@write_listing_bytes@12',3),('@write_listing_spaces@4',1)];plan=receipt/'entry-plan.txt'
    selected=[(next(r for r in config['driver']if r['name']==name),count)for name,count in names]
    plan.write_text(''.join('%x %02x %d %s\n'%(r['start']-original.base,original.read(r['start'],1)[0],count|0x100,r['name'])for r,count in selected));image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment()
    for name in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(name,None)
    env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY='@write_listing_spaces@4')
    command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
    begin=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
    (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
scratch=Path(tempfile.mkdtemp(prefix='listing-abi-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="import gdb,json; s=gdb.parse_and_eval('recon_iob[1]'); used=int(s['_ptr'])-int(s['_base'])if int(s['_base'])else 0; print(TAG+json.dumps(dict(column=int(gdb.parse_and_eval('listing_column')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,stdout=dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not int(s['_base'])),used=used,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex()if used else ''))))"
arguments="import gdb,json; count=int(gdb.parse_and_eval('count')); assert 0<=count<=20; p=int(gdb.parse_and_eval('bytes')); print('C2_BYTES_ARGS '+json.dumps(dict(offset=int(gdb.parse_and_eval('offset')),count=count,pointer=p,bytes_hex=bytes(gdb.selected_inferior().read_memory(p,count)).hex())))"
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break write_listing_bytes','-ex','break write_listing_spaces','-ex','run > native-inferior.stdout','-ex','python '+arguments,'-ex','python '+snapshot.replace('TAG',repr('C2_BYTES_STATE ')),'-ex','continue','-ex','printf "C2_SPACES_COUNT %d\\n",count','-ex','python '+snapshot.replace('TAG',repr('C2_SPACES_STATE ')),str(Path('build/native/C23232.EXE').resolve())]
begin=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log
raw=(receipt/'original.txt').read_text().splitlines();rows=[];i=0
while i<len(raw):
    header=raw[i].split();parts=raw[i+1].split();stdout={parts[n]:int(parts[n+1],16 if parts[n]in['cursor','value']else 10)for n in range(1,len(parts),2)};stdout['buffer_hex']=raw[i+2].removeprefix('buffer_hex ');i+=3
    parts=raw[i].split();state={parts[n]:int(parts[n+1])for n in range(1,len(parts),2)};i+=1;extra={}
    while raw[i]!='entry_end':key,_,value=raw[i].partition(' ');extra[key]=value;i+=1
    i+=1;rows.append(dict(stage=header[0],name=header[1],args=[int(v,16)for v in header[5:]],state=state,stdout=stdout,extra=extra))
assert [(r['stage'],r['name'])for r in rows]==[('enter',n)for n,c in names]
native_args=json.loads(re.search('C2_BYTES_ARGS (.*)',log)[1]);native_bytes=json.loads(re.search('C2_BYTES_STATE (.*)',log)[1]);native_spaces=json.loads(re.search('C2_SPACES_STATE (.*)',log)[1]);count=int(re.search(r'C2_SPACES_COUNT (-?\d+)',log)[1])
assert rows[0]['args'][:2]==[native_args['offset'],native_args['count']]==[0,1]and len(rows[0]['args'])==3 and rows[0]['args'][2]and native_args['pointer']
assert rows[0]['extra']['bytes_hex']==native_args['bytes_hex']=='55'and rows[1]['args']==[count]==[1]
for r,n,column in zip(rows,[native_bytes,native_spaces],[0,7]):
    assert r['state']==dict(column=column,cw=0x27f,sw=0,fastcall=1)
    assert {k:n[k]for k in ['column','cw','sw']}==dict(column=column,cw=0x27f,sw=0)
    assert n['stdout']=={k:v for k,v in r['stdout'].items()if k not in ['counter','cursor','value','position']}
validation=dict(matched=True,original=rows,native_bytes_arguments=native_args,native_bytes_state=native_bytes,native_spaces_count=count,native_spaces_state=native_spaces,formals=[dict(function=names[0][0],parameters=[dict(name='offset',location='ECX',type='unsigned32',value=0),dict(name='count',location='EDX',type='signed32',value=1),dict(name='bytes',location='entryESP+4',type='input byte pointer',original=rows[0]['args'][2],native=native_args['pointer'],defined_input_bytes='55')],callee_stack_cleanup=4,hidden_inputs=[],proof='Original29a60..29b2b consumes ECX/EDX and sole stack pointer, with RET4; entry callee-saved registers only saved before assignment.'),dict(function=names[1][0],parameters=[dict(name='count',location='ECX',type='signed32',value=1)],callee_stack_cleanup=0,hidden_inputs=[],proof='Original29990..29a55 consumes ECX count and listing_column; other registers saved or assigned before use; RET0.')],known_implicit_state='listing_column0→7 and full defined stdout FILE buffer match; CW027f/SW0 observed',target_sha256=hashlib.sha256(original.data).hexdigest(),config_sha256=sha('functions.json'),manifest_sha256=sha(fixture/'manifest.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual case function-entry ABI inputs, no injected target calls. All known formals/defined input bytes observed, no hidden or output pointer arguments in these two original bodies. This supplements masked-original/native frontier matching; not branch coverage or completion.')
validation['native_executable_sha256']=sha('build/native/C23232.EXE')
(receipt/'validation.json').write_text(json.dumps(validation,indent=2)+'\n');print('Actual bytes(offset0,count1,pointer→55) and spaces(count1) ABI inputs/state match')
