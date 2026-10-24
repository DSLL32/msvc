#!/usr/bin/env python3
"""Cached typed priority-list setup and genuine dump continuation; native ELF only."""
import argparse,hashlib,importlib.util,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-priority-archives'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(item):
    p=Path(item['path']);assert not p.is_absolute()and'..'not in p.parts;b=(CASE/p).read_bytes();assert len(b)==item['bytes']and sha(b)==item['sha256'];return b
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',action='store_true');a=p.parse_args();begin=time.perf_counter();m=json.loads((CASE/'manifest.json').read_text());cfg=json.loads((BINARY/'functions.json').read_text());r=json.loads(captured(m['original_receipt']));events=[json.loads(l)for l in captured(m['original_events']).decode().splitlines()]
    assert r['mode']=='original'and r['summary']==dict(event='summary',failed=0,stopped=1,exit=0,stdout_bytes=36586,stderr_bytes=0)
    assert r['original_sha256']==cfg['target']['sha256']==m['original_sha256']and m['normalization']==[]
    entries=list(dict.fromkeys('insert_other_input'for e in events if e.get('event')=='entry'and e.get('target')==0x20a10));assert entries==m['expected_application_entries']==['insert_other_input']
    assert cfg['accepted'][m['prefix_accepted_count']:m['accepted_last']]==['_insert_other_input']
    payload=captured(m['inputs'][-1]);assert len(payload)==256
    report=struct.pack('<8I',0x31495250,1,0,4,1,2,0,3)+payload
    for i,next_node in enumerate([2,3,0xffffffff,0]):report+=struct.pack('<4I',i,i*32,i*32,next_node)
    assert report==captured(m['external_report'])and len(report)==352
    output=captured(m['stdout']);positions=[output.index(n.encode())for n in m['expected_order']];assert positions==sorted(positions)
    d=Path(tempfile.mkdtemp(prefix='priority-archives-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,m['cwd']);refs={Path(x['path']).name:captured(x)for x in m['files']}
    for item in m['inputs'][:-1]:
        name=Path(item['path']).name;data=captured(item);assert data==refs[name];(work/name).write_bytes(data)
    (d/'names.bin').write_bytes(payload);env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build'/('native-trace'if a.trace else'native')/'priority-archives';assert exe.read_bytes()[:5]==b'\x7fELF\x01';stream=None;fds=();prepare=0
    if a.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('tr',BINARY/'scripts/native-traces.py');tr=importlib.util.module_from_spec(spec);spec.loader.exec_module(tr);cache=tr.load(BINARY/'build/native-trace');app=cache['application_definitions'];seen=[name for name in cache['accepted_application']if cfg['accepted'].index(app[name]['logical'])<m['prefix_accepted_count']];assert not set(seen)&set(entries)
        (d/'filter.txt').write_text(''.join(n+'\n'for n in sorted(app)));(d/'seen.txt').write_text(''.join(n+'\n'for n in seen));stream=(d/'trace.txt').open('wb');fds=(stream.fileno(),);env.update(RECON_TRACE_FD=str(stream.fileno()),RECON_TRACE_FILTER=str(d/'filter.txt'),RECON_TRACE_SEEN=str(d/'seen.txt'));prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin;t=time.perf_counter()
    try:run=subprocess.run([str(exe),str(d/'names.bin'),str(d/'results.bin'),*m['target_argv']],cwd=work,env=env,capture_output=True,pass_fds=fds,timeout=5)
    finally:
        if stream:stream.close()
    execution=time.perf_counter()-t;t=time.perf_counter();assert run.returncode==0 and run.stdout==output and run.stderr==captured(m['stderr'])==b'';assert(d/'results.bin').read_bytes()==report
    assert sorted(p.name for p in work.iterdir())==sorted(refs)
    for name,data in refs.items():assert(work/name).read_bytes()==data,name
    assert(d/'names.bin').read_bytes()==payload;oracle=time.perf_counter()-t;t=time.perf_counter();actual=None
    if a.trace:actual=(d/'trace.txt').read_text().splitlines();assert actual==entries,actual
    traced=time.perf_counter()-t;row=dict(status='pass',case='direct-priority-archives',trace=a.trace,entries=actual,exit=run.returncode,normalization=[],timing=dict(setup_seconds=setup,trace_prepare_seconds=prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=traced,total_seconds=time.perf_counter()-begin),elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),scope='Capturedreferences only; fourdefinedownerlinks/count/cursors externalreport, actualpublicdumporder/fullstdout, all4archives+names unchanged, status/trace; noWine/debugger/compiler/originalrefresh.')
    (d/'receipt.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(dict(status='pass',case=row['case'],trace=a.trace,receipt=str(d/'receipt.json'),seconds=row['timing']['total_seconds'])))
if __name__=='__main__':main()
