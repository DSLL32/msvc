"""Explicit original/native CRT seam capture; not used by fixtures."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys,tempfile,time
sys.path.insert(0,'scripts')
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,windows,native_work
config=json.load(open('functions.json'));phase=json.load(open('../fixtures/c2-debug-asm-listing/manifest.json'))['phases']['c2']
manifest=Path('../fixtures/c2-debug-asm-listing');receipt=Path('cases/dbglist-stdhandle');receipt.mkdir(exist_ok=False)
started=time.perf_counter()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='stdhandle-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
 for p in work.iterdir():assert p.is_file() and not p.is_symlink();p.unlink()
 for item in phase['inputs']:shutil.copyfile(manifest/item['path'],work/Path(item['path']).name)
 original,rebuilt,cfg,records,unused=idx.indexes();assert original.offset(0x78816)==0x68216;image=scratch/'C23232.EXE';idx.mask(original,records,[n for n in cfg['accepted'] if n!='import:SetStdHandle'],image)
 env=idx.wt.environment();env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_IMAGE=windows(idx.wt.TC/'bin/C23232.EXE'))
 command=['wine',windows(Path('build/reference-stdhandle-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
 result=subprocess.run(command,env=env,capture_output=True,timeout=60)
 assert result.returncode==0,(result.stdout,result.stderr)
 print((receipt/'original.txt').read_text())
original_seconds=time.perf_counter()-started
(receipt/'timing.json').write_text(json.dumps(dict(original_observer_seconds=original_seconds),indent=2)+'\n')
