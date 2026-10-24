"""Observe actual data callback input/caller-local ownership/current record."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
parser=argparse.ArgumentParser();parser.add_argument('--native-only',action='store_true');options=parser.parse_args();started=time.perf_counter();original_receipt=Path('cases/omf-data-abi');receipt=Path('cases/omf-data-native-abi')if options.native_only else original_receipt;receipt.mkdir();name='@omf_output_data@8';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
original_seconds=None
if options.native_only:
 binding=json.loads((original_receipt/'original-binding.json').read_text());assert all(sha(original_receipt/n)==h for n,h in binding['files'].items())and sha('functions.json')==binding['config_sha256']and sha('src/reference-stdio-observer.c')==binding['observer_source_sha256']and sha('build/reference-stdio-observer.exe')==binding['observer_sha256']
else:
 with capture_lock():
  scratch=Path(tempfile.mkdtemp(prefix='data-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
  for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
  for r in phase['inputs']:
   p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
  original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']=='_omf_output_data');assert row['start']==0x6b6c0;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 258 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
  for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
  env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
 binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='data-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval('$ecx'))&0xffffffff;n=int(gdb.parse_and_eval('$edx'))&0xffffffff;f=gdb.selected_frame();caller=f.older();assert caller.name()=='write_codeview_signature';caller.select();owner=int(gdb.parse_and_eval('&signature'));size=int(gdb.parse_and_eval('sizeof(signature)'));f.select();current=int(gdb.parse_and_eval('omf_current_record'));end=int(gdb.parse_and_eval('omf_current_record->cursor'));used=end-current;assert n==size==4 and 20<=used<=1004
print('C2_DATA '+json.dumps(dict(source=p,count=n,owner=owner,owner_size=size,caller=caller.name(),current=current,base=int(gdb.parse_and_eval('&omf_records[0]')),debug_owner=int(gdb.parse_and_eval('debug_symbols_segment')),used=used,source_hex=read(p,n).hex(),record_hex=read(current,used).hex())))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_output_data','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_DATA (.*)',r.stdout)[1]);lines=(original_receipt/'original.txt').read_text().splitlines();assert len(lines)==8 and lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==2
parts=lines[4].split();observed={parts[n]:int(parts[n+1],10 if parts[n]in ['count','used']else 16)for n in range(1,len(parts),2)};observed['source_hex']=lines[5].removeprefix('omf_data_hex ');observed['record_hex']=lines[6].removeprefix('omf_data_record_hex ')
assert args==[observed['source'],observed['count']]and observed['source']==observed['entry_sp']+4 and observed['return_address']==0x69cf6 and native['source']==native['owner']and native['owner_size']==4 and native['caller']=='write_codeview_signature'
assert observed['source_hex']==native['source_hex']=='01000000'and observed['count']==native['count']==4
shapes=[]
for d in [observed,native]:
 assert d['current']==d['base']and d['used']==28
 raw=bytes.fromhex(d['record_hex']);assert len(raw)==28
 typ,context,position,count,cursor=struct.unpack('<5I',raw[:20]);assert typ==0xa1 and context==d['debug_owner']and position==count==0 and cursor==d['current']+28
 shapes.append(raw[20:])
assert shapes[0]==shapes[1]
r=dict(original_reference=dict(directory=str(original_receipt),binding_sha256=sha(original_receipt/'original-binding.json')),matched=True,function=name,original_name_before_abi='_omf_output_data',original_va='0x6b6c0',formals=[dict(name='data',location='ECX',type='void pointer',original=observed['source'],native=native['source']),dict(name='bytes',location='EDX',type='unsigned32',value=4)],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original6b6c0..6b6e4 saves ECX data/EDX count before reading omf_current_record and calling reserve/append. RET0, no hidden stack/register/flags/x87 input. Actual4-byte signature source aliases original entryESP+4 with return69cf6 from caller69cd0; native frame write_codeview_signature &signature/sizeof4. All source bytes/current record fields and context ownership observed; destination uninitialized tail not read.',original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='First callback input is caller-local initialized unsigned signature1, current A1 slot0 with debug context/count0/offset0/cursor28. Raw GPRs observed before typed reconstruction.28 owned record bytes compare after context/cursor pointer normalization; no uninitialized tail snapshot/seed or child capacity behavior claim.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual data signature4-byte caller-local and current record inputs match')
