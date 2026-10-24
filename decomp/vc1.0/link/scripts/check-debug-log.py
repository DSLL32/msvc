#!/usr/bin/env python3
"""Cached diagnostic-log lifecycle: native ELF and captured references only."""
import argparse,hashlib,importlib.util,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-debug-log'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(item):
    p=Path(item['path']);assert not p.is_absolute()and'..'not in p.parts
    b=(CASE/p).read_bytes();assert len(b)==item['bytes']and sha(b)==item['sha256'];return b
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',action='store_true');a=p.parse_args();begin=time.perf_counter()
    m=json.loads((CASE/'manifest.json').read_text());cfg=json.loads((BINARY/'functions.json').read_text());r=json.loads(captured(m['original_receipt']))
    events=[json.loads(l)for l in captured(m['original_events']).decode().splitlines()]
    assert r['mode']=='original'and r['summary']==dict(event='summary',failed=0,stopped=1,exit=0,stdout_bytes=14,stderr_bytes=44)
    assert r['original_sha256']==cfg['target']['sha256']==m['original_sha256']and m['normalization']==[]
    entries=list(dict.fromkeys(e['name']for e in events if e.get('event')=='entry'and e.get('name')in m['expected_application_entries']))
    assert entries==m['expected_application_entries']==['open_debug_log','set_debug_flag']
    accepted=cfg['accepted'][m['prefix_accepted_count']:m['accepted_last']]
    assert [n for n in accepted if n in ['_open_debug_log','_set_debug_flag']]==['_open_debug_log','_set_debug_flag']
    payload=captured(m['inputs'][0]);assert len(payload)==256
    report=captured(m['external_report']);assert len(report)==1384 and report[:12]==struct.pack('<3I',0x31474c44,1,9)and report[12:268]==payload
    for phase in range(9):
        row=report[268+phase*124:268+(phase+1)*124];assert len(row)==124
        assert struct.unpack_from('<2I',row)==(phase,[0,0,6,1,0,7,0,0,6][phase])
        flags=bytearray(100)
        if 0<=phase<7:flags[1]=1
        if phase in[4,5]:flags[2]=1
        assert row[8:108]==flags
        assert struct.unpack_from('<I',row,108)[0]==(0xffffffff if phase<2 else 3)
        assert struct.unpack_from('<3I',row,112)==[(0,0,0),(2,3,3),(2,3,0),(2,3,0),(2,3,0),(2,3,0),(2,3,0),(2,3,0),(2,3,0)][phase]
    output=captured(m['stdout']);error=captured(m['stderr']);assert output==b'first\r\nfinal\r\n'and error==b'missing/log.txt: No such file or directory\r\n'
    refs={Path(x['path']).name:captured(x)for x in m['files']};assert refs=={'db.log':b'first\r\n','second.log':b'second\r\n'}
    d=Path(tempfile.mkdtemp(prefix='debug-log-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,m['cwd']);assert not list(work.iterdir());(d/'names.bin').write_bytes(payload)
    env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build'/('native-trace'if a.trace else'native')/'debug-log';assert exe.read_bytes()[:5]==b'\x7fELF\x01';stream=None;fds=();prepare=0
    if a.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('tr',BINARY/'scripts/native-traces.py');tr=importlib.util.module_from_spec(spec);spec.loader.exec_module(tr);cache=tr.load(BINARY/'build/native-trace');app=cache['application_definitions']
        seen=[name for name in cache['accepted_application']if cfg['accepted'].index(app[name]['logical'])<m['prefix_accepted_count']];assert not set(seen)&set(entries)
        (d/'filter.txt').write_text(''.join(n+'\n'for n in sorted(app)));(d/'seen.txt').write_text(''.join(n+'\n'for n in seen));stream=(d/'trace.txt').open('wb');fds=(stream.fileno(),)
        env.update(RECON_TRACE_FD=str(stream.fileno()),RECON_TRACE_FILTER=str(d/'filter.txt'),RECON_TRACE_SEEN=str(d/'seen.txt'));prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin;t=time.perf_counter()
    try:
        # The actual original/debugger capture redirects to regular files.
        # perror's reviewed lowio contract deliberately does not admit pipes.
        with(d/'stdout.bin').open('wb')as out,(d/'stderr.bin').open('wb')as err:
            run=subprocess.run([str(exe),str(d/'names.bin'),str(d/'results.bin')],cwd=work,env=env,stdout=out,stderr=err,pass_fds=fds,timeout=5)
    finally:
        if stream:stream.close()
    execution=time.perf_counter()-t;t=time.perf_counter();actual_output=(d/'stdout.bin').read_bytes();actual_error=(d/'stderr.bin').read_bytes()
    (d/'execution.json').write_text(json.dumps(dict(exit=run.returncode,stdout_sha256=sha(actual_output),stderr_sha256=sha(actual_error),redirection='regular files, matching original'),indent=2)+'\n')
    assert run.returncode==0 and actual_output==output and actual_error==error
    assert(d/'results.bin').read_bytes()==report and(d/'names.bin').read_bytes()==payload
    assert sorted(p.name for p in work.iterdir())==sorted(refs)
    for name,data in refs.items():assert(work/name).read_bytes()==data,name
    oracle=time.perf_counter()-t;t=time.perf_counter();actual=None
    if a.trace:actual=(d/'trace.txt').read_text().splitlines();assert actual==entries,actual
    traced=time.perf_counter()-t;row=dict(status='pass',case='direct-debug-log',trace=a.trace,entries=actual,exit=run.returncode,normalization=[],timing=dict(setup_seconds=setup,trace_prepare_seconds=prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=traced,total_seconds=time.perf_counter()-begin),elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),scope='Captured references only; exact100B flag transitions and all other98bytes, real log replacement/close, stdout/stderr, complete two-log inventory, errors/results/report/input immutability. No Wine/debugger/compiler/original refresh.')
    (d/'receipt.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(dict(status='pass',case=row['case'],trace=a.trace,receipt=str(d/'receipt.json'),seconds=row['timing']['total_seconds'])))
if __name__=='__main__':main()
