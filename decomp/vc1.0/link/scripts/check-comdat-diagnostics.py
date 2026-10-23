#!/usr/bin/env python3
"""Cached-only native COMDAT diagnostic fixture, no originals or debugger."""
import argparse,hashlib,importlib.util,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from case_paths import ROOT,SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/comdat-graph-diagnostics'
sha=lambda b:hashlib.sha256(b).hexdigest()
def captured(record):
    data=(CASE/record['path']).read_bytes();assert sha(data)==record['sha256']
    if 'bytes' in record:assert len(data)==record['bytes']
    return data

def validate_trace(actual,expected):
    assert expected==['dump_comdat_graph','debug_printf','print_comdat_node','comdat_usage_label','print_comdat_edge'] and actual==expected,'mandatory COMDAT diagnostic first-entry sequence'

def validate_stdout(data):
    expected=(b'Dump of comdat dependency graph for TCE\r\n---------------------------------------\r\n'
              b'PNOD|R|sec=.textXYZ|grp=.text$gr|con=0x12000100|mod=graph.obj|entry()\r\n'
              b'    PEDG|N|sec=.textXYZ|grp=.text$gr|con=0x12000140|mod=graph.obj\r\n\r\n'
              b'PNOD|N|sec=.textXYZ|grp=.text$gr|con=0x12000140|mod=graph.obj\r\n\r\n')
    assert data==expected,'full ordered raw diagnostic, including defined pointer inputs'

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--trace',action='store_true');args=ap.parse_args();begin=time.perf_counter()
    metadata=json.loads((CASE/'manifest.json').read_text());config=json.loads((BINARY/'functions.json').read_text());assert metadata['original_sha256']==config['target']['sha256']
    receipt=json.loads(captured(metadata['original_receipt']));assert receipt['mode']=='masked' and receipt['trial']=='print_comdat_edge'
    count=receipt['accepted_count'];assert count==metadata['last']-1 and config['accepted'][count]=='_print_comdat_edge'
    assert receipt['accepted_sha256']==sha(json.dumps(config['accepted'][:count]).encode())
    events=[json.loads(x) for x in captured(metadata['original_events']).decode().splitlines()]
    assert events[0]==dict(event='owned_mapping',requested=0x12000000,returned=0x12000000,size=4096)
    proof=[x for x in events if x['event']=='owned_mapping_proof'];assert proof==[dict(event='owned_mapping_proof',before_state=0x10000,after_state=0x1000,allocation_base=0x12000000,base=0x12000000,size=4096,protect=4,type=0x20000)]
    assert receipt['summary']==dict(event='summary',failed=0,stopped=1,exit=0,stdout_bytes=287,stderr_bytes=0)
    names={r['start']:r['name'][1:] for r in config['driver']};actual=list(dict.fromkeys(names[x['eip']] for x in events if x['event']=='entry'))
    validate_trace(actual,metadata['expected_application_entries'])
    assert config['accepted'][metadata['first']:metadata['last']]==['_'+n for n in actual]
    payload=captured(metadata['input']);assert len(payload)==4096
    stdout=captured(metadata['stdout']);validate_stdout(stdout);stderr=captured(metadata['stderr']);assert not stderr
    for x in events:
        if x['event'] in ['entry','return']:assert x['input_base']==0x12000000 and bytes.fromhex(x['region'])==payload
    exe=BINARY/'build'/('native-trace' if args.trace else 'native')/'comdat-diagnostics';assert exe.read_bytes()[:5]==b'\x7fELF\x01'
    directory=Path(tempfile.mkdtemp(prefix='comdat-cached-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(directory,r'S:\vc1.0\link\comdat-graph-diagnostics')
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    fd=None;pass_fds=();trace_prepare=0
    if args.trace:
        t=time.perf_counter();spec=importlib.util.spec_from_file_location('comdat_trace_names',BINARY/'scripts/native-traces.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);cache=module.load(BINARY/'build/native-trace');app=cache['application_definitions']
        expected=metadata['expected_application_entries'];assert all(app[n]['logical']=='_'+n for n in expected)
        seen=[n for n in cache['accepted_application'] if config['accepted'].index(app[n]['logical'])<metadata['first']];assert not set(seen)&set(expected)
        (directory/'filter.txt').write_text(''.join(n+'\n' for n in sorted(app)));(directory/'seen.txt').write_text(''.join(n+'\n' for n in seen))
        fd=(directory/'trace.txt').open('wb');pass_fds=(fd.fileno(),);env.update(RECON_TRACE_FD=str(fd.fileno()),RECON_TRACE_FILTER=str(directory/'filter.txt'),RECON_TRACE_SEEN=str(directory/'seen.txt'));trace_prepare=time.perf_counter()-t
    setup=time.perf_counter()-begin;t=time.perf_counter();result=subprocess.run([str(exe),str(CASE/'inputs/graph.bin')],cwd=work,env=env,pass_fds=pass_fds,capture_output=True,timeout=5);execution=time.perf_counter()-t
    if fd:fd.close()
    t=time.perf_counter();assert result.returncode==0 and result.stdout==stdout and result.stderr==stderr;validate_stdout(result.stdout)
    assert sorted(str(p.relative_to(work)) for p in work.rglob('*') if p.is_file())==metadata['expected_target_files'];oracle=time.perf_counter()-t
    t=time.perf_counter();entries=None
    if args.trace:entries=(directory/'trace.txt').read_text().splitlines();validate_trace(entries,metadata['expected_application_entries'])
    trace=time.perf_counter()-t
    result=dict(status='pass',case='comdat-graph-diagnostics',trace=args.trace,entries=entries,stdout_bytes=len(stdout),stderr_bytes=0,exit=0,input_sha256=sha(payload),elf_sha256=sha(exe.read_bytes()),manifest_sha256=sha((CASE/'manifest.json').read_bytes()),timing=dict(setup_seconds=setup,trace_prepare_seconds=trace_prepare,execution_seconds=execution,oracle_seconds=oracle,trace_seconds=trace,total_seconds=time.perf_counter()-begin),scope='Explicit external direct test; exact controlled caller addresses, no normalization, no Wine/helper/debugger')
    (directory/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='pass',trace=args.trace,receipt=str(directory/'receipt.json'),seconds=result['timing']['total_seconds'])))
if __name__=='__main__':main()
