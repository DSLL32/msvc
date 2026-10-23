"""Explicit original-only listing-input observation and native public seam."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-only',action='store_true');parser.add_argument('--refresh-original',action='store_true');args=parser.parse_args();assert not(args.native_only and args.refresh_original)
started=time.perf_counter();receipt=Path('cases/dbglist-input-support');receipt.mkdir(exist_ok=args.native_only or args.refresh_original)
if args.refresh_original:
    archive=receipt/('original-attempt-%d'%(1+len(list(receipt.glob('original-attempt-*')))));archive.mkdir()
    for name in ['original.txt','original.stdout','original.stderr','original.events','entry-plan.txt','observer.stdout','observer.stderr','original-binding.json']:
        if(receipt/name).exists():shutil.copyfile(receipt/name,archive/name)
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
if not args.native_only:
    with capture_lock():
        scratch=Path(tempfile.mkdtemp(prefix='input-support-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
        for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
        for row in phase['inputs']:
            source=fixture/row['path'];assert sha(source)==row['sha256'];shutil.copyfile(source,work/source.name)
        original,rebuilt,config,records,unused=idx.indexes();runtime=json.loads(Path('runtime.json').read_text());plan=receipt/'entry-plan.txt'
        selected=[(next(r for r in runtime if r['name']==name),count)for name,count in [('_fgets',3),('_strchr',2)]]
        plan.write_text(''.join('%x %02x %d %s\n'%(r['start']-original.base,original.read(r['start'],1)[0],count,r['name'])for r,count in selected))
        image=scratch/'C23232.EXE';image.write_bytes(original.data)
        env=idx.wt.environment();env.pop('C2_PRINTF_FORMAT',None);env.pop('C2_STOP_ENTRY',None)
        env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_STREAM_INPUT='1',C2_STOP_RETURN='_strchr')
        command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
        begin=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-begin
        (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr)
        assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
    binding=dict(original_sha256=sha(receipt/'original.txt'),plan_sha256=sha(plan),manifest_sha256=sha(fixture/'manifest.json'),target_sha256=hashlib.sha256(original.data).hexdigest(),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),execution_seconds=original_seconds)
    (receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
else:
    binding=json.loads((receipt/'original-binding.json').read_text());plan=receipt/'entry-plan.txt'
    assert binding['original_sha256']==sha(receipt/'original.txt')and binding['plan_sha256']==sha(plan)and binding['manifest_sha256']==sha(fixture/'manifest.json')and binding['target_sha256']==sha(idx.wt.TC/'bin/C23232.EXE')
    original_seconds=binding['execution_seconds']
if(receipt/'native.txt').exists():
    attempt=1+len(list(receipt.glob('native-attempt-*.txt')))
    shutil.copyfile(receipt/'native.txt',receipt/('native-attempt-%d.txt'%attempt))
    if(receipt/'validation.json').exists():shutil.copyfile(receipt/'validation.json',receipt/('validation-attempt-%d.json'%attempt))
scratch=Path(tempfile.mkdtemp(prefix='input-support-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="""import gdb,json
s=gdb.parse_and_eval('(ShimFILE *)$c2stream').dereference()
base=int(s['_base']);used=int(s['_ptr'])-base if base else 0;valid=used+max(int(s['_cnt']),0);fd=int(s['_file']);host=int(gdb.parse_and_eval('recon_osfhnd[%d]'%fd))-1
info=open('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,host)).read();position=int(next(x.split()[1]for x in info.splitlines()if x.startswith('pos:')))
print('C2_INPUT_'+STAGE+' '+json.dumps(dict(stream=int(gdb.parse_and_eval('$c2stream')),cw=int(gdb.parse_and_eval('$fctrl'))&65535,sw=int(gdb.parse_and_eval('$fstat'))&65535,flags=int(s['_flag']),count=int(s['_cnt']),fd=fd,size=int(s['_bufsiz']),null=int(not int(s['_ptr'])and not base),used=used,valid=valid,position=position,osfile=int(gdb.parse_and_eval('recon_osfile[%d]'%fd)),file_buffer_hex=bytes(gdb.selected_inferior().read_memory(base,valid)).hex()if valid else '',destination_hex=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$c2dest')),256)).hex())))
"""
# Observe the actual caller's strchr call/return, avoiding the host libc IFUNC
# resolver (which has a different ABI and is not the public search function).
strchr_site="import gdb; start=int(gdb.parse_and_eval('&read_listing_source')); block=gdb.block_for_pc(start); instructions=gdb.selected_frame().architecture().disassemble(start,block.end); calls=[(i,r)for i,r in enumerate(instructions)if r['asm'].startswith('call')and '<strchr@plt>'in r['asm']]; assert len(calls)==2; i,row=calls[0]; gdb.Breakpoint('*%d'%row['addr']); gdb.execute('set $c2strret = %d'%instructions[i+1]['addr']); print('C2_STRCHR_CALLSITE',row,instructions[i+1])"
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break recon_fgets','-ex','run > native-inferior.stdout','-ex','set $c2dest = dest','-ex','set $c2stream = s','-ex','printf "C2_INPUT_ARGS %u %u %u\\n",dest,size,s']
for stage,actions in [('FGETS_BEFORE',[]),('FGETS_AFTER',['finish','printf "C2_FGETS_RESULT %u\\n",$eax','disable 1']),('STRCHR_BEFORE',['python '+strchr_site,'continue','printf "C2_STRCHR_ARGS %u %u\\n",*(unsigned *)$esp,*(unsigned *)($esp+4)']),('STRCHR_AFTER',['tbreak *$c2strret','continue','printf "C2_STRCHR_RESULT %u\\n",$eax'])]:
    for action in actions:command+=['-ex',action]
    command+=['-ex','python '+snapshot.replace('STAGE',repr(stage))]
command.append(str(Path('build/native/C23232.EXE').resolve()));begin=time.perf_counter()
result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-begin
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log
blocks=(receipt/'original.txt').read_text().splitlines();observations=[];i=0
while i<len(blocks):
    header=blocks[i].split();state=blocks[i+1].split();stdout={state[n]:int(state[n+1],16 if state[n]in ['cursor','value']else 10)for n in range(1,len(state),2)};stdout['buffer_hex']=blocks[i+2].removeprefix('buffer_hex ');i+=3
    raw=blocks[i].split();stream={raw[n]:int(raw[n+1])for n in range(1,len(raw),2)};stream['file_buffer_hex']=blocks[i+1].removeprefix('file_buffer_hex ');stream['destination_hex']=blocks[i+2].removeprefix('destination_hex ');assert blocks[i+3]=='input_end';i+=4
    observations.append(dict(stage=header[0],name=header[1],eax=int(header[3],16),args=[int(x,16)for x in header[5:]],stdout=stdout,input=stream))
assert [(r['name'],r['stage'])for r in observations]==[('_fgets','enter'),('_fgets','return'),('_strchr','enter'),('_strchr','return')]
native=[json.loads(re.search('C2_INPUT_'+stage+' (.*)',log)[1])for stage in ['FGETS_BEFORE','FGETS_AFTER','STRCHR_BEFORE','STRCHR_AFTER']]
native_args=list(map(int,re.search(r'C2_INPUT_ARGS (\d+) (\d+) (\d+)',log).groups()));strchr_args=list(map(int,re.search(r'C2_STRCHR_ARGS (\d+) (\d+)',log).groups()));fgets_result=int(re.search(r'C2_FGETS_RESULT (\d+)',log)[1]);strchr_result=int(re.search(r'C2_STRCHR_RESULT (\d+)',log)[1])
defined=bytes.fromhex(observations[1]['input']['destination_hex']).index(0)+1
fields=['cw','sw','flags','count','fd','size','null','used','valid','position','osfile','file_buffer_hex']
matched=all({k:r['input'][k]for k in fields}=={k:n[k]for k in fields}for r,n in zip(observations,native))
matched=matched and observations[0]['args'][1]==native_args[1]==256 and observations[1]['eax']==observations[0]['args'][0]and fgets_result==native_args[0]
matched=matched and all(r['input']['stream']==observations[0]['args'][2]for r in observations)and all(n['stream']==native_args[2]for n in native)
matched=matched and strchr_args[0]==native_args[0]and observations[2]['args'][0]==observations[0]['args'][0]and observations[2]['args'][1]==strchr_args[1]==10
offset=observations[3]['eax']-observations[2]['args'][0];matched=matched and strchr_result-strchr_args[0]==offset
original_dest=[bytes.fromhex(r['input']['destination_hex'])for r in observations];native_dest=[bytes.fromhex(r['destination_hex'])for r in native]
matched=matched and original_dest[1][:defined]==native_dest[1][:defined]and original_dest[0][defined:]==original_dest[1][defined:]and native_dest[0][defined:]==native_dest[1][defined:]
matched=matched and original_dest[1]==original_dest[2]==original_dest[3]and native_dest[1]==native_dest[2]==native_dest[3]
validation=dict(status='MATCHED'if matched else'PENDING public input mismatch',matched_public_seam=matched,original_observations=observations,native_observations=native,native_fgets_args=native_args,native_strchr_args=strchr_args,native_fgets_result=fgets_result,native_strchr_result=strchr_result,defined_destination_bytes=defined,strchr_result_offset=offset,source_sha256=sha(fixture/'source/dbglist.c'),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,original_reused=args.native_only,executable_sha256=sha('build/native/C23232.EXE'),scope='Actual first listing-source fgets256 then strchr LF. Full initialized FILE input buffer, descriptor state/position, public return pointer identities, defined destination prefix and each side own untouched tail; no native internal CRT entries or universal stream/locale/error claim. Full final three-artifact oracle remains mandatory.')
(receipt/'validation.json').write_text(json.dumps(validation,indent=2)+'\n');assert matched,validation
print('Actual fgets/strchr public input seam matches;',defined,'defined destination bytes; newline offset',offset)
