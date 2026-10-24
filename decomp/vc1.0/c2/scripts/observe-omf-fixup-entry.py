"""Observe fixup four-formal ABI, actual descriptor/record and named thread state."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-fixup-abi');receipt.mkdir();name='@omf_add_fixup@16';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='fixup-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6b990;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 260 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='fixup-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
symbol=gdb.parse_and_eval('symbol');pointer=int(symbol);typ=int(gdb.parse_and_eval('type'));displacement=int(gdb.parse_and_eval('displacement'));relative=int(gdb.parse_and_eval('relative'));f=gdb.selected_frame();caller=f.older();assert caller.name()=='omf_output_bytes';caller.select();owner=int(gdb.parse_and_eval('symbol'));assert displacement==int(gdb.parse_and_eval('displacement'));f.select();identifier=int(symbol['identifier']);bucket=identifier&63;member=gdb.parse_and_eval('global_scope')['buckets'][bucket];index=0
while int(member)and int(member['identifier'])!=identifier and index<1024:member=member['hash_next'];index+=1
assert int(member)and index<1024
current=gdb.parse_and_eval('omf_current_record');state=dict(symbol=pointer,type=typ,displacement=displacement,relative=relative,owner_symbol=owner,identifier=identifier,bucket=bucket,index=index,member=int(member),current=int(current),base=int(gdb.parse_and_eval('&omf_records[0]')),count=int(current['count']),dos=int(gdb.parse_and_eval('dos_object')),caller=caller.name());threads={n:int(gdb.parse_and_eval('omf_fixup_threads.'+n))for n in ['group_frame','bss_target','data_target','const_target','code_target']};kind=int(symbol['kind']);payload={};segment=None
if kind==1:
 flags=int(symbol['field0e']);attributes=int(symbol.cast(gdb.lookup_type('BackendSymbol').pointer())['attributes']);storage=2 if flags&32 else attributes>>7&15;payload=dict(kind=kind,offset=int(symbol['field04']),flags=flags,attributes=attributes,storage=storage)
 if storage in[1,4]:
  target_id=int(symbol['field18']['identifier']);tbucket=target_id&63;tmember=gdb.parse_and_eval('global_scope')['buckets'][tbucket];ti=0
  while int(tmember)and int(tmember['identifier'])!=target_id and ti<1024:tmember=tmember['hash_next'];ti+=1
  assert int(tmember)and ti<1024;segment=dict(identifier=target_id,pointer=int(tmember),order=int(tmember.cast(gdb.lookup_type('Segment').pointer())['identity']['order']),bucket=tbucket,index=ti)
 elif storage in[2,3]:payload['external_index']=int(symbol['field14'])
elif kind==4:payload=dict(kind=kind,index=int(symbol['field14']),attributes=int(symbol.cast(gdb.lookup_type('BackendSymbol').pointer())['attributes']))
else:raise AssertionError('Unobserved descriptor kind')
print('C2_FIXUP '+json.dumps(dict(state=state,threads=threads,payload=payload,segment=segment)))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_add_fixup','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_FIXUP (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==4
rows={v.split(' ',1)[0]:v.split(' ',1)[1]for v in lines[4:-1]}
def parse(text,hexkeys=()):
 parts=text.split();return {parts[n]:int(parts[n+1],16 if parts[n]in hexkeys else 10)for n in range(0,len(parts),2)}
observed=dict(state=parse(rows['omf_fixup'],['symbol','displacement','return_address','owner_symbol','member','current','base']),threads=parse(rows['omf_fixup_threads']),payload=parse(rows['omf_fixup_symbol']),segment=parse(rows['omf_fixup_segment'],['pointer'])if 'omf_fixup_segment'in rows else None)
if 'omf_fixup_external_index'in rows:observed['payload']['external_index']=int(rows['omf_fixup_external_index'])
assert args[0]==observed['state']['symbol']and args[1]&255==observed['state']['type']and args[2:]==[observed['state'][k]for k in['displacement','relative']]and observed['state']['return_address']==0x6b2cf
for d in[observed,native]:
 st=d['state'];assert st['symbol']==st['owner_symbol']==st['member']and st['symbol']and st['identifier']&63==st['bucket'];slot,rem=divmod(st['current']-st['base'],1004);assert not rem and 0<=slot<4;st['slot']=slot
 if d['segment']:
  sg=d['segment'];assert sg['pointer']and sg['bucket']==sg['identifier']&63

def shape(d):
 st=d['state'];p=dict(formals={k:st[k]for k in['type','displacement','relative']},symbol_owner={k:st[k]for k in['identifier','bucket','index']},record={k:st[k]for k in['slot','count']},dos=st['dos'],threads=d['threads'],payload=d['payload'],segment={k:v for k,v in d['segment'].items()if k!='pointer'}if d['segment']else None);return p
assert shape(observed)==shape(native)
r=dict(matched=True,function=name,original_va='0x6b990',formals=[dict(name=k,location=l,type=t,original=observed['state'][k],native=native['state'][k])for k,l,t in [('symbol','ECX','SymbolHeader pointer'),('type','DL','unsigned8'),('displacement','entryESP+4','unsigned32'),('relative','entryESP+8','signed32')]],hidden_inputs=[],callee_stack_cleanup=8,no_hidden_inputs_proof='Original6b990..6bcd8 saves ECX symbol/DLtype, consumes displacement stack+4/relative stack+8, RET8. No other incoming register/flags/x87/out pointer. Actual original EBP/globalhash symbol identity and wrapper return6b2cf, native omf_output_bytes formal aliases match. Defined consumed descriptor fields/current record count/named five fixup threads/dos option and segment lookup identity when used observed. Allocator-returned entry is a future child result, not invented incoming pointer/data; no old output fixup bytes captured or seeded.',raw_original_arguments=args,shape=shape(observed),original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual four formals/active descriptor fields and contextual globals only. Child can change later state, no claim entry snapshot replaces post-child reload. Label kind3 not reached/observed, no current_symbol pointee fabricated. Whole body/cold branches/allocator not accepted by this observation.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual fixup four formals and contextual inputs match',r['shape'])
