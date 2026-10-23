#!/usr/bin/env python3
"""Cached dispatcher fixtures. Only a native ELF runs; no capture/debugger/PE."""
import argparse,hashlib,importlib.util,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/dispatcher-assistance'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(record):
    data=(CASE/record['path']).read_bytes();assert len(data)==record['bytes'] and sha(data)==record['sha256'];return data

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--trace',action='store_true');ap.add_argument('--phase',choices=['helper','direct']);args=ap.parse_args();metadata=json.loads((CASE/'manifest.json').read_text());config=json.loads((BINARY/'functions.json').read_text());rows=[]
    exe=BINARY/'build'/('native-trace' if args.trace else 'native')/'dispatcher';assert exe.read_bytes()[:5]==b'\x7fELF\x01'
    for phase in ['helper','direct']:
        if args.phase and phase!=args.phase:continue
        begin=time.perf_counter();m=metadata['phases'][phase];logical=m['expected_application_entries'][0]
        reference=json.loads(captured(m['original_receipt']));assert reference['mode']=='masked' and reference['trial']==logical and reference['phase']==phase
        assert reference['accepted_count']==m['first'] and reference['accepted_sha256']==sha(json.dumps(config['accepted'][:m['first']]).encode())
        assert config['accepted'][m['first']]==('_'+logical) and config['target']['sha256']==reference['original_sha256']==metadata['original_sha256']
        events=[json.loads(x) for x in captured(m['original_events']).decode().splitlines()];assert [e['name'] for e in events if e['event']=='entry']==[logical]
        assert events[-1]==dict(event='summary',failed=0,stopped=1,exit=3,stdout_bytes=m['stdout']['bytes'],stderr_bytes=0)
        stdout=captured(m['stdout']);stderr=captured(m['stderr']);assert not stderr
        directory=Path(tempfile.mkdtemp(prefix='dispatcher-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(directory,'S:\\vc1.0\\link\\dispatcher-assistance-'+phase)
        env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
        for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(key,None)
        fd=None;pass_fds=();trace_prepare=0
        if args.trace:
            t=time.perf_counter();spec=importlib.util.spec_from_file_location('dispatcher_trace_names',BINARY/'scripts/native-traces.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);cache=module.load(BINARY/'build/native-trace');app=cache['application_definitions']
            assert app[logical]['logical']=='_'+logical
            seen=[n for n in cache['accepted_application'] if config['accepted'].index(app[n]['logical'])<m['first']];assert logical not in seen
            (directory/'filter.txt').write_text(''.join(n+'\n' for n in sorted(app)));(directory/'seen.txt').write_text(''.join(n+'\n' for n in seen));fd=(directory/'trace.txt').open('wb');pass_fds=(fd.fileno(),);env.update(RECON_TRACE_FD=str(fd.fileno()),RECON_TRACE_FILTER=str(directory/'filter.txt'),RECON_TRACE_SEEN=str(directory/'seen.txt'));trace_prepare=time.perf_counter()-t
        setup=time.perf_counter()-begin;t=time.perf_counter();result=subprocess.run([str(exe),'-helper' if phase=='helper' else '--direct'],cwd=work,env=env,pass_fds=pass_fds,capture_output=True,timeout=5);execution=time.perf_counter()-t
        if fd:fd.close()
        t=time.perf_counter();assert result.returncode==3 and result.stdout==stdout and result.stderr==stderr
        assert sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file())==[];oracle=time.perf_counter()-t
        t=time.perf_counter();entries=None
        if args.trace:entries=(directory/'trace.txt').read_text().splitlines();assert entries==m['expected_application_entries'],'unexpected/missing application entry'
        trace=time.perf_counter()-t
        row=dict(status='pass',case='dispatcher-assistance-'+phase,trace=args.trace,reach=m['reach'],entries=entries,stdout_bytes=len(stdout),stderr_bytes=0,exit=3,elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),timing=dict(setup_seconds=setup,trace_prepare_seconds=trace_prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-begin),scope='ELF and captured references only; public helper vs explicit direct usage caller distinguished')
        (directory/'receipt.json').write_text(json.dumps(row,indent=2)+'\n');rows.append(row);print(json.dumps(dict(status='pass',phase=phase,trace=args.trace,receipt=str(directory/'receipt.json'),seconds=row['timing']['total_seconds'])))
if __name__=='__main__':main()
