"""Observe all relocation callback formals/current caller aliases and active union inputs."""
from pathlib import Path
import hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/omf-relocation-abi');receipt.mkdir();name='@omf_output_relocation@12';fixture=Path('../fixtures/c2-omf-typed-debug');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='relocation-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6afc0;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 259 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='relocation-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json,struct
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
p=int(gdb.parse_and_eval('initializer'));symbol=int(gdb.parse_and_eval('symbol'));segment=int(gdb.parse_and_eval('segment'));f=gdb.selected_frame();caller=f.older();assert caller.name()=='emit_initializer';caller.select();loop_owner=int(gdb.parse_and_eval('initializer'));symbol_owner=int(gdb.parse_and_eval('symbol'));segment_owner=int(gdb.parse_and_eval('segment'));f.select();node=gdb.parse_and_eval('initializer');kind=int(node['kind']);payload={}
if kind==1:
 dtype=int(node['content']['datum']['type']);width=int(node['content']['datum']['bytes']);assert 0<=width<=4096 and (dtype==6 or width<=12);source=int(node['content']['datum']['value']['bytes'])if dtype==6 else p+12;payload=dict(datum_hex=read(p+8,4).hex(),repeat_hex=read(p+24,4).hex(),data_pointer=source,data_hex=read(source,width).hex())
elif kind==2:
 raw=read(p+8,9);rid=struct.unpack('<I',raw[:4])[0];bucket=rid&63;member=gdb.parse_and_eval('global_scope')['buckets'][bucket];index=0
 while int(member)and int(member['identifier'])!=rid and index<1024:member=member['hash_next'];index+=1
 assert int(member)and index<1024
 rkind=int(member['kind']);label=int(gdb.Value(int(member)).cast(gdb.lookup_type('LabelSymbol').pointer())['label_kind'])if rkind==3 else -1;payload=dict(relocation_hex=raw.hex(),reference=dict(pointer=int(member),identifier=rid,kind=rkind,label_kind=label,bucket=bucket,index=index,member=int(member)),reference_name_hex=(member['name'].string(encoding='latin1').encode('latin1')+bytes([0])).hex())
elif kind==4:payload=dict(alignment_hex=read(p+8,1).hex())
elif kind==8:payload=dict(padding_hex=read(p+8,4).hex())
aliases=[name for name,var in [('code','code_segment'),('data','data_segment'),('constant','const_segment'),('bss','bss_segment'),('debug_symbols','debug_symbols_segment'),('debug_types','debug_types_segment')]if int(gdb.parse_and_eval(var))==segment]
print('C2_RELOCATION '+json.dumps(dict(initializer=p,symbol=symbol,segment=segment,loop_owner=loop_owner,symbol_owner=symbol_owner,segment_owner=segment_owner,combine=int(gdb.parse_and_eval('segment->combine')),position=int(gdb.parse_and_eval('segment->symbol.field04')),kind_hex=read(p,4).hex(),payload=payload,segment_aliases=aliases,caller=caller.name(),initializer_size=int(gdb.parse_and_eval('sizeof(*initializer)')))))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break omf_output_relocation','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_RELOCATION (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name];args=[int(v,16)for v in lines[0].split()[5:]];assert len(args)==3
rawrows={line.split(' ',1)[0]:line.split(' ',1)[1]for line in lines[4:-1]}
parts=rawrows['omf_relocation'].split();observed={parts[n]:int(parts[n+1],10 if parts[n]in ['combine','position']else 16)for n in range(0,len(parts),2)};parts=rawrows['omf_segment_owners'].split();owners={parts[n]:int(parts[n+1],16)for n in range(0,len(parts),2)};observed['segment_aliases']=[n for n,p in owners.items()if p==observed['segment']];observed['kind_hex']=rawrows['omf_initializer_kind_hex'];kind=struct.unpack('<i',bytes.fromhex(observed['kind_hex']))[0];payload={}
if kind==1:
 payload=dict(datum_hex=rawrows['omf_initializer_datum_hex'],repeat_hex=rawrows['omf_initializer_repeat_hex'],data_pointer=int(rawrows['omf_initializer_data_pointer'],16),data_hex=rawrows['omf_initializer_data_hex'])
elif kind==2:
 parts=rawrows['omf_initializer_reference'].split();reference={parts[n]:int(parts[n+1],16 if parts[n]in ['pointer','member']else 10)for n in range(0,len(parts),2)};payload=dict(relocation_hex=rawrows['omf_initializer_relocation_hex'],reference=reference,reference_name_hex=rawrows['omf_initializer_reference_name_hex'])
elif kind==4:payload=dict(alignment_hex=rawrows['omf_initializer_alignment_hex'])
elif kind==8:payload=dict(padding_hex=rawrows['omf_initializer_padding_hex'])
observed['payload']=payload
assert args==[observed['initializer'],observed['symbol'],observed['segment']]and observed['return_address']==0x693f8 and native['caller']=='emit_initializer'and native['initializer_size']==28
for d in [observed,native]:assert d['initializer']==d['loop_owner']and d['symbol']==d['symbol_owner']and d['segment']==d['segment_owner']and d['initializer']and d['segment']
assert bool(observed['symbol'])==bool(native['symbol'])and observed['kind_hex']==native['kind_hex']and observed['combine']==native['combine']and observed['position']==native['position']and observed['segment_aliases']==native['segment_aliases']and observed['segment_aliases']
def projection(d):
 p=d['payload'].copy()
 if kind==1:
  dtype,width=struct.unpack('<2h',bytes.fromhex(p['datum_hex']));assert 0<=width<=4096 and (dtype==6 or width<=12)and len(bytes.fromhex(p['data_hex']))==width
  if dtype!=6:assert p['data_pointer']==d['initializer']+12
  p.pop('data_pointer')
 elif kind==2:
  r=p['reference'].copy();assert r['pointer']==r['member']and r['identifier']==struct.unpack('<I',bytes.fromhex(p['relocation_hex'])[:4])[0]and r['bucket']==r['identifier']&63;r.pop('pointer');r.pop('member');p['reference']=r
 return p
assert projection(observed)==projection(native)
r=dict(matched=True,function=name,original_va='0x6afc0',formals=[dict(name=key,location=location,type=typ,original=observed[key],native=native[key])for key,location,typ in [('initializer','ECX','Initializer pointer'),('symbol','EDX','SymbolHeader pointer'),('segment','entryESP+4','Segment pointer')]],hidden_inputs=[],callee_stack_cleanup=4,no_hidden_inputs_proof='Original6afc0..6b1a9 receives Initializer ECX/unused Symbol EDX/Segment stack+4, RET4. All3 actual args alias original emit_initializer loopEDI/symbolEBX/segmentEBP at call693f2 return693f8; native exact caller variables in emit_initializer. Active initializer kind/union fields, Segment.combine/position/named owner and referenced lookup kind/label/global hash owner when used observed. No other incoming registers/flags/x87/output pointer. Unused Symbol formal still observed by caller identity; no pointee read/seed invented.',original=observed,native=native,kind=kind,normalized_payload=projection(observed),original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual3-formal callback ABI and only active union fields, source data bounded by observed width if datum, referenced global owner/kind if relocation. Caller alias proof applies to normal/inline CodeView chains without assuming all nodes are global-list heads. No complete28-byte union/tail or old output snapshot; observer4096/1024 bounds are observation-only, never target guards. Whole body/children/return/cold branches/artifact still unaccepted by this input observation.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual relocation callback3 inputs/active union match kind',kind,'payload',r['normalized_payload'])
