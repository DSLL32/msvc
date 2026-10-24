"""Original-only actual hidden EAX at SEGDEF8 or the COMDAT callsite."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
ap=argparse.ArgumentParser();ap.add_argument('--comdat',action='store_true');args=ap.parse_args();started=time.perf_counter();receipt=Path('cases/comdat-alignment-selection-original'if args.comdat else'cases/comdat-alignment8-original');receipt.mkdir();name='@omf_alignment@4';fixture=Path('../fixtures/c2-omf-comdat-communal');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='alignment8-entry-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'packets'
 for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
 for r in phase['inputs']:
  p=fixture/r['path'];assert sha(p)==r['sha256'];shutil.copyfile(p,work/p.name)
 original,canonical,cfg,_,_=idx.indexes();row=next(r for r in cfg['driver']if r['name']==name);assert row['start']==0x6a310;plan=receipt/'entry-plan.txt';plan.write_text('%x %02x 257 %s\n'%(row['start']-original.base,original.read(row['start'],1)[0],name));image=scratch/'C23232.EXE';image.write_bytes(original.data);env=idx.wt.environment()
 for k in ['C2_PRINTF_FORMAT','C2_STREAM_INPUT','C2_STOP_RETURN','C2_STOP_ENTRY']:env.pop(k,None)
 env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_PLAN=windows(plan),C2_ENTRY_INPUT='1',C2_ALIGNMENT_EAX_CAPTURE='2'if args.comdat else'1',C2_STOP_RETURN=name);t=time.perf_counter();r=subprocess.run(['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(receipt/('original.'+k))for k in ['stdout','stderr','events']],windows(work)],env=env,capture_output=True,timeout=60);original_seconds=time.perf_counter()-t;(receipt/'observer.stdout').write_bytes(r.stdout);(receipt/'observer.stderr').write_bytes(r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
binding=dict(target_sha256=sha(idx.wt.TC/'bin/C23232.EXE'),manifest_sha256=sha(fixture/'manifest.json'),config_sha256=sha('functions.json'),observer_source_sha256=sha('src/reference-stdio-observer.c'),observer_sha256=sha('build/reference-stdio-observer.exe'),files={n:sha(receipt/n)for n in ['entry-plan.txt','original.txt','original.stdout','original.stderr','original.events','observer.stdout','observer.stderr']});(receipt/'original-binding.json').write_text(json.dumps(binding,indent=2)+'\n')
lines=(receipt/'original.txt').read_text().splitlines();print('\n'.join(lines))
