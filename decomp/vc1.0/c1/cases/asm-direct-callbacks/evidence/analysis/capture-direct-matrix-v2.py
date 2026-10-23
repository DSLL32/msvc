from pathlib import Path
import json,subprocess,time,sys,os,importlib.util
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');C=B/'cases/asm-direct-callbacks';sp=importlib.util.spec_from_file_location('contract',B/'scripts/direct-contract.py');module=importlib.util.module_from_spec(sp);sp.loader.exec_module(module);cfg=json.loads((B/'functions.json').read_text());rows=[]
for i,row in enumerate(json.loads((C/'manifest.json').read_text())['direct_plan']):
 tag='asm-direct-v2-%02d-997'%i;original=S/'captures'/tag;native=S/'validation'/tag;ops=[]
 commands=[('original', ['python3',str(B/'scripts/capture-frontier.py'),'--case',str(C),'--directory',str(original),'--debugger',str(S/'tools/capture-original.exe'),'--stop-function','_report_asm_operand_error@8','--direct-function',row['function'],'--direct-arguments',*[str(v)for v in row['arguments']],'--direct-body']),('native',['python3',str(B/'scripts/direct-native.py'),'--function',row['function'][1:].split('@')[0],'--arguments',*[str(v)for v in row['arguments']],'--directory',str(native),'--body'])]
 for label,command in commands:
  env=dict(os.environ,C1_READ_MEMORY='4b228:4,5b700:4,610f4:10,4b148:10');start=time.perf_counter()
  with(S/(tag+'-'+label+'.log')).open('w')as output:code=subprocess.call(command,cwd=B,env=env,stdout=output,stderr=subprocess.STDOUT)
  ops.append(dict(kind=label,seconds=time.perf_counter()-start,exit=code,command=command));assert not code,(tag,label)
 result=module.validate(original,native,dict(logical=row['function'],arguments=row['arguments']),cfg);(native/'comparison.json').write_text(json.dumps(result,indent=2)+'\n');rows.append(dict(index=i,original=str(original),native=str(native),input=row,operations=ops,comparison=result));(S/'asm-direct-matrix-v2.json').write_text(json.dumps(rows,indent=2)+'\n');print(tag,row['function'],row['arguments'],'PASS',flush=True)
