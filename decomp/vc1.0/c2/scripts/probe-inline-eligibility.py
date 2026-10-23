import sys,json,hashlib,shutil,subprocess,tempfile,time
from pathlib import Path
sys.path.insert(0,'scripts')
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,windows
started=time.perf_counter()
manifest=Path('../fixtures/c2-inline-records/manifest.json');raw=manifest.read_bytes();phase=json.loads(raw)['phases']['c2']
with capture_lock():
 directory=Path(tempfile.mkdtemp(prefix='inlrec-eligibility-',dir=C2_SCRATCH));work=C2_SCRATCH/'inlrec'
 for item in phase['inputs']:shutil.copyfile(manifest.parent/item['path'],work/Path(item['path']).name)
 original=idx.PE(idx.wt.TC/'bin/C23232.EXE');data=bytearray(original.data);address=0x65690;data[original.offset(address)]=0xcc
 image=directory/'C23232.EXE';image.write_bytes(data)
 env=idx.wt.environment();env['MSC_CMD_FLAGS']=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin'))
 command=['wine',windows(Path('build/debug-run.exe')),windows(image),'',*[windows(directory/name) for name in ['stdout','stderr','events']],windows(work)]
 setup=time.perf_counter()-started;phase_started=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45);execution=time.perf_counter()-phase_started
 assert r.returncode==0,(r.returncode,r.stdout,r.stderr)
 events=(directory/'events').read_text();assert events=='exception 80000003 rva 00055690 base 00010000 first 1\nexit 99\n',events
 assert not (directory/'stdout').read_bytes() and not (directory/'stderr').read_bytes()
 for name in ['stdout','stderr','events']:shutil.copyfile(directory/name,Path('cases')/('inlrec-eligibility.'+name))
 d=dict(method='Original-only eligibility analysis: unchanged original code except INT3 at new_inline_site entry; not reconstruction acceptance',logical='@new_inline_site@16',original_va=hex(address),target_sha256=hashlib.sha256(original.data).hexdigest(),probe_image_sha256=hashlib.sha256(data).hexdigest(),manifest_sha256=hashlib.sha256(raw).hexdigest(),events=events,wine_prefix=str(idx.wt.PREFIX),canonical_working_directory=phase['canonical_working_directory'],inputs={r['path']:r['sha256'] for r in phase['inputs']},environment=phase['environment'],timing=dict(setup_seconds=setup,execution_seconds=execution,total_seconds=time.perf_counter()-started),limits='This proves original inliner candidate construction executes, not that every helper or every site ultimately inlines. Normal acceptance uses fully masked original/native matching separately.')
 Path('cases/inlrec-eligibility.json').write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d,indent=2))
