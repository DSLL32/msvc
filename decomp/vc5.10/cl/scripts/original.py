#!/usr/bin/env python3
"""Explicit original-only reference capture; never a fixture dependency/import."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time
from mask import construct
from build_inputs import inventory as build_inputs,store_blob
from case_inputs import load as load_case,copy_inputs,dependencies,copy_dependencies

HERE=Path(__file__).resolve().parent
PROJECT=HERE.parent
ROOT=PROJECT.parents[2]
SCRATCH=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
CRT_SHA='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
spec=importlib.util.spec_from_file_location('cl_original_wine',HERE/'wine-tool.py')
wt=importlib.util.module_from_spec(spec);spec.loader.exec_module(wt)
sha=lambda data:hashlib.sha256(data).hexdigest()


def dependency_mode(config,baseline,case_name,analysis,complete,response):
    assert not response or (not baseline and not analysis and not complete and case_name=='minimal-c'), 'response mode requires a separate masked minimal-C trial'
    assert not response or '_execute_pass' in config['accepted'], 'response mode cannot patch an unaccepted dispatcher'
    assert not analysis or (baseline and case_name=='minimal-c'), 'dependency analysis requires explicit original-only minimal-C baseline'
    assert not complete or analysis, 'complete dependency observation requires explicit analysis mode'
    if response:return 'CL_DEPENDENCY_RESPONSE_V1\n'
    if analysis:return 'CL_DEPENDENCY_ANALYSIS_V2_COMPLETE\n'if complete else'CL_DEPENDENCY_ANALYSIS_V1\n'
    return None


def link_mode(config,baseline,case,analysis,complete,response,enabled):
    if not enabled:return None
    assert case=='link-object' and not baseline and not analysis and not complete and not response, 'link response requires separate masked linker trial'
    assert {'_link_pass','_write_link_response','_write_link_message','_execute_pass'}<=set(config['accepted']), 'link response may not enable an unaccepted caller'
    return 'CL_LINK_RESPONSE_V1\n'


def clock_analysis_mode(baseline,case,analysis,complete,response,link_response,enabled):
    if not enabled:return None
    assert baseline and case=='link-timing' and not any((analysis,complete,response,link_response)), 'clock analysis requires an explicit separate unmasked LINK-timing baseline'
    return 'CL_LINK_CLOCK_ANALYSIS_V1\n'


def clock_response_mode(config,baseline,case,analysis,complete,response,link_response,clock_analysis,enabled):
    if not enabled:return None
    assert not baseline and case=='link-timing'and not any((analysis,complete,response,link_response,clock_analysis)), 'clock response requires a separate masked timing trial'
    assert {'_start_timer','_link_pass','_write_link_response','_write_link_message','_execute_pass'}<=set(config['accepted']), 'clock response cannot enable an unaccepted caller'
    return 'CL_LINK_CLOCK_RESPONSE_V1\n'


def mre_analysis_mode(baseline,case,analysis,complete,response,link_response,clock_analysis,clock_response,enabled):
    if not enabled:return None
    assert baseline and case=='mre-prefix'and not any((analysis,complete,response,link_response,clock_analysis,clock_response)), 'MRE prefix analysis requires separate unmasked original-only case'
    return 'CL_MRE_PREFIX_ANALYSIS_V2\n'


def capture(label,config_path,baseline=False,case_name='help',dependency_analysis=False,dependency_complete=False,dependency_response=False,link_response=False,clock_analysis=False,clock_response=False,mre_analysis=False):
    assert not dependency_analysis or (baseline and case_name=='minimal-c'),'dependency analysis requires explicit original-only minimal-C baseline'
    assert not dependency_complete or dependency_analysis,'complete dependency observation requires explicit analysis mode'
    start=time.perf_counter()
    config=json.loads(config_path.read_text())
    mre_mode=mre_analysis_mode(baseline,case_name,dependency_analysis,dependency_complete,dependency_response,link_response,clock_analysis,clock_response,mre_analysis)
    mode=clock_response_mode(config,baseline,case_name,dependency_analysis,dependency_complete,dependency_response,link_response,clock_analysis,clock_response) or clock_analysis_mode(baseline,case_name,dependency_analysis,dependency_complete,dependency_response,link_response,clock_analysis) or link_mode(config,baseline,case_name,dependency_analysis,dependency_complete,dependency_response,link_response) or dependency_mode(config,baseline,case_name,dependency_analysis,dependency_complete,dependency_response)
    target=ROOT/config['target']['path']
    assert sha(target.read_bytes())==config['target']['sha256']
    paired={key:sha((PROJECT/path).read_bytes()) for key,path in {
        'native_sha256':'build/native-trace/CL.EXE','rebuilt_sha256':'build/cl.exe'}.items()}
    dependency_hashes=build_inputs(PROJECT,ROOT)
    helper_build=json.loads((PROJECT/'build/reference-debugger-build.json').read_text())
    assert helper_build['binary']['sha256']==sha((PROJECT/helper_build['binary']['path']).read_bytes())
    for name,digest in helper_build['sources'].items():
        assert sha((PROJECT/name).read_bytes())==digest,'original-only debugger build is stale: '+name
    directory=SCRATCH/label
    assert not directory.exists(),'capture directory already exists; choose a fresh label'
    directory.mkdir(parents=True)
    case=load_case(case_name)
    copy_inputs(case,directory)
    compiler_inputs=dependencies(case_name)
    if compiler_inputs:copy_dependencies(compiler_inputs,directory)
    if case['inputs']:
        (directory/'case-inputs.json').write_text(json.dumps(case,indent=2)+'\n')
    image=target.read_bytes() if baseline else construct(config)
    (directory/'CL.EXE').write_bytes(image)
    (directory/'config.json').write_bytes(config_path.read_bytes())
    if case_name=='preprocessor-pass-options'and '_set_pass_suffix'in config['accepted']:
        from pass_state import CONTROL
        (directory/'pass-state-observer.txt').write_bytes(CONTROL)
    if mode:
        (directory/'dependency-observer.txt').write_text(mode)
    if mre_mode:(directory/'mre-observer.txt').write_text(mre_mode)
    if clock_analysis:(directory/'clock-observer.txt').write_text('CL_CLOCK_OBSERVER_V1\n')
    if clock_response:(directory/'clock-observer.txt').write_text('CL_CLOCK_RESPONSE_V1\n')
    if not baseline or dependency_analysis or clock_analysis or mre_analysis:
        from binary import PE
        pe=PE(target)
        rows=[]
        for record in config['driver']:
            if record['name'] in config['accepted']:
                rows.append('%x %d %s\n'%(record['start']-pe.base,pe.read(record['start'],1)[0],record['native_symbol']))
        (directory/'entries.txt').write_text(''.join(rows))
    for name in ('MSVCRT.DLL','MSPDB50.DLL'):
        source=ROOT/'toolchains/5.10/bin'/name
        shutil.copyfile(source,directory/name)
    assert sha((directory/'MSVCRT.DLL').read_bytes())==CRT_SHA
    shutil.copyfile(PROJECT/'build/reference-debugger.exe',directory/'reference-debugger.exe')
    shutil.copyfile(HERE/'reference-debugger.c',directory/'reference-debugger.c')
    shutil.copyfile(HERE/'dependency-observer.inc',directory/'dependency-observer.inc')
    shutil.copyfile(HERE/'export-reader.h',directory/'export-reader.h')
    shutil.copyfile(HERE/'clock-observer.inc',directory/'clock-observer.inc')
    shutil.copyfile(HERE/'mre-observer.inc',directory/'mre-observer.inc')
    controller_source=store_blob(Path(__file__).read_bytes())
    helper_source=store_blob((HERE/'reference-debugger.c').read_bytes())
    env=wt.environment();env.update(TMP='.',TEMP='.')
    command=['/usr/bin/wine',wt.windows(directory/'reference-debugger.exe'),
        wt.windows(directory/'CL.EXE'),subprocess.list2cmdline(case['arguments']),wt.windows(directory),'0' if baseline else '1',
        '1' if '_load_string' in config['accepted'] else '0']
    setup=time.perf_counter()-start
    # The fresh directory contains only these controlled files before the child
    # starts; _mktemp's collision checks must not depend on hidden fixtures.
    initial=[]
    for path in sorted(directory.iterdir()):
        assert path.is_file(),'unexpected original input-directory entry'
        initial.append({'name':path.name,'bytes':path.stat().st_size,'sha256':sha(path.read_bytes())})
    (directory/'input-directory.json').write_text(json.dumps({'entries':initial},indent=2)+'\n')
    run_start=time.perf_counter()
    process=subprocess.Popen(command,cwd=directory,env=env,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    # Read-only inspection of the stopped original. A failed decoder releases
    # this helper/child pair and preserves partial raw files before reporting.
    import selectors,struct
    first=b'';controller_error=None
    try:
        selector=selectors.DefaultSelector();selector.register(process.stdout,selectors.EVENT_READ)
        assert selector.select(45),'bounded original capture produced no debugger response'
        first=process.stdout.readline()
        if first.startswith(b'TRAP '):
            from frontier_state import capture_original
            address=int(first.split()[1],16)
            stack=struct.unpack_from('<16I',(directory/'context.bin').read_bytes(),204)
            capture_original(directory,address,stack,process)
            process.stdin.write(b'DONE\n');process.stdin.flush()
        stdout,stderr=process.communicate(timeout=45)
    except Exception as error:
        controller_error=repr(error)
        try:
            process.stdin.write(b'DONE\n');process.stdin.flush()
            stdout,stderr=process.communicate(timeout=5)
        except Exception:
            process.kill();stdout,stderr=process.communicate(timeout=5)
        (directory/'controller-error.txt').write_text(controller_error+'\n')
    result=type('CaptureResult',(),{'stdout':first+stdout,'stderr':stderr,'returncode':process.returncode})()
    run_time=time.perf_counter()-run_start
    (directory/'helper.stdout').write_bytes(result.stdout)
    (directory/'helper.stderr').write_bytes(result.stderr)
    loaded=directory/'loaded-msvcrt.bin'
    captured_paths=directory.rglob('*')if (dependency_response or link_response or clock_analysis or clock_response or mre_analysis) else directory.iterdir()
    receipt={'schema':1,'controller_error':controller_error,**case,'baseline':baseline,'dependency_analysis':dependency_analysis,'dependency_complete':dependency_complete,'dependency_response':dependency_response,'link_response':link_response,'clock_analysis':clock_analysis,'clock_response':clock_response,'mre_analysis':mre_analysis,
        'target_sha256':sha(target.read_bytes()),'config_sha256':sha(config_path.read_bytes()),
        'original_mask_sha256':sha(image),'helper_status':result.returncode,
        'loaded_msvcrt_sha256':sha(loaded.read_bytes()) if loaded.exists() else None,
        'loaded_target_sha256':sha((directory/'loaded-target.bin').read_bytes()) if (directory/'loaded-target.bin').exists() else None,
        'helper_sha256':sha((directory/'reference-debugger.exe').read_bytes()),'helper_build':helper_build,
        **paired,'build_dependency_hashes':dependency_hashes,
        'helper_source_sha256':sha((HERE/'reference-debugger.c').read_bytes()),
        'helper_sources':{name:sha((HERE/name).read_bytes())for name in ('reference-debugger.c','dependency-observer.inc','clock-observer.inc','mre-observer.inc','export-reader.h')},
        'controller_source':controller_source,'helper_source':helper_source,
        'command':command,'environment':{k:env[k] for k in ('WINEPREFIX','WINEDLLOVERRIDES','INCLUDE','LIB','WINEPATH','TMP','TEMP')},
        'timing':{'setup_seconds':setup,'reference_execution_seconds':run_time},
        'files':{p.relative_to(directory).as_posix():{'sha256':sha(p.read_bytes()),'bytes':p.stat().st_size} for p in captured_paths if p.is_file()}}
    (directory/'capture.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'path':str(directory),'status':result.returncode,'loaded_msvcrt_sha256':receipt['loaded_msvcrt_sha256'],'timing':receipt['timing']}),flush=True)
    assert controller_error is None,'original decoder failed; raw evidence preserved: '+str(controller_error)
    if result.returncode:
        # The debugger deliberately returns the child's normal status. Its
        # private errors lack this exact sole normal-exit event and no-trap
        # condition; preserve them as controller failures.
        import re
        events=(directory/'events.txt').read_text()
        exits=re.findall(r'^exit .*status=(\d+)$',events,re.M)
        assert result.returncode==(case.get('expected_status',0)&255) and exits==[str(case.get('expected_status',0))]and not re.search(r'^trap ',events,re.M),'original capture failed; raw evidence preserved'
    # Paired PE/ELF/source hashes above are historical bookkeeping, not inputs
    # to this original process. Current pair assembly binds native/audit inputs.
    assert receipt['loaded_target_sha256']==receipt['original_mask_sha256'],'actual loaded target differs from trial mask'
    assert receipt['loaded_msvcrt_sha256']==CRT_SHA,'loaded original CRT differs; raw evidence preserved'
    return directory,receipt


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--label',required=True)
    parser.add_argument('--config',type=Path,default=PROJECT/'functions.json')
    parser.add_argument('--baseline',action='store_true')
    parser.add_argument('--case',default='help')
    parser.add_argument('--dependency-analysis',action='store_true',help='Explicit original-only loader/Invoke observation, stopping before C2')
    parser.add_argument('--dependency-complete',action='store_true',help='Also observe both putenv calls, actual C2 return and normal CL exit')
    parser.add_argument('--dependency-response',action='store_true',help='Observe dependencies during a fresh masked candidate trial, preserving its real next frontier')
    parser.add_argument('--link-response',action='store_true',help='Observe LINK-only child during a masked candidate trial')
    parser.add_argument('--clock-analysis',action='store_true',help='Explicit unmasked LINK-clock analysis only; never reconstruction acceptance')
    parser.add_argument('--clock-response',action='store_true',help='Observe actual clock and LINK effects in a masked timing trial; success remains closed until typed comparison')
    parser.add_argument('--mre-analysis',action='store_true',help='Bounded unmasked Open/Query/Init analysis only, stopping before option-signature logic')
    args=parser.parse_args();capture(args.label,args.config,args.baseline,args.case,args.dependency_analysis,args.dependency_complete,args.dependency_response,args.link_response,args.clock_analysis,args.clock_response,args.mre_analysis)
