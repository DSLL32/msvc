"""Observe the scope-discovery expression graph and label identities."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();out=Path('cases/seh-collect-abi');out.mkdir();name='@collect_exception_scopes@4';fixture=Path('../fixtures/c2-seh-packet');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='seh-collect-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'sehpacket'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for row in phase['inputs']:
  p=fixture/row['path'];assert sha(p)==row['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x65b00;plan=out/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for key in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN','C2_ALIGNMENT_EAX_CAPTURE']:env.pop(key,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(out/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_STOP_ENTRY=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(out/('original.'+n))for n in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(out/'observer.stdout').write_bytes(r.stdout);(out/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(out/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(out/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
scratch=Path(tempfile.mkdtemp(prefix='seh-collect-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
script="""import gdb,json
function=gdb.parse_and_eval('function');expression=function['expressions'];rows=[];index=0
while int(expression):
 assert index<4096
 tree=expression['tree'];opcode=int(tree['opcode']);row=dict(index=index,pointer=int(expression),next=int(expression['next']),tree=int(tree),opcode=opcode);rows.append(row)
 if opcode in [0x29,0x3a]:symbol=tree['right']['symbol'];row.update(symbol=int(symbol),marker=int(symbol['field0d'])&255)
 expression=expression['next'];index+=1
caller=gdb.selected_frame().older();caller.select();parent=int(gdb.parse_and_eval('function'));print('C2_SEH_COLLECT '+json.dumps(dict(function=int(function),current=int(gdb.parse_and_eval('current_function')),parent=parent,first=int(function['expressions']),depth=int(gdb.parse_and_eval('exception_finally_depth')),caller=caller.name(),expressions=rows)))"""
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break collect_exception_scopes','-ex','run > inferior.stdout','-ex','python '+script,str(Path('build/native/C23232.EXE').resolve())];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-t;(out/'native.txt').write_bytes(r.stdout);assert r.returncode==0,r.stdout;native=json.loads(re.search(rb'C2_SEH_COLLECT (.*)',r.stdout)[1]);lines=(out/'original.txt').read_text().splitlines();assert lines[-1]=='entry_end'
def fields(line):
 words=line.split()[1:];hexkeys={'function','current','parent','first','return_address','pointer','next','tree'};return {words[i]:int(words[i+1],16 if words[i]in hexkeys else 10)for i in range(0,len(words),2)}
observed=fields(next(v for v in lines if v.startswith('seh_collect ')));observed['expressions']=[]
for line in lines:
 if line.startswith('seh_expression '):row=fields(line);observed['expressions'].append(row)
 elif line.startswith('seh_label '):v=fields(line);row.update(symbol=v['pointer'],marker=v['marker'])
assert observed['return_address']==0x65717 and native['caller']=='prepare_inline_function'
for d in [observed,native]:assert d['function']==d['current']==d['parent']and d['function']and d['first']==d['expressions'][0]['pointer']and d['depth']==0
from seh_collect_inputs import projection
pa,pb=projection(observed),projection(native);assert pa==pb,(pa,pb)
report=dict(matched=True,function=name,formals=[dict(name='function',location='ECX',type='Function pointer',original=observed['function'],native=native['function'])],hidden_inputs=[],callee_stack_cleanup=0,return_contract='void; caller ignores EAX',no_hidden_inputs_proof='Original65b00..65d26 consumes soleECX Function, saved for final expressions reload. Directinput Function.expressions+16, closed Expression.next+0/tree+4, Node.opcode+0, conditional label/jump right.symbol+20/markerbyte+13 and pointeridentity; local returnmarker found by opcode47, speciallabel jump relocation searches actualsymbol identity. Global finallydepth7b9a0 updated on8e/90; actual0. New/queried scope pointers and newlabel are childoutputs, not seeded incoming payload. No incoming otherGPR/stack/flags/x87/outpointer; RET0/void. Actual Function aliases wrapperESI/current_function; native parentformal matches.',original=observed,native=native,projection=pa,binding=binding,native_executable_sha256=sha('build/native/C23232.EXE'),native_shared_source_revision='574227e0e09ab9b2aee5d295ecfa185143a9615d',original_execution_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,scope='Actual soleformal, closed inputexpression graph with conditional label marker fields and both Node/label alias identities, current/caller ownership and globalfinallydepth0. No fullFunction/Node/Symbol snapshots, inactive unions, old write-only label.scope or Node.right outputs, childscope payload/globalstack seed, or priorreturnlabel input. Whole550B body controls/fresh acceptance remain required; initial graph inventory is not an executed splice/branch claim.')
(out/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print('Actual Function and',len(pa),'closed expression/label inputs match')
