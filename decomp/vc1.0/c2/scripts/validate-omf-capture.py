"""Explicit independent original OMF stream/artifact validation; no fixture use."""
from pathlib import Path
import hashlib,json,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,windows
from artifacts import compare
started=time.perf_counter();fixture=Path('../fixtures/c2-omf-typed-debug');manifest=fixture/'manifest.json';phase=json.loads(manifest.read_text())['phases']['c2'];out=fixture/'capture';prefix='independent';assert not(out/(prefix+'.json')).exists()
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 work=C2_SCRATCH/'omftypes'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 setup_seconds=time.perf_counter()-started;env=idx.wt.environment();env['MSC_CMD_FLAGS']=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin'))
 command=['wine',windows(Path('build/debug-run.exe')),windows(idx.wt.TC/'bin/C23232.EXE'),'',*[windows(out/(prefix+'.'+n))for n in ['stdout','stderr','events']],windows(work)];begin=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,capture_output=True,timeout=60);execution_seconds=time.perf_counter()-begin
 (out/(prefix+'-supervisor.stdout')).write_bytes(r.stdout);(out/(prefix+'-supervisor.stderr')).write_bytes(r.stderr);begin=time.perf_counter();assert r.returncode==0 and not r.stdout and not r.stderr
 assert not(out/(prefix+'.stdout')).read_bytes()and not(out/(prefix+'.stderr')).read_bytes()and (out/(prefix+'.events')).read_text()=='exit 0\n'
 assert {p.name for p in work.iterdir()}=={Path(r['path']).name for r in phase['inputs']}|{'omftypes.obj'}
 outputs=[compare((work/r['output_name']).read_bytes(),(fixture/r['path']).read_bytes(),r)for r in phase['outputs']]
 report=dict(matched=True,manifest_sha256=sha(manifest),setup_seconds=setup_seconds,execution_seconds=execution_seconds,check_seconds=time.perf_counter()-begin,outputs=outputs,files={p.name:sha(p)for p in out.glob(prefix+'.*')},scope='Independent original full stream/status/artifact/inventory check with distinct child and supervisor paths. Earlier capture helper result logs overlapped child stream paths; those raw files are retained but are not stream evidence. This independent proof supplies actual untouched original C2 stdout/stderr.')
 (out/(prefix+'.json')).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
