#!/usr/bin/env python3
"""Cached-only native Win16 callback tests. No original/debugger execution."""
import argparse,hashlib,importlib.util,json,os,subprocess,tempfile,time
from pathlib import Path
from case_paths import ROOT,SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1]
CASE=BINARY/'cases/legacy-resource-diagnostics'
sha=lambda data:hashlib.sha256(data).hexdigest()
def captured(path,record):
    data=path.read_bytes()
    assert len(data)==record['bytes'] and sha(data)==record['sha256'],str(path)+': frozen bytes differ'
    return data

def validate_case(work,result,item,stdout,stderr,input_data):
    assert result.returncode==item['expected_driver_exit'],('driver exit',result.returncode)
    assert result.stdout==stdout and result.stderr==stderr,'callback output differs'
    assert sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file())==item['expected_target_created_files'],'callback file effects differ'
    assert input_data==captured(CASE/item['input']['path'],item['input']),'callback input changed'

def validate_trace(actual,item):
    assert item['expected_application_entries']==['display_legacy_'+item['name']] and actual,'mandatory nonempty callback identity'
    assert actual==item['expected_application_entries'],('unexpected callback entry trace',actual,item['expected_application_entries'])

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--trace',action='store_true')
    options=ap.parse_args()
    suite_start=time.perf_counter()
    variant='native-trace' if options.trace else 'native'
    executable=BINARY/'build'/variant/'legacy-diagnostics'
    assert executable.read_bytes()[:5]==b'\x7fELF\x01','native i386 executable required'
    config=json.loads((BINARY/'functions.json').read_text())
    manifest=json.loads((CASE/'manifest.json').read_text())
    assert manifest['original_sha256']==config['target']['sha256']
    assert sha((CASE/'analysis/provenance.json').read_bytes())==manifest['provenance_sha256']
    prepare_start=time.perf_counter()
    spec=importlib.util.spec_from_file_location('legacy_trace_names',BINARY/'scripts/native-traces.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    cache=module.load(BINARY/'build/native-trace') if options.trace else None
    trace_prepare=time.perf_counter()-prepare_start
    directory=Path(tempfile.mkdtemp(prefix='legacy-cached-',dir=SCRATCH/'vc1.0/link'))
    results=[]
    for item in manifest['cases']:
        begin=time.perf_counter();name=item['name'];case_directory=directory/name
        work,drives=native_work(case_directory,r'S:\vc1.0\link\legacy-resource-diagnostics')
        stdout=captured(CASE/item['stdout']['path'],item['stdout']);stderr=captured(CASE/item['stderr']['path'],item['stderr'])
        input_data=captured(CASE/item['input']['path'],item['input'])
        entry=CASE/'acceptance'/name/'original-entry'
        assert sha((entry/'events.jsonl').read_bytes())==item['original_entry_sha256']
        assert sha((entry/'receipt.json').read_bytes())==item['original_entry_receipt_sha256']
        original=json.loads((entry/'receipt.json').read_text());prefix=item['accepted_prefix']
        assert original['accepted_count']==prefix
        assert original['accepted_sha256']==sha(json.dumps(config['accepted'][:prefix]).encode())
        assert config['accepted'][prefix]==item['logical']
        env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
        env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
        pass_fds=();trace_fd=None
        if options.trace:
            app=cache['application_definitions'];assert app[item['expected_application_entries'][0]]['logical']==item['logical']
            seen=[n for n in cache['accepted_application'] if config['accepted'].index(app[n]['logical'])<prefix]
            assert not set(item['expected_application_entries'])&set(seen)
            filter_path=case_directory/'filter.txt';seen_path=case_directory/'seen.txt'
            filter_path.write_text(''.join(n+'\n' for n in sorted(app)));seen_path.write_text(''.join(n+'\n' for n in seen))
            trace_fd=(case_directory/'trace.txt').open('wb')
            env.update(RECON_TRACE_FD=str(trace_fd.fileno()),RECON_TRACE_FILTER=str(filter_path),RECON_TRACE_SEEN=str(seen_path))
            pass_fds=(trace_fd.fileno(),)
        setup=time.perf_counter()-begin
        execute_start=time.perf_counter()
        result=subprocess.run([str(executable),str(item['type']),str(CASE/item['input']['path'])],cwd=work,env=env,pass_fds=pass_fds,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
        execution=time.perf_counter()-execute_start
        if trace_fd:trace_fd.close()
        oracle_start=time.perf_counter();validate_case(work,result,item,stdout,stderr,input_data);oracle=time.perf_counter()-oracle_start
        trace_start=time.perf_counter();entries=None
        if options.trace:
            entries=(case_directory/'trace.txt').read_text().splitlines();validate_trace(entries,item)
        trace=time.perf_counter()-trace_start
        results.append(dict(case=name,status='pass',scope='external direct code test; not public CLI',entries=entries,stdout_bytes=len(result.stdout),stderr_bytes=len(result.stderr),exit=result.returncode,timing=dict(setup_seconds=setup,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-begin)))
    receipt=dict(status='pass',cases=results,case_count=len(results),trace=options.trace,trace_prepare_seconds=trace_prepare,internal_suite_seconds=time.perf_counter()-suite_start,receipt_directory=str(directory),elf_sha256=sha(executable.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()))
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status='pass',cases=len(results),trace=options.trace,seconds=receipt['internal_suite_seconds'],receipt=str(directory/'receipt.json'))))
if __name__=='__main__':main()
