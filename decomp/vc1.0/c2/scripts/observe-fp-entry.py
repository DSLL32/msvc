"""Explicit original-only startup precision proof, stopping at main entry."""
from pathlib import Path
import hashlib,json,re,struct,subprocess,tempfile,time
import pe_indexes as idx
from case_paths import C2_SCRATCH,capture_lock,windows
started=time.perf_counter();out=Path('cases/dbglist-fp-entry');out.mkdir(exist_ok=False)
fixture=Path('../fixtures/c2-debug-asm-listing');phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
with capture_lock():
    scratch=Path(tempfile.mkdtemp(prefix='fp-entry-',dir=C2_SCRATCH));work=C2_SCRATCH/'dbglist'
    for p in work.iterdir():assert p.is_file()and not p.is_symlink();p.unlink()
    for row in phase['inputs']:
        data=(fixture/row['path']).read_bytes();assert hashlib.sha256(data).hexdigest()==row['sha256'];(work/Path(row['path']).name).write_bytes(data)
    original,unused,config,records,unused=idx.indexes();main=next(r for r in config['driver']if r['name']=='_main');plan=out/'entry-plan.txt'
    plan.write_text('%x %02x 3 _main\n'%(main['start']-original.base,original.read(main['start'],1)[0]))
    image=scratch/'C23232.EXE';image.write_bytes(original.data)
    env=idx.wt.environment();env.update(MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(idx.wt.TC/'bin')),C2_CRT_CAPTURE=windows(out/'original.txt'),C2_CRT_PLAN=windows(plan),C2_PRINTF_FORMAT='; %g',C2_STOP_ENTRY='_main')
    command=['wine',windows(Path('build/reference-stdio-observer.exe')),windows(image),'',*[windows(out/('original.'+name))for name in ['stdout','stderr','events']],windows(work)]
    execution=time.perf_counter();result=subprocess.run(command,env=env,capture_output=True,timeout=60);seconds=time.perf_counter()-execution
    (out/'observer.stdout').write_bytes(result.stdout);(out/'observer.stderr').write_bytes(result.stderr);assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
text=(out/'original.txt').read_text();assert text.startswith('enter _main ')
fields=re.search(r'^numeric (.*)$',text,re.M)[1].split();state={fields[i]:int(fields[i+1],16)for i in range(0,len(fields),2)}
assert state['cw']&0xffff==0x027f and state['sw']&0xffff==0
assert struct.unpack('<I',original.read(0x86648,4))[0]==0x71cb0
assert original.read(0x74360,19).hex()=='68000003006800000100e8c1e2ffff83c408c3'
receipt=dict(target_sha256=hashlib.sha256(original.data).hexdigest(),main_entry=hex(main['start']),stopped_before_application=True,original_control_word=state['cw']&0xffff,original_status_word=state['sw']&0xffff,registers=state,
 startup_chain=['__cinit72470 loads86648=71cb0','__fpmath71cb0 (LIBC fpinit.obj) calls conversion-pointer initialization71cd0 then tail-jumps74360','74360 pushes mask0x30000/value0x10000 and calls_controlfp72630, selecting53-bit precision'],
 plan_sha256=hashlib.sha256(plan.read_bytes()).hexdigest(),observer_source_sha256=hashlib.sha256(Path('src/reference-stdio-observer.c').read_bytes()).hexdigest(),observer_sha256=hashlib.sha256(Path('build/reference-stdio-observer.exe').read_bytes()).hexdigest(),manifest_sha256=hashlib.sha256((fixture/'manifest.json').read_bytes()).hexdigest(),original_execution_seconds=seconds,total_seconds=time.perf_counter()-started,
 scope='Untouched original target except observation breakpoint; exact MSC_CMD_FLAGS-only child environment/canonical cwd; stop at main before source precision call or application logic. No CRT application coverage credit.')
(out/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(receipt)
