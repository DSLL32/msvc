#!/usr/bin/env python3
"""Cached complete MRE regression: Linux ELF, strict captured dependency, no GDB/Wine."""
import argparse,contextlib,hashlib,importlib.util,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from build_inputs import verify_native_build
from case_inputs import load,copy_inputs
from mre_native import Provider
P=Path(__file__).resolve().parents[1];ROOT=P.parents[2];sha=lambda b:hashlib.sha256(b).hexdigest()
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--config',type=Path);a=p.parse_args();start=time.perf_counter();ref=P/'cases/mre-object/reference';master=json.loads((ref/'fixture.json').read_bytes());config=json.loads((a.config or P/'functions.json').read_bytes());case=load('mre-object')
assert master['arguments']==case['arguments']and master['inputs']==case['inputs']
assert set(master['required_accepted'])<=set(config['accepted'])
assert sha((ref/'cached-contract.json').read_bytes())==master['cached_contract_sha256']and sha((ref/'packet.json').read_bytes())==master['packet_sha256']
env={k:v for k,v in os.environ.items()if not k.startswith(('RECON_','CL_'))and k not in('MAKEFLAGS','MFLAGS')}
build=time.perf_counter();subprocess.run(['make','--no-print-directory','-j4','native-trace'],cwd=P,env=env,check=True,stdout=subprocess.DEVNULL);build=time.perf_counter()-build
binary=P/'build/native-trace/CL.EXE';binary_hash=sha(binary.read_bytes());receipt=json.loads(Path(str(binary)+'.build.json').read_bytes());verify_native_build(receipt,binary_hash,P)
with tempfile.TemporaryDirectory(prefix='fixture-mre-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as tmp:
 d=Path(tmp);setup=time.perf_counter();copy_inputs(case,d);shutil.copyfile(ref/'resources.bin',d/'resources.bin')
 assert sha((d/'resources.bin').read_bytes())==master['native_resource_provider_sha256']
 (d/'trace-filter.txt').write_text(''.join(r['native_symbol']+'\n'for r in config['driver']));(d/'trace-seen.txt').write_text('')
 actual_cwd=Path(master['program_path'][3:].replace('\\','/')).parent;mirror=d/'drive-s'/actual_cwd;mirror.parent.mkdir(parents=True);mirror.symlink_to(d,target_is_directory=True)
 def module(name):
  raw=subprocess.check_output(['git','show',receipt['shared_revision']+':lib/recon/'+name+'.py'],cwd=ROOT);f=d/(name+'_reader.py');f.write_bytes(raw);spec=importlib.util.spec_from_file_location('fixture_mre_'+name,f);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
 status_module=module('status');mktemp_module=module('mktemp')
 with contextlib.ExitStack()as contexts:
  terminal=contexts.enter_context(status_module.TerminalStatus());call=master['temporary_name_boundary']['calls'][0];temporary=contexts.enter_context(mktemp_module.MktempObservation(call['template'],call['result'][-6:],[call['before'][n]for n in('errno','doserrno','last_error')]))
  provider=Provider(ref,d,jobs_candidate=True,cached=True)
  try:
   for key,value in master['environment'].items():
    if value is None:env.pop(key,None)
    else:env[key]=value
   env.update(provider.environment(),**terminal.environment(),**temporary.environment(),CL_ENTRY_PROFILE=master['entry_profile'],RECON_PROGRAM_PATH=master['program_path'],RECON_DRIVE_R=str(ROOT),RECON_DRIVE_S=str(d/'drive-s'),RECON_RESOURCE_PROVIDER=str(d/'resources.bin'),RECON_TRACE_FILTER=str(d/'trace-filter.txt'),RECON_TRACE_SEEN=str(d/'trace-seen.txt'))
   with(d/'stdout.bin').open('wb')as out,(d/'stderr.bin').open('wb')as err,(d/'trace.txt').open('wb')as trace:
    env['RECON_TRACE_FD']=str(trace.fileno());setup=time.perf_counter()-setup;execution=time.perf_counter();process=subprocess.Popen(master['argv'],executable=str(binary),cwd=d,env=env,stdin=subprocess.DEVNULL,stdout=out,stderr=err,pass_fds=(*provider.descriptors(),terminal.fd,temporary.fd,trace.fileno()));process.wait(timeout=10);execution=time.perf_counter()-execution
   check=time.perf_counter();assert terminal.read(process.pid,process.returncode)==0
   observed=temporary.read(process.pid);assert observed['before']==call['template']and observed['after']==call['result']and observed['argument']==observed['result']
   for phase in('before','after'):assert{n:observed[phase+'_'+n]for n in('errno','doserrno','last_error')}==call[phase]
   result=provider.finish(process.pid);assert result['cached']is True
   for n in('stdout','stderr'):
    expected=(ref/(n+'.bin')).read_bytes();assert sha(expected)==master['streams'][n]and(d/(n+'.bin')).read_bytes()==expected
   assert sha((ref/'trace.txt').read_bytes())==master['trace_sha256']and(d/'trace.txt').read_bytes()==(ref/'trace.txt').read_bytes()
   assert sha((d/'case.idb').read_bytes())==master['final_idb_sha256']
   check=time.perf_counter()-check
  finally:provider.close()
report={'passed':True,'case':'mre-object','mode':'cached native regression; fresh raw/GDB acceptance separate','application_entries':len((ref/'trace.txt').read_bytes().splitlines()),'MRE_calls':6,'mktemp_calls':1,'status':0,'final_idb_bytes':25600,'native_sha256':binary_hash,'seconds':{'build':build,'setup':setup,'execution':execution,'checks':check,'total':time.perf_counter()-start}}
(P/'build/fixture-mre-times.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
