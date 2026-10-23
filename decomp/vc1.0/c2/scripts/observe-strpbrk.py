"""Explicit original/native CRT seam capture; not used by fixtures."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys,tempfile,time
sys.path.insert(0,'scripts')
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,windows,native_work
config=json.load(open('functions.json'));phase=json.load(open('../fixtures/c2-debug-asm-listing/manifest.json'))['phases']['c2']
manifest=Path('../fixtures/c2-debug-asm-listing');receipt=Path('cases/dbglist-strpbrk');receipt.mkdir(exist_ok=False)
started=time.perf_counter()
with capture_lock():
 scratch=Path(tempfile.mkdtemp(prefix='strpbrk-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
 for p in work.iterdir():assert p.is_file() and not p.is_symlink();p.unlink()
 for item in phase['inputs']:shutil.copyfile(manifest/item['path'],work/Path(item['path']).name)
 original,rebuilt,cfg,records,unused=idx.indexes();assert original.offset(0x71b00)==0x61500;image=scratch/'C23232.EXE';idx.mask(original,records,[n for n in cfg['accepted'] if n!='_strpbrk'],image)
 env=idx.wt.environment();env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'),C2_CRT_IMAGE=windows(idx.wt.TC/'bin/C23232.EXE'))
 command=['wine',windows(Path('build/reference-strpbrk-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
 result=subprocess.run(command,env=env,capture_output=True,timeout=60)
 assert result.returncode==0,(result.stdout,result.stderr)
 print((receipt/'original.txt').read_text())
original_seconds=time.perf_counter()-started
scratch=Path(tempfile.mkdtemp(prefix='strpbrk-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for item in phase['inputs']:shutil.copyfile(manifest/item['path'],work/Path(item['path']).name)
env.update(PATH='/usr/bin:/bin',LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break parse_options','-ex','run','-ex','break strpbrk','-ex','continue','-ex','set $left=*(char**)($esp+4)','-ex','set $right=*(char**)($esp+8)','-ex','x/s $left','-ex','x/s $right','-ex','finish','-ex','printf "C2_RESULT_NONNULL=%u OFFSET=%d\\n",$eax!=0,$eax?(char*)$eax-$left:-1',str(Path('build/native/C23232.EXE').resolve())]
started=time.perf_counter();result=subprocess.run(command,env=env,cwd=work,capture_output=True,timeout=30)
(receipt/'native.txt').write_bytes(result.stdout+result.stderr);print((receipt/'native.txt').read_text())
(receipt/'timing.json').write_text(json.dumps(dict(original_observer_seconds=original_seconds,native_debug_seconds=time.perf_counter()-started,native_command=command),indent=2)+'\n')
