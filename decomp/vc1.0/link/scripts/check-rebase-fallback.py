#!/usr/bin/env python3
"""Cached direct relocation lifecycle; native ELF/captured references only."""
import argparse,hashlib,importlib.util,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-rebase-fallback'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(item):
    p=Path(item['path']);assert not p.is_absolute()and'..'not in p.parts
    b=(CASE/p).read_bytes();assert len(b)==item['bytes']and sha(b)==item['sha256'];return b
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',action='store_true');a=p.parse_args();begin=time.perf_counter()
    m=json.loads((CASE/'manifest.json').read_text());cfg=json.loads((BINARY/'functions.json').read_text());r=json.loads(captured(m['original_receipt']))
    assert r['mode']=='original'and r['summary']==dict(event='summary',failed=0,stopped=1,exit=0,stdout_bytes=0,stderr_bytes=0)and r['files']==[]
    assert r['original_sha256']==cfg['target']['sha256']==m['original_sha256']and m['normalization']==[]
    events=[json.loads(l)for l in captured(m['original_events']).decode().splitlines()]
    entries=list(dict.fromkeys(e['name']for e in events if e.get('event')=='entry'))
    assert entries==m['expected_application_entries']==['fallback_process_relocations']
    assert cfg['accepted'][m['prefix_accepted_count']:m['accepted_last']]==['_fallback_process_relocations@16']
    payload=captured(m['inputs'][0]);report=captured(m['external_report'])
    spec=importlib.util.spec_from_file_location('oracle',BINARY/'scripts/rebase-fallback-report.py');oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle);oracle.check(report,payload)
    output=captured(m['stdout']);error=captured(m['stderr']);assert output==error==b''
    d=Path(tempfile.mkdtemp(prefix='rebase-fallback-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,m['cwd']);assert not list(work.iterdir());(d/'payload.bin').write_bytes(payload)
    env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build'/('native-trace'if a.trace else'native')/'rebase-fallback';assert exe.read_bytes()[:5]==b'\x7fELF\x01';stream=None;fds=();prepare=0
    if a.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('tr',BINARY/'scripts/native-traces.py');tr=importlib.util.module_from_spec(spec);spec.loader.exec_module(tr);cache=tr.load(BINARY/'build/native-trace');app=cache['application_definitions']
        seen=[name for name in cache['accepted_application']if cfg['accepted'].index(app[name]['logical'])<m['prefix_accepted_count']];assert not set(seen)&set(entries)
        (d/'filter.txt').write_text(''.join(n+'\n'for n in sorted(app)));(d/'seen.txt').write_text(''.join(n+'\n'for n in seen));stream=(d/'trace.txt').open('wb');fds=(stream.fileno(),)
        env.update(RECON_TRACE_FD=str(stream.fileno()),RECON_TRACE_FILTER=str(d/'filter.txt'),RECON_TRACE_SEEN=str(d/'seen.txt'));prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin;t=time.perf_counter()
    try:
        with(d/'stdout.bin').open('wb')as out,(d/'stderr.bin').open('wb')as err:
            run=subprocess.run([str(exe),str(d/'payload.bin'),str(d/'results.bin')],cwd=work,env=env,stdout=out,stderr=err,pass_fds=fds,timeout=5)
    finally:
        if stream:stream.close()
    execution=time.perf_counter()-t;t=time.perf_counter()
    assert run.returncode==0 and(d/'stdout.bin').read_bytes()==output and(d/'stderr.bin').read_bytes()==error
    actual=(d/'results.bin').read_bytes();oracle.check(actual,payload);assert actual==report and(d/'payload.bin').read_bytes()==payload and not list(work.iterdir())
    checked=time.perf_counter()-t;t=time.perf_counter();actual_trace=None
    if a.trace:actual_trace=(d/'trace.txt').read_text().splitlines();assert actual_trace==entries,actual_trace
    traced=time.perf_counter()-t;row=dict(status='pass',case='direct-rebase-fallback',trace=a.trace,entries=actual_trace,exit=run.returncode,normalization=[],timing=dict(setup_seconds=setup,trace_prepare_seconds=prepare,execution_seconds=execution,oracle_seconds=checked,trace_seconds=traced,total_seconds=time.perf_counter()-begin),elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),scope='Captured-only four cumulative phases; count0/all6types plus HIGHADJ pair/negative delta/type6NULL, complete page+records+guards+errors/input and no target files. No Wine/GDB/reference refresh.')
    (d/'receipt.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(dict(status='pass',case=row['case'],trace=a.trace,receipt=str(d/'receipt.json'),seconds=row['timing']['total_seconds'])))
if __name__=='__main__':main()
