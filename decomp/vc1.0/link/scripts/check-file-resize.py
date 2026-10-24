#!/usr/bin/env python3
"""Cached direct file resize fixtures. Native ELF only; no original or debugger."""
import argparse,hashlib,importlib.util,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-file-resize'
sha=lambda data:hashlib.sha256(data).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--trace',action='store_true');p.add_argument('--phase',choices=['mapped','buffered']);a=p.parse_args();manifest=json.loads((CASE/'manifest.json').read_text());config=json.loads((BINARY/'functions.json').read_text())
    exe=BINARY/'build'/('native-trace' if a.trace else 'native')/'file-resize';assert exe.read_bytes()[:5]==b'\x7fELF\x01';rows=[]
    for phase,m in manifest['phases'].items():
        if a.phase and a.phase!=phase:continue
        begin=time.perf_counter();ref=CASE/m['reference'];receipt=json.loads((ref/'receipt.json').read_text());assert receipt['original_sha256']==manifest['original_sha256']==config['target']['sha256'];assert receipt['mode']=='original' and receipt['phase']==phase and receipt['summary']['exit']==0
        expected={}
        for item in m['files']:
            raw=(ref/'files'/item['path']).read_bytes();assert len(raw)==item['bytes'] and sha(raw)==item['sha256'];expected[item['path']]=raw
        item=m['external_report'];raw=(CASE/item['path']).read_bytes();assert len(raw)==item['bytes']==60 and sha(raw)==item['sha256'];expected['results.bin']=raw
        stdout=(ref/'stdout.bin').read_bytes();stderr=(ref/'stderr.bin').read_bytes();assert stdout==stderr==b''
        d=Path(tempfile.mkdtemp(prefix='file-resize-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,'S:\\vc1.0\\link\\direct-file-resize-'+phase)
        env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(key,None)
        fd=None;pass_fds=();prepare=0
        if a.trace:
            t=time.perf_counter();spec=importlib.util.spec_from_file_location('resize_trace',BINARY/'scripts/native-traces.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);cache=module.load(BINARY/'build/native-trace');app=cache['application_definitions'];first=m['prefix_accepted_count']
            for name in m['expected_application_entries']:assert app[name]['logical']=='_'+name
            seen=[name for name in cache['accepted_application'] if config['accepted'].index(app[name]['logical'])<first]
            assert not set(seen)&set(m['expected_application_entries']);(d/'filter.txt').write_text(''.join(n+'\n' for n in sorted(app)));(d/'seen.txt').write_text(''.join(n+'\n' for n in seen));fd=(d/'trace.txt').open('wb');pass_fds=(fd.fileno(),);env.update(RECON_TRACE_FD=str(fd.fileno()),RECON_TRACE_FILTER=str(d/'filter.txt'),RECON_TRACE_SEEN=str(d/'seen.txt'));prepare=time.perf_counter()-t
        setup=time.perf_counter()-begin;t=time.perf_counter();r=subprocess.run([str(exe),phase],cwd=work,env=env,pass_fds=pass_fds,capture_output=True,timeout=5);execution=time.perf_counter()-t
        if fd:fd.close()
        t=time.perf_counter();assert r.returncode==0 and r.stdout==stdout and r.stderr==stderr,(r.returncode,r.stdout,r.stderr)
        inventory=sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file());assert inventory==sorted(expected),inventory
        for name,data in expected.items():assert (work/name).read_bytes()==data,('nonidentical file',name)
        oracle=time.perf_counter()-t;t=time.perf_counter();entries=None
        if a.trace:entries=(d/'trace.txt').read_text().splitlines();assert entries==m['expected_application_entries'],entries
        trace=time.perf_counter()-t;row=dict(status='pass',case='direct-file-resize-'+phase,reach='External direct support; not public CLI',trace=a.trace,entries=entries,exit=r.returncode,inventory=inventory,elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),startup='RECON_NATIVE_STARTUP_CONTROLFP_53, constructor103 before app entry',timing=dict(setup_seconds=setup,trace_prepare_seconds=prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-begin),scope='Captured references exclusively, zero normalization. Native ELF only, no Wine/debugger/original/response recapture. results.bin is external driver report, not original target artifact.')
        (d/'receipt.json').write_text(json.dumps(row,indent=2)+'\n');rows.append(row);print(json.dumps(dict(status='pass',phase=phase,trace=a.trace,receipt=str(d/'receipt.json'),seconds=row['timing']['total_seconds'])))
if __name__=='__main__':main()
