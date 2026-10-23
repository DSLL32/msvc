"""Explicit original/native segment identity observation; outside fixtures."""
from pathlib import Path
import hashlib,json,os,re,shutil,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,ROOT,capture_lock,native_work,windows
started=time.perf_counter();receipt=Path('cases/dbglist-debug-segment-state');receipt.mkdir(exist_ok=False)
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='debug-segment-original-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file() and not p.is_symlink();p.unlink()
    for row in phase['inputs']:
        data=(fixture/row['path']).read_bytes();assert hashlib.sha256(data).hexdigest()==row['sha256'];(work/Path(row['path']).name).write_bytes(data)
    original,rebuilt,config,records,unused=idx.indexes();image=scratch/'C23232.EXE'
    idx.mask(original,records,[n for n in config['accepted'] if n!='_write_codeview_object_name'],image)
    env=idx.wt.environment();env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_SEGMENT_CAPTURE=windows(receipt/'original.txt'))
    command=['wine',windows(Path('build/reference-debug-segment-observer.exe')),windows(image),'',*[windows(receipt/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
    result=subprocess.run(command,env=env,capture_output=True,timeout=60)
    assert result.returncode==0,(result.stdout,result.stderr)
original_seconds=time.perf_counter()-started
scratch=Path(tempfile.mkdtemp(prefix='debug-segment-native-',dir=C2_SCRATCH));work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:shutil.copyfile(fixture/row['path'],work/Path(row['path']).name)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),RECON_PROGRAM_PATH=windows(idx.wt.TC/'bin/C23232.EXE'))
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break write_codeview_object_name','-ex','run > native-inferior.stdout','-ex','set $s=debug_symbols_segment','-ex',
 'printf "C2_SEG offset=%u kind=%u identifier=%u alignment=%u format_word_section=%u object_declared=%u identity=%u\\n",$s->symbol.field04,$s->symbol.kind,$s->symbol.identifier,$s->alignment,$s->format_flags+($s->word_size<<8)+($s->section_number<<16),!!$s->object_section,$s->identity.order',
 '-ex','printf "C2_SEG_NAME %s\\nC2_SEG_CLASS %s\\n",$s->symbol.name,$s->class_name',str(Path('build/native/C23232.EXE').resolve())]
phase_started=time.perf_counter();result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);native_seconds=time.perf_counter()-phase_started
(receipt/'native.txt').write_bytes(result.stdout);log=result.stdout.decode(errors='replace')
assert result.returncode==0,log
left={k:(v if k in ['name','class'] else int(v))for k,v in (line.split(' ',1)for line in (receipt/'original.txt').read_text().splitlines())}
m=re.search(r'C2_SEG (.*)',log);assert m,log
right={k:int(v)for k,v in (item.split('=')for item in m[1].split())};right.update(name=re.search(r'C2_SEG_NAME (.*)',log)[1],**{'class':re.search(r'C2_SEG_CLASS (.*)',log)[1]})
assert left==right,(left,right)
assert left['offset']==4 and left['identifier']==7 and left['object_declared']==1 and left['identity']==5,left
(receipt/'validation.json').write_text(json.dumps(dict(original=left,native=right,matched=True,stage='Entry of object-name writer, after signature writer has declared the shared original7914c segment and emitted four bytes',original_observer_seconds=original_seconds,native_debug_seconds=native_seconds,total_seconds=time.perf_counter()-started,executable_sha256=hashlib.sha256(Path('build/native/C23232.EXE').read_bytes()).hexdigest()),indent=2)+'\n')
print(left)
