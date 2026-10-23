"""Explicit original-only numeric printf observation plus actual native seam."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-only',action='store_true');parser.add_argument('--refresh-original',action='store_true');args=parser.parse_args();assert not(args.native_only and args.refresh_original)
started=time.perf_counter();receipt=Path('cases/dbglist-numeric-support');receipt.mkdir(exist_ok=args.native_only or args.refresh_original)
previous=json.loads((receipt/'validation.json').read_text())if(receipt/'validation.json').exists()else None
if args.refresh_original:
    archive=receipt/('original-attempt-%d'%(1+len(list(receipt.glob('original-attempt-*')))));archive.mkdir()
    for name in ['original.txt','original.stdout','original.stderr','original.events','entry-plan.txt','observer.stdout','observer.stderr','original-binding.json']:
        if(receipt/name).exists():shutil.copyfile(receipt/name,archive/name)
if previous:
    attempt=1+len(list(receipt.glob('validation-attempt-*.json')))
    shutil.copyfile(receipt/'validation.json',receipt/('validation-attempt-%d.json'%attempt))
    if(receipt/'native.txt').exists():shutil.copyfile(receipt/'native.txt',receipt/('native-attempt-%d.txt'%attempt))
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
contracts={'_printf':3,'__cfltcvt':5,'__cftog':4,'__cftoe_g':4,'__cftof_g':3,'__cftoe':4,'__cftof':3,'__fltout':2,'___dtold':2,'_$I10_OUTPUT':6,'___add_12':2,'___addl':3,'___ld12mul':2,'___multtenpow12':3,'___shl_12':1,'___shr_12':1,'__fptostr':3,'__shift':2,'_memcpy':3,'__cropzeros':1,'__positive':1,'__forcdecpt':1}
if not args.native_only:
    with capture_lock():
        scratch=Path(tempfile.mkdtemp(prefix='numeric-support-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
        for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
        for row in phase['inputs']:
            data=(fixture/row['path']).read_bytes();assert hashlib.sha256(data).hexdigest()==row['sha256'];(work/Path(row['path']).name).write_bytes(data)
        original,rebuilt,config,records,unused=idx.indexes();runtime=json.loads(Path('runtime.json').read_text());plan=receipt/'entry-plan.txt'
        selected=[next(r for r in runtime if r['name']==name)for name in contracts]
        plan.write_text(''.join('%x %02x %d %s\n'%(r['start']-original.base,original.read(r['start'],1)[0],contracts[r['name']],r['name'])for r in selected))
        image=scratch/'C23232.EXE';image.write_bytes(original.data)
        env=idx.wt.environment();env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_PRINTF_FORMAT='; %g')
        command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
        phase_started=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-phase_started
        (receipt/'observer.stdout').write_bytes(result.stdout);(receipt/'observer.stderr').write_bytes(result.stderr)
        assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
    (receipt/'original-binding.json').write_text(json.dumps(dict(original_sha256=hashlib.sha256((receipt/'original.txt').read_bytes()).hexdigest(),plan_sha256=hashlib.sha256(plan.read_bytes()).hexdigest(),manifest_sha256=hashlib.sha256((fixture/'manifest.json').read_bytes()).hexdigest(),target_sha256=hashlib.sha256(original.data).hexdigest(),observer_source_sha256=hashlib.sha256(Path('src/reference-stdio-observer.c').read_bytes()).hexdigest(),observer_sha256=hashlib.sha256(Path('build/reference-stdio-observer.exe').read_bytes()).hexdigest(),execution_seconds=original_seconds),indent=2)+'\n')
else:
    bound=json.loads((receipt/'original-binding.json').read_text());plan=receipt/'entry-plan.txt';assert bound['target_sha256']==hashlib.sha256((idx.wt.TC/'bin/C23232.EXE').read_bytes()).hexdigest();assert bound['original_sha256']==hashlib.sha256((receipt/'original.txt').read_bytes()).hexdigest()and bound['plan_sha256']==hashlib.sha256(plan.read_bytes()).hexdigest()and bound['manifest_sha256']==hashlib.sha256((fixture/'manifest.json').read_bytes()).hexdigest();original_seconds=bound['execution_seconds']
scratch=Path(tempfile.mkdtemp(prefix='numeric-support-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
snapshot="import gdb,json; s=gdb.parse_and_eval('recon_iob[1]'); used=int(s['_ptr'])-int(s['_base']) if int(s['_base']) else 0; fd=int(gdb.parse_and_eval('recon_osfhnd[1]'))-1; info=open('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,fd)).read(); position=int(next(x.split()[1]for x in info.splitlines()if x.startswith('pos:'))); print(PREFIX+json.dumps(dict(flags=int(s['_flag']),count=int(s['_cnt']),fd=int(s['_file']),size=int(s['_bufsiz']),null=int(not int(s['_ptr']) and not int(s['_base'])),used=used,position=position,buffer_hex=bytes(gdb.selected_inferior().read_memory(int(s['_base']),used)).hex() if used else '')));print(FLOAT+json.dumps(dict(cw=int(gdb.parse_and_eval('$fctrl')),sw=int(gdb.parse_and_eval('$fstat')))))"
args="import gdb,json; a=int(gdb.parse_and_eval('&format')); print('C2_NUMERIC_ARGS '+json.dumps(dict(format_hex=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('format')),5)).hex(),double_hex=bytes(gdb.selected_inferior().read_memory(a+4,8)).hex())))"
condition="break recon_printf if format[0]==59 && format[1]==32 && format[2]==37 && format[3]==103 && format[4]==0"
command=['gdb','-q','--batch','-ex','set pagination off','-ex',condition,'-ex','run > native-inferior.stdout','-ex','python '+args,'-ex','python '+snapshot.replace('PREFIX',repr('C2_NUMERIC_BEFORE ')).replace('FLOAT',repr('C2_FLOAT_BEFORE ')),'-ex','finish','-ex','printf "C2_NUMERIC_RESULT %d\\n",$eax','-ex','python '+snapshot.replace('PREFIX',repr('C2_NUMERIC_AFTER ')).replace('FLOAT',repr('C2_FLOAT_AFTER ')),str(Path('build/native/C23232.EXE').resolve())]
phase_started=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-phase_started
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log
blocks=(receipt/'original.txt').read_text().splitlines();observations=[];i=0
while i<len(blocks):
    header=blocks[i].split();state=blocks[i+1].split();fields={state[n]:int(state[n+1],16 if state[n]in ['cursor','value']else 10)for n in range(1,len(state),2)};fields['buffer_hex']=blocks[i+2].removeprefix('buffer_hex ');i+=3
    numeric=blocks[i].split();registers={numeric[n]:int(numeric[n+1],16)for n in range(1,len(numeric),2)};i+=1;extra={}
    while blocks[i]!='numeric_end':key,_,value=blocks[i].partition(' ');extra[key]=value;i+=1
    i+=1;observations.append(dict(stage=header[0],name=header[1],eax=int(header[3],16),args=[int(x,16)for x in header[5:]],file=fields,registers=registers,extra=extra))
public=[r for r in observations if r['name']=='_printf'];assert [r['stage']for r in public]==['enter','return']
left=[{k:v for k,v in r['file'].items()if k not in ['counter','cursor','value']}for r in public]
right=[json.loads(re.search('C2_NUMERIC_'+stage+' (.*)',log)[1])for stage in ['BEFORE','AFTER']]
floating=[json.loads(re.search('C2_FLOAT_'+stage+' (.*)',log)[1])for stage in ['BEFORE','AFTER']]
actual_args=json.loads(re.search('C2_NUMERIC_ARGS (.*)',log)[1]);expected_bits=struct.pack('<d',1.25).hex()
assert actual_args==dict(format_hex=b'; %g\0'.hex(),double_hex=expected_bits)
assert public[0]['extra']['format_hex']==actual_args['format_hex']and struct.pack('<II',*public[0]['args'][1:]).hex()==expected_bits
matched=left==right
original_floating=[{key:r['registers'][key]&0xffff for key in ['cw','sw']}for r in public]
validation=dict(observations=observations,public_original_file=left,public_native_file=right,public_original_floating=original_floating,public_native_floating=floating,public_original_result=public[-1]['eax'],public_native_result=int(re.search(r'C2_NUMERIC_RESULT (-?\d+)',log)[1]),public_args=actual_args,matched_public_text_and_file=matched,matched_public_seam=matched and original_floating==floating and public[-1]['eax']==int(re.search(r'C2_NUMERIC_RESULT (-?\d+)',log)[1])==6,status='MATCHED' if matched and original_floating==floating else 'PENDING public seam mismatch',
 boundary='Internal CRT native ABIs are not matched or fabricated. Actual public printf format/double/results/FILE/defined buffers/descriptor positions and floating state are observed. Full listing/object/PDB oracle remains mandatory.',original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,plan_sha256=hashlib.sha256(plan.read_bytes()).hexdigest(),executable_sha256=hashlib.sha256(Path('build/native/C23232.EXE').read_bytes()).hexdigest())
(receipt/'validation.json').write_text(json.dumps(validation,indent=2)+'\n')
assert matched,(left,right)
assert validation['public_original_result']==validation['public_native_result']==6
assert bytes.fromhex(right[-1]['buffer_hex'])==bytes.fromhex(right[0]['buffer_hex'])+b'; 1.25'
assert original_floating==floating,(original_floating,floating)
print('numeric public seam matches; observed helpers',sorted({r['name']for r in observations}),'floating',floating)
