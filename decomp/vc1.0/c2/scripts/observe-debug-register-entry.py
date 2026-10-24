"""Observe no-formal fixup writer count and typed pool identity, no field dump."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/debug-register-abi');receipt.mkdir();name='_record_debug_register_locations';fixture=Path('../fixtures/c2-omf-comdat-communal');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='debug-register-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'packets'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x2b1b0;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 0 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='debug-register-entry-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for r in phase['inputs']:shutil.copyfile(fixture/r['path'],work/Path(r['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
heads=[];nodes=[];effective=[];buckets=gdb.parse_and_eval('register_variable_buckets')
for group in range(3):
 for bucket in range(32):
  node=buckets[group][bucket];heads.append([group,bucket,int(node)]);index=0
  while int(node):
   index+=1;assert index<=4096;v=node.dereference();home=v['home_symbol'];kind=int(home['kind'])if int(home)else 0;attributes=int(home.cast(gdb.lookup_type('BackendSymbol').pointer())['attributes'])if int(home)and kind==1 else 0
   nodes.append(dict(node=int(node),next=int(v['next']),home=int(home),kind=kind,attributes=attributes))
   if int(home)and kind==1 and attributes&0x70==0x10:
    flags=int(v['flags']);rep=int(v['representative'])if flags&0x800 else 0;target=node
    if flags&0x800:
     target=buckets[rep>>14][rep&31];count=0
     while int(target)and int(target['number'])!=rep:target=target['next'];count+=1;assert count<4096
     assert int(target)
    effective.append(dict(owner=int(node),source_flags=flags,representative=rep,target=int(target),flags=int(target['flags']),cost=int(target['spill_cost']),conflicts=bool(int(target['conflicts']))))
   node=v['next']
print('C2_DEBUG_REGISTERS '+json.dumps(dict(heads=heads,nodes=nodes,effective=effective,caller=gdb.selected_frame().older().name())))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break record_debug_register_locations','-ex','run > native-inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(receipt/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_DEBUG_REGISTERS (.*)',r.stdout)[1]);lines=(receipt/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'and lines[0].split()[:2]==['enter',name]and len(lines[0].split())==5
observed=dict(heads=[],nodes=[],effective=[],return_address=int(next(v.split()[1]for v in lines if v.startswith('debug_register_caller ')),16))
for line in lines:
 if line.startswith('debug_register_head '):parts=line.split();observed['heads'].append([int(parts[1]),int(parts[2]),int(parts[3],16)])
 elif line.startswith('debug_register_node '):
  parts=line.split();d=dict(node=int(parts[1],16));d.update({parts[n]:int(parts[n+1],10 if parts[n]=='kind'else 16)for n in range(2,len(parts),2)});observed['nodes'].append(d)
 elif line.startswith('debug_register_effective '):
  parts=line.split()[1:];observed['effective'].append({parts[n]:int(parts[n+1],10 if parts[n]in['representative','cost','conflicts']else 16)for n in range(0,len(parts),2)})
def shape(d):
 assert len(d['heads'])==96 and[(g,b)for g,b,_ in d['heads']]==[(g,b)for g in range(3)for b in range(32)]
 owners={v['node']:i+1 for i,v in enumerate(d['nodes'])};assert len(owners)==len(d['nodes']);homes={};rows=[]
 for v in d['nodes']:
  if v['home']not in homes:homes[v['home']]=len(homes)+1 if v['home']else 0
  rows.append(dict(next=owners.get(v['next'],0),home=homes[v['home']],kind=v['kind'],attributes=v['attributes']))
 effective=[dict(owner=owners[v['owner']],source_flags=v['source_flags'],representative=v['representative'],target=owners[v['target']],flags=v['flags'],cost=v['cost'],conflicts=bool(v['conflicts']))for v in d['effective']]
 return dict(heads=[[g,b,owners.get(p,0)]for g,b,p in d['heads']],nodes=rows,effective=effective)
assert shape(observed)==shape(native)and native['caller']=='allocate_machine_registers'
r=dict(matched=True,function=name,original_va='0x2b1b0',formals=[],hidden_inputs=[],callee_stack_cleanup=0,no_hidden_inputs_proof='Original2b1b0..2b249 initializes own three groups/32 buckets/800mask and consumes no incoming register/stack/flags/x87/out pointer; RET0. Actual global3x32 closed register lists, next/home identity, conditional symbolkind/attributes, eligible variable aliasflags/representative and actual effective target flags/spillcost/conflicts presence observed. Childlookup is accepted/read-only; resolved effective target aliases captured bucket member. Old symbolfield28 output is never read/dumped/seeded. Caller allocate_machine_registers and original return recorded.',shape=shape(observed),original=observed,native=native,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Defined conditional list context only, no complete RegisterVariable/Symbol snapshots, names/unuseddegree/padding/outputword omitted.4096 cap observer-only. Full body/controls and fresh acceptance pending.')
(receipt/'validation.json').write_text(json.dumps(r,indent=2)+'\n');print('Actual debug-register context matches',len(r['shape']['nodes']),'nodes',len(r['shape']['effective']),'eligible',hex(observed['return_address']))
