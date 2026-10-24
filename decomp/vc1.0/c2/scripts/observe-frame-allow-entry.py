"""Observe the owned function work/inline-assembly input graph."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();out=Path('cases/frame-allow-abi');out.mkdir();name='@function_allows_frame_register@4';fixture=Path('../fixtures/c2-seh-packet');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='frame-allow-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'sehpacket'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for row in phase['inputs']:
  p=fixture/row['path'];assert sha(p)==row['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x2f500;plan=out/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN','C2_ALIGNMENT_EAX_CAPTURE']:env.pop(key,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(out/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(out/('original.'+n))for n in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(out/'observer.stdout').write_bytes(r.stdout);(out/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(out/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(out/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='frame-allow-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
function=gdb.parse_and_eval('function');work=function['work'];rows=[];wi=0
while int(work):
 assert wi<4096
 block=work['body']['block'];flags=int(block['flags'])&255;row=dict(index=wi,pointer=int(work),next=int(work['next']),block=int(block),flags=flags);rows.append(row)
 if flags&2:
  instruction=block['next'];row['first']=int(instruction);row['instructions']=[];ii=0
  while int(instruction):
   assert ii<65536
   opcode=int(instruction['opcode']);kind=opcode&15;ins=dict(work=wi,index=ii,pointer=int(instruction),kind=kind);row['instructions'].append(ins)
   if kind==7:break
   ins['next']=int(instruction['next'])
   if not kind:
    operation=(opcode>>8)&255;count=int(gdb.parse_and_eval('machine_operand_counts')[operation]);assert 0<=count<=2
    ins.update(operation=operation,count=count,implicit=int(bool(int(gdb.parse_and_eval('machine_selection_properties')[operation])&1)),operands=[])
    for j in range(count-1,-1,-1):
     operand=instruction['operands'][j];okind=int(operand['flags'])&15;o=dict(index=j,kind=okind);ins['operands'].append(o)
     if okind==1:o['reg']=int(operand['value']['reg']['words']['number'])
     if okind in [4,5]:
      address=operand['value']['address'];index=0
      if okind==5:index=int(address['index']['words']['number']);o['index_reg']=index
      if okind==4 or index!=5:base=int(address['base']['words']['number']);o['base']=base
      if okind==4 and base==5:
       symbol=address['symbol'];o['symbol_pointer']=int(symbol)
       if int(symbol):
        skind=int(symbol['kind'])&255;o['symbol_kind']=skind
        if skind==1:o['storage']=int(gdb.parse_and_eval('((TemporarySymbol *)'+str(int(symbol))+')->attributes'))&0x70
   instruction=instruction['next'];ii+=1
 work=work['next'];wi+=1
caller=gdb.selected_frame().older();print('C2_FRAME_ALLOW '+json.dumps(dict(function=int(function),current=int(gdb.parse_and_eval('current_function')),first_work=int(function['work']),caller=caller.name(),works=rows)))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break function_allows_frame_register','-ex','run > inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(out/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_FRAME_ALLOW (.*)',r.stdout)[1]);lines=(out/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'
def fields(line):
 words=line.split()[1:];hexkeys={'pointer','next','block','function','current','first_work','return_address'};return {words[i]:int(words[i+1],16 if words[i]in hexkeys else 10)for i in range(0,len(words),2)}
observed=fields(next(v for v in lines if v.startswith('frame_allow ')));observed['works']=[]
for line in lines:
 if line.startswith('frame_work '):row=fields(line);observed['works'].append(row)
 elif line.startswith('frame_block_first '):d=fields(line);assert d['work']==row['index'];row['first']=d['pointer'];row['instructions']=[]
 elif line.startswith('frame_instruction '):ins=fields(line);assert ins['work']==row['index'];row['instructions'].append(ins)
 elif line.startswith('frame_next '):ins['next']=fields(line)['pointer']
 elif line.startswith('frame_operation '):ins.update(fields(line));ins['operands']=[]
 elif line.startswith('frame_operand '):operand=fields(line);ins['operands'].append(operand)
 elif line.startswith('frame_register '):operand['reg']=fields(line)['value']
 elif line.startswith('frame_index '):operand['index_reg']=fields(line)['value']
 elif line.startswith('frame_base '):operand['base']=fields(line)['value']
 elif line.startswith('frame_symbol '):operand['symbol_pointer']=fields(line)['pointer']
 elif line.startswith('frame_symbol_kind '):operand['symbol_kind']=fields(line)['value']
 elif line.startswith('frame_storage '):operand['storage']=fields(line)['value']
assert observed['return_address']==0x2bf12 and native['caller']=='frame_register_can_be_allocated'
for d in [observed,native]:assert d['function']==d['current']and d['function']and d['first_work']==d['works'][0]['pointer']
from frame_allow_inputs import projection
pa,pb=projection(observed),projection(native);assert pa==pb,(pa,pb)
report=dict(matched=True,function=name,formals=[dict(name='function',location='ECX',type='Function pointer',original=observed['function'],native=native['function'])],hidden_inputs=[],callee_stack_cleanup=0,return_contract='full EAX boolean0/1',no_hidden_inputs_proof='Original2f500..2f681 consumes soleECX function, function.work+12, work.next+0/bodyblock+8, block.flags lowbyte+28 and nextinstruction+0. For flaggedblocks only, kind0 instructions read counts/properties of operationbyte+9, explicitregister4/write predicate, implicitmaskbit10/DATA exclusions, and accepted operand mentions ofregister5. Only basedoperand4 with register5 may read address.symbol+4 and conditional kind/TemporarySymbol.attributes+36 storage bits70; other kinds skip/stop atkind7. No incomingEDX/stack/flags/x87/outpointer. Callee-saved registers set before use. Exact RET0/fullboolean paths.',original=observed,native=native,projection=pa,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),native_shared_source_revision='45f7a2ace',original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual sole formal aliases current_function; closed work.next graph and all flagged-block machine inputs are typed inventory with conditional live operand fields and pointer-alias shape. This is input inventory, not a claim that body executes every listed row before early return. No full descriptors, inactive unions, WorkItem output storage, hidden global seed, child result or branch execution claim. First entry only; full original/canonical controls and fresh frontier acceptance follow.')
(out/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print('Actual function/current alias and',len(pa),'work blocks match with typed assembly inputs')
