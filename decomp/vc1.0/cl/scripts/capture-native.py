#!/usr/bin/env python3
"""Explicit original-only child-boundary capture, outside native fixture runs.

Existing transport oracles are checked unless explicitly staging a path migration.
Normal native fixture execution never imports this original-only capture tool.
The debugger snapshots each synchronous _spawnvpe input/output tree and stream
slice. It executes the original CL and its original children only.
"""
import argparse
import fcntl
import gzip
import hashlib
import importlib.machinery
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time

HERE = Path(__file__).resolve().parent
cases = importlib.machinery.SourceFileLoader('native_cases',str(HERE/'native-cases.py')).load_module()
wt = importlib.machinery.SourceFileLoader('reference_wine',str(HERE/'wine-tool.py')).load_module()
STORE = cases.CASES/'native-boundaries'
SCRATCH = cases.SCRATCH/'vc1.0/cl'
artifacts = importlib.machinery.SourceFileLoader('cl_capture_artifact_analysis',str(HERE/'native-artifacts.py')).load_module()
traces = importlib.machinery.SourceFileLoader('cl_original_trace_plan',str(HERE/'native-traces.py')).load_module()


def blob(data):
    digest = cases.sha(data)
    destination = STORE/'blobs'/(digest+'.gz')
    destination.parent.mkdir(parents=True,exist_ok=True)
    if not destination.exists():
        destination.write_bytes(gzip.compress(data,mtime=0))
    return dict(sha256=digest,bytes=len(data))


def tree(path):
    return {str(p.relative_to(path)):blob(p.read_bytes()) for p in sorted(path.rglob('*')) if p.is_file()}


def request(path):
    data=path.read_bytes(); mode,explicit=struct.unpack_from('<II',data); fields=data[8:].split(b'\0')
    program=fields.pop(0).decode('latin1')
    def vector():
        result=[]
        while fields[0]:result.append(fields.pop(0).decode('latin1'))
        fields.pop(0)
        return result
    argv=vector(); environment=vector()
    assert fields==[b''],fields
    return dict(mode=mode,file=program,argv=argv,environment=environment if explicit else None)


def run(case, force, migrate_paths=False):
    case_id=case['id']; destination=STORE/(case_id+'.json')
    assert force or not destination.exists(), 'capture already exists: '+case_id
    target=(cases.TC/'bin/CL.EXE').read_bytes()
    assert cases.sha(target)==json.loads((cases.BINARY/'functions.json').read_text())['target']['sha256'], 'original CL image changed'
    start=time.perf_counter(); work=SCRATCH/case_id; capture=SCRATCH/(case_id+'.capture')
    SCRATCH.mkdir(parents=True,exist_ok=True)
    lease=(SCRATCH/(case_id+'.lock')).open('a')
    fcntl.flock(lease,fcntl.LOCK_EX)
    for path in (work,capture):
        if path.exists():shutil.rmtree(path)
        path.mkdir(parents=True)
    cases.prepare(case,work)
    initial=tree(work)
    assert (wt.PREFIX/'dosdevices/r:').resolve()==cases.ROOT
    assert (wt.PREFIX/'dosdevices/s:').resolve()==cases.SCRATCH
    substitutions=cases.substitutions(case_id)
    env=wt.environment();env.update(PATH=cases.windows(cases.TC/'bin'),TMP='.',TEMP='.')
    for key in ('CL','_CL_','_MSC_IDE_FLAGS','LINK','_LINK_','MSC_CMD_FLAGS'):env.pop(key,None)
    env.update({k:v.format(**substitutions) for k,v in case.get('environment',{}).items()})
    config=capture/'config.txt'
    functions={r['name']:r['start']-0x10000 for r in json.loads((cases.BINARY/'functions.json').read_text())['driver']}
    config.write_text('%x %x %x %x %x %x\n' % (functions['_control_handler@4'],0x140d4,functions['_link_pass'],0x16018,functions['_ide_message'],0x140d0)+'%u %u %u %u\n'%tuple(case.get('clock_calendar',[2026,1,15,0])))
    projection=traces.project(cases.BINARY/'build/native-trace',verify_capture=False)
    segment=projection['cases'][case_id]
    seen=set(projection['accepted_application'][:segment['seen_count']])
    plan={record['original_extent'][0]-0x10000:name for name,record in projection['application_definitions'].items() if name not in seen}
    with config.open('a') as file:
        file.write(str(len(plan))+'\n'+''.join('%x\n'%rva for rva in plan))
    original=cases.BINARY/'build/native-reference.exe'
    if not original.exists():original.write_bytes(target)
    assert original.read_bytes()==target, 'capture image changed'
    stdout,stderr,events=[capture/name for name in ('stdout.bin','stderr.bin','events.txt')]
    mode=case.get('mode') or ('clock' if case_id=='timing' else 'ordinary')
    command=['/usr/bin/wine',cases.windows(cases.BINARY/'build/capture-run.exe'),cases.windows(original),case['arguments'].format(**substitutions),*[cases.windows(p) for p in (stdout,stderr,events)],cases.canonical_cwd(case_id),cases.windows(config),mode,cases.windows(capture)]
    setup=time.perf_counter()-start;begin=time.perf_counter()
    subprocess.run(command,env=env,check=True,timeout=90,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    execution=time.perf_counter()-begin;begin=time.perf_counter()
    lines=events.read_text().splitlines()
    assert not any(line.startswith(('exception ','timeout ','dispatch-error')) for line in lines),events.read_text()
    observed=[plan[int(line.split()[1],16)] for line in lines if line.startswith('application-entry ')]
    assert observed==segment['expected'], 'original application entry progression changed: '+repr((observed,segment['expected']))
    if mode=='clock':assert [line for line in lines if line.startswith('clock-call ')]==['clock-call %08x'%i for i in range(1,5)]
    if mode in ('handler-idle','handler-child'):assert 'child-running %08x'%(mode=='handler-child') in lines
    if mode=='omf-state':assert 'internal-omf-state 00000000' in lines
    if mode=='ide-null-buffer':assert 'internal-ide-null-buffer 00000000' in lines
    expected=cases.oracles()[case_id]
    assert 'exit %08x'%case['expected_exit'] in events.read_text(),events.read_text()
    for name,path in [('stdout',stdout),('stderr',stderr)]:
        oracle=expected['streams'][name]
        digest=oracle if isinstance(oracle,str) else oracle['sha256']
        if not migrate_paths:assert cases.sha(path.read_bytes())==digest, 'existing stream oracle differs: '+case_id+' '+name
    boundaries=[]
    for path in sorted(capture.glob('*-request.bin')):
        stem=path.name[:3]; result=capture/(stem+'-result.txt')
        if not result.exists():
            # Interrupt-child exits through the handler after the child returns;
            # all other successful process boundaries must have a return record.
            raise AssertionError('incomplete boundary '+str(path))
        status,out_start,out_end,err_start,err_end,child_errno=map(int,result.read_text().split())
        entry=request(path)
        executable=cases.host_path(entry['file']) if ':' in entry['file'] else work/entry['file']
        if executable.is_file():
            entry['executable_input']=dict(path=cases.windows(executable),**blob(executable.read_bytes()))
            if executable.parent==cases.BINARY/'build':
                entry['executable_input']['fixture_source']=dict(path='src/pass-helper.c',sha256=cases.sha((cases.BINARY/'src/pass-helper.c').read_bytes()))
        entry.update(status=status,errno=child_errno if status==-1 else 0,cwd=cases.canonical_cwd(case_id),
          before=tree(capture/(stem+'-before')),after=tree(capture/(stem+'-after')),
          stdout=blob(stdout.read_bytes()[out_start:out_end]),stderr=blob(stderr.read_bytes()[err_start:err_end]))
        boundaries.append(entry)
    final=tree(work)
    for name,metadata in final.items():
        normalized,ignored=artifacts.normalized_artifact(work/name)
        metadata.update(normalized_sha256=cases.sha(normalized),ignored_fields=ignored)
    if not migrate_paths:
        assert set(final)==set(expected['products']), 'existing artifact set changed'
        for name,metadata in final.items():
            previous=expected['products'][name]['original']
            assert metadata['normalized_sha256']==previous['normalized_sha256'], 'existing artifact changed: '+name
    record=dict(schema=2,provenance='Fresh original CL and child capture using fixed shared R:/S: drives; no native execution or relabeled raw data',
       target_sha256=cases.sha((cases.TC/'bin/CL.EXE').read_bytes()),case=case_id,arguments=case['arguments'].format(**substitutions),
       environment={k:env[k] for k in ('PATH','INCLUDE','LIB','TMP','TEMP','CL','_CL_','_MSC_IDE_FLAGS','LINK','_LINK_','TZ') if k in env},
       initial=initial,
       original_entry_proof=dict(method='One-shot original application entry breakpoints; accepted application prefix excluded',seen_prefix=projection['accepted_application'][:segment['seen_count']],plan={name:'0x%x'%rva for rva,name in plan.items()},entries=observed,events=events.read_text(),capture_helper_sha256=cases.sha((cases.BINARY/'build/capture-run.exe').read_bytes()),capture_source_sha256=cases.sha((cases.BINARY/'src/capture-run.c').read_bytes()),case_cwd=cases.canonical_cwd(case_id)),
       boundaries=boundaries,final=final,stdout=blob(stdout.read_bytes()),stderr=blob(stderr.read_bytes()),events=events.read_text(),
       timing=dict(setup_seconds=setup,capture_execution_seconds=execution,capture_checks_seconds=time.perf_counter()-begin,total_seconds=time.perf_counter()-start))
    STORE.mkdir(parents=True,exist_ok=True)
    destination.write_text(json.dumps(record,indent=2)+'\n')
    print(case_id,len(boundaries),record['timing'],flush=True)
    fcntl.flock(lease,fcntl.LOCK_UN);lease.close()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--migrate-paths',action='store_true',help='Explicit maintenance capture into a separate candidate store; never refreshes active oracles')
    parser.add_argument('--output-store',type=Path)
    parser.add_argument('--case',action='append');parser.add_argument('--all',action='store_true');parser.add_argument('--force',action='store_true')
    options=parser.parse_args();registry=cases.specifications()
    global STORE
    if options.output_store:STORE=options.output_store.resolve()
    if options.migrate_paths:assert options.output_store and STORE!=cases.CASES/'native-boundaries', 'Migration captures must be staged separately'
    SCRATCH.mkdir(parents=True,exist_ok=True)
    capture_lease=(SCRATCH/'capture.lock').open('a');fcntl.flock(capture_lease,fcntl.LOCK_EX)
    selected=list(registry) if options.all else options.case
    assert selected, 'select --case or --all explicitly'
    for case_id in selected:
        if options.all and not options.force and (STORE/(case_id+'.json')).exists():continue
        run(registry[case_id],options.force,options.migrate_paths)
    # Pin the complete immutable SDK/tool binaries and libraries, including all
    # transitive include/library files. Replay validates these before launch.
    inventory={str(p.relative_to(cases.TC)):dict(sha256=cases.sha(p.read_bytes()),bytes=p.stat().st_size) for p in sorted(cases.TC.rglob('*')) if p.is_file()}
    inventory_path=STORE/'toolchain.json'
    if inventory_path.exists():
        assert json.loads(inventory_path.read_text())==inventory, 'captured toolchain dependency inventory changed'
    else:inventory_path.write_text(json.dumps(inventory,indent=2)+'\n')

if __name__=='__main__':main()
