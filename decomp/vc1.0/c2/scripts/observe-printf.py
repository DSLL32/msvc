"""Observe original/native narrow printf; never called by cached fixtures."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/dbglist-printf');receipt.mkdir(exist_ok=False)
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='printf-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file() and not p.is_symlink();p.unlink()
    for row in phase['inputs']:
        data=(fixture/row['path']).read_bytes();assert hashlib.sha256(data).hexdigest()==row['sha256'];(work/Path(row['path']).name).write_bytes(data)
    original,rebuilt,config,records,unused=idx.indexes();data=bytearray(original.data)
    assert original.read(0x71b40,1)==b'\x56'
    data[original.offset(0x71b40)]=0xcc;image=scratch/'C23232.EXE';image.write_bytes(data)
    env=idx.wt.environment();env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(receipt/'original.txt'))
    command=['wine',windows(Path('build/reference-printf-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
    result=subprocess.run(command,env=env,capture_output=True,timeout=60)
    assert result.returncode==0,(result.stdout,result.stderr)
original_seconds=time.perf_counter()-started
scratch=Path(tempfile.mkdtemp(prefix='printf-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break parse_options','-ex','run > native-inferior.stdout','-ex','break recon_printf','-ex','continue',
    '-ex','bt 4','-ex','set $fmt=format','-ex','set $arg=*(char**)($ebp+12)',
    '-ex','printf "C2_PRINTF_FORMAT %s",$fmt','-ex','printf "C2_PRINTF_ARGUMENT %s\\n",$arg','-ex','finish','-ex','printf "C2_PRINTF_RESULT %d\\n",$eax',str(Path('build/native/C23232.EXE').resolve())]
phase_started=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-phase_started
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace');assert result.returncode==0,log
left=dict(line.split(' ',1)for line in (receipt/'original.txt').read_text().splitlines());left['result']=int(left['result']);return_va=left.pop('return_va')
assert left['format_hex']==b'\tTITLE\t%s\n\t.386P\n\0'.hex(),left
assert left['argument_hex']==b'dbglist.c\0'.hex(),left
assert 'C2_PRINTF_FORMAT \tTITLE\t%s\n\t.386P\n' in log,log
assert 'C2_PRINTF_ARGUMENT dbglist.c\n' in log,log
right=dict(format_hex=b'\tTITLE\t%s\n\t.386P\n\0'.hex(),argument_hex=b'dbglist.c\0'.hex(),result=int(re.search(r'C2_PRINTF_RESULT (-?\d+)',log)[1]))
assert left==right,(left,right)
(receipt/'validation.json').write_text(json.dumps(dict(original=left,native=right,matched=True,return_va=return_va,original_observer_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,observation_scope='Otherwise unmodified original with only printf entry and its dynamic return breakpoint; no masked acceptance credit for CRT children',executable_sha256=hashlib.sha256(Path('build/native/C23232.EXE').read_bytes()).hexdigest()),indent=2)+'\n')
print(left)
