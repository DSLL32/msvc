"""Observe actual machine operands, callback identity and opaque context ABI."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();out=Path('cases/machine-symbol-visitor-abi');out.mkdir();name='@visit_machine_symbols@12';fixture=Path('../fixtures/c2-seh-packet');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='machine-symbol-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'sehpacket'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for row in phase['inputs']:
  p=fixture/row['path'];assert sha(p)==row['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x27920;plan=out/'entry-plan.txt';plan.write_text('%x %02x 257 @extract_assembly_labels@4\n%x %02x 259 %s\n'%(0x27850-original.base,original.read(0x27850,1)[0],row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN','C2_ALIGNMENT_EAX_CAPTURE']:env.pop(key,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(out/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(out/('original.'+n))for n in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(out/'observer.stdout').write_bytes(r.stdout);(out/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(out/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(out/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='machine-symbol-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
instruction=gdb.parse_and_eval('instruction');visitor=int(gdb.parse_and_eval('visitor'));context=int(gdb.parse_and_eval('context'));operation=(int(instruction['opcode'])>>8)&255;count=int(gdb.parse_and_eval('machine_operand_counts[%d]'%operation));operands=[]
for counter in range(count,0,-1):
 index=1 if counter==2 else 0;operand=instruction['operands'][index];flags=int(operand['flags'])&0xffffffff;kind=flags&15;address=slot=symbol=0
 if kind in [3,4,5,6]:
  pointer=operand['value']['address'];address=int(pointer);slot=address+4;symbol=int(pointer['symbol'])
 elif kind==8:slot=int(operand.address)+4;symbol=int(operand['value']['symbol'])
 operands.append(dict(index=index,flags=flags,kind=kind,address=address,slot=slot,symbol=symbol))
callback=int(gdb.parse_and_eval('&collect_assembly_reference'));caller=gdb.selected_frame().older();caller.select();node=gdb.parse_and_eval('node');owner_context=int(gdb.parse_and_eval('&head'));head=int(gdb.parse_and_eval('head'));print('C2_MACHINE_SYMBOLS '+json.dumps(dict(instruction=int(instruction),visitor=visitor,callback=callback,context=context,owner_context=owner_context,initial_head=head,parent_instruction=int(node['left']['pointer']),operation=operation,count=count,operands=operands,caller=caller.name())))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break visit_machine_symbols','-ex','run > inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(out/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_MACHINE_SYMBOLS (.*)',r.stdout)[1]);lines=(out/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'
def fields(prefix):
 words=next(v for v in lines if v.startswith(prefix+' ')).split()[1:];return {words[i]:int(words[i+1],10 if words[i]in ['operation','count']else 16)for i in range(0,len(words),2)}
observed=fields('machine_symbol_visit');parent=fields('assembly_references');operands=[]
for line in lines:
 if line.startswith('machine_symbol_operand '):
  w=line.split()[1:];operands.append({w[i]:int(w[i+1],10 if w[i]in ['index','kind']else 16)for i in range(0,len(w),2)})
observed['operands']=operands;raw=[int(v,16)for v in next(v for v in lines if v.startswith('enter '+name+' ')).split()[5:]];assert raw==[observed[k]for k in ['instruction','visitor','context']];assert observed['instruction']==parent['instruction']and observed['visitor']==0x279a0 and observed['context']==observed['entry_sp']+12 and observed['initial_head']==0 and observed['return_address']==0x2786e
assert native['instruction']==native['parent_instruction']and native['visitor']==native['callback']and native['context']==native['owner_context']and native['initial_head']==0 and native['caller']=='extract_assembly_labels'
assert (observed['operation'],observed['count'])==(native['operation'],native['count'])
assert [(v['index'],v['kind'],bool(v['symbol']))for v in operands]==[(v['index'],v['kind'],bool(v['symbol']))for v in native['operands']]
for d in [observed,native]:
 for v in d['operands']:
  assert v['kind']==v['flags']&15
  if v['kind']in [3,4,5,6]:assert v['address']and v['slot']==v['address']+4
  elif v['kind']==8:assert v['slot']==d['instruction']+16+v['index']*8+4
  else:assert v['slot']==v['address']==v['symbol']==0
report=dict(matched=True,function=name,formals=[dict(name='instruction',location='ECX',type='MachineInstruction pointer',original=observed['instruction'],native=native['instruction']),dict(name='visitor',location='EDX',type='MachineSymbolVisitor function pointer',logical='@collect_assembly_reference@8',original=observed['visitor'],native=native['visitor']),dict(name='context',location='entryESP+4',type='opaque callback context pointer',original=observed['context'],native=native['context'])],hidden_inputs=[],callee_stack_cleanup=4,return_contract='void; EAX unconsumed',no_hidden_inputs_proof='Original27920..27995 reads operation byte+9, existing signed count table, then operand low4 flags and only kind3..6 address.symbol or kind8 direct value.symbol. CX/EDX/one stack pointer are all input formals, RET4. Callback identity and forwarding context observed; context aliases parent local head (original entryESP+12, native &head), initializedNULL by prior parent. No hidden register/stack/x87/flags. No pointed SymbolHeader fields are read by this function, so none dumped; callback contract is deferred.',original=observed,native=native,original_parent=parent,original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual instruction operation/count and only consumed operand kind/address/symbol slots, callback code identity, opaque context ownership and stack location. Raw full flags preserved; only low4 are consumed/comparable. Parent initial-headNULL is prior encountered output storage, not a new visitor input field or seed. No whole instruction/symbol snapshots or unsupported ABI normalization.')
(out/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print('Actual instruction/count/operand slots, callback and local context ABI match')
