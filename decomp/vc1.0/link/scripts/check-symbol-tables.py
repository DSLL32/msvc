#!/usr/bin/env python3
"""Cached custom-symbol workflow. Native ELF only; no original or debugger."""
import argparse,hashlib,importlib.util,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-symbol-tables'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(item):
    p=Path(item['path']);assert not p.is_absolute() and '..' not in p.parts
    raw=(CASE/p).read_bytes();assert len(raw)==item['bytes'] and sha(raw)==item['sha256'];return raw
def report(raw):
    assert struct.unpack_from('<4I',raw)==(0x314d5953,1,4,0);p=16;tables=[]
    for table in range(4):
        count=struct.unpack_from('<I',raw,p)[0];p+=4;entries=[]
        for i in range(count):
            flags,short,length=struct.unpack_from('<3I',raw,p);p+=12;name=raw[p:p+length];p+=length
            assert 1<length<=64 and name[-1:]==b'\0' and b'\0' not in name[:-1] and short in [0,1]
            entries.append((flags,short,name))
        tables.append(entries)
    assert p==len(raw) and [len(t) for t in tables]==[32,21,0,0]
    assert {n for f,s,n in tables[1]}=={n for f,s,n in tables[0] if f&1}
    assert all(f==0 for f,s,n in tables[1]);return tables

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',action='store_true');p.add_argument('--manifest',type=Path,default=CASE/'manifest.json');a=p.parse_args();begin=time.perf_counter()
    m=json.loads(a.manifest.read_text());config=json.loads((BINARY/'functions.json').read_text());r=json.loads(captured(m['original_receipt']))
    assert r['original_sha256']==m['original_sha256']==config['target']['sha256'] and r['mode']=='original'
    assert r['summary']==dict(event='summary',failed=0,stopped=1,exit=0,phase=23,stdout_bytes=1274,stderr_bytes=0,counts=[32,21,0,0]) and r['files']==[]
    events=[json.loads(line) for line in captured(m['original_events']).decode().splitlines()]
    roots={0x26510:'print_external_table',0x2dcf0:'copy_defined_names'}
    original_entries=list(dict.fromkeys(roots[e['target']] for e in events if e.get('event')=='entry'))
    assert original_entries==m['expected_application_entries'] and len([e for e in events if e.get('target') in roots])==12
    data=captured(m['input']);stdout=captured(m['stdout']);stderr=captured(m['stderr']);expected=captured(m['external_report']);report(expected)
    assert m['normalization']==[] and m['expected_application_entries']==[n[1:] for n in config['accepted'][m['prefix_accepted_count']:m['accepted_last']]]
    exe=BINARY/'build'/('native-trace' if a.trace else 'native')/'symbol-tables';assert exe.read_bytes()[:5]==b'\x7fELF\x01'
    d=Path(tempfile.mkdtemp(prefix='symbol-tables-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,r['canonical_cwd'])
    (d/'names.bin').write_bytes(data)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    stream=None;pass_fds=();prepare=0
    if a.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('symbol_trace',BINARY/'scripts/native-traces.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);cache=module.load(BINARY/'build/native-trace');app=cache['application_definitions'];first=m['prefix_accepted_count']
        seen=[n for n in cache['accepted_application'] if config['accepted'].index(app[n]['logical'])<first]
        assert not set(seen)&set(m['expected_application_entries'])
        (d/'filter.txt').write_text(''.join(n+'\n' for n in sorted(app)));(d/'seen.txt').write_text(''.join(n+'\n' for n in seen));stream=(d/'trace.txt').open('wb');pass_fds=(stream.fileno(),)
        env.update(RECON_TRACE_FD=str(stream.fileno()),RECON_TRACE_FILTER=str(d/'filter.txt'),RECON_TRACE_SEEN=str(d/'seen.txt'));prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin;t=time.perf_counter()
    try:result=subprocess.run([str(exe),str(d/'names.bin')],cwd=work,env=env,pass_fds=pass_fds,capture_output=True,timeout=5)
    finally:
        if stream:stream.close()
    execution=time.perf_counter()-t;t=time.perf_counter();assert result.returncode==0 and result.stdout==stdout and result.stderr==stderr
    assert (d/'names.bin').read_bytes()==data
    inventory=sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file());assert inventory==['tables.bin']
    actual=(work/'tables.bin').read_bytes();assert actual==expected;report(actual)
    oracle=time.perf_counter()-t;t=time.perf_counter();entries=None
    if a.trace:entries=(d/'trace.txt').read_text().splitlines();assert entries==m['expected_application_entries'],entries
    trace=time.perf_counter()-t;receipt=dict(status='pass',case='direct-symbol-tables',reach=m['reach'],trace=a.trace,entries=entries,exit=result.returncode,inventory=inventory,stdout_bytes=len(result.stdout),stderr_bytes=len(result.stderr),elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha(a.manifest.read_bytes()),timing=dict(setup_seconds=setup,trace_prepare_seconds=prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-begin),scope='Native ELF only, cached original inputs/fulloutputs exclusively, zero normalization; driver report external to target graph. Startup53 before application entry.')
    (d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='pass',case='direct-symbol-tables',trace=a.trace,receipt=str(d/'receipt.json'),seconds=receipt['timing']['total_seconds'])))
if __name__=='__main__':main()
