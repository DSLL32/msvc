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

HERE=Path(__file__).resolve().parent
PROJECT=HERE.parent
ROOT=PROJECT.parents[2]
SCRATCH=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
CRT_SHA='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
spec=importlib.util.spec_from_file_location('cl_original_wine',HERE/'wine-tool.py')
wt=importlib.util.module_from_spec(spec);spec.loader.exec_module(wt)
sha=lambda data:hashlib.sha256(data).hexdigest()


def capture(label,config_path,baseline=False):
    start=time.perf_counter()
    config=json.loads(config_path.read_text())
    target=ROOT/config['target']['path']
    assert sha(target.read_bytes())==config['target']['sha256']
    paired={key:sha((PROJECT/path).read_bytes()) for key,path in {
        'native_sha256':'build/native-trace/CL.EXE','rebuilt_sha256':'build/cl.exe'}.items()}
    dependency_hashes=build_inputs(PROJECT,ROOT)
    directory=SCRATCH/label
    assert not directory.exists(),'capture directory already exists; choose a fresh label'
    directory.mkdir(parents=True)
    image=target.read_bytes() if baseline else construct(config)
    (directory/'CL.EXE').write_bytes(image)
    (directory/'config.json').write_bytes(config_path.read_bytes())
    if not baseline:
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
    controller_source=store_blob(Path(__file__).read_bytes())
    helper_source=store_blob((HERE/'reference-debugger.c').read_bytes())
    env=wt.environment();env.update(TMP='.',TEMP='.')
    command=['/usr/bin/wine',wt.windows(directory/'reference-debugger.exe'),
        wt.windows(directory/'CL.EXE'),'/?',wt.windows(directory),'0' if baseline else '1',
        '1' if '_load_string' in config['accepted'] else '0']
    setup=time.perf_counter()-start;run_start=time.perf_counter()
    # The fresh directory contains only these controlled files before the child
    # starts; _mktemp's collision checks must not depend on hidden fixtures.
    initial=[]
    for path in sorted(directory.iterdir()):
        assert path.is_file(),'unexpected original input-directory entry'
        initial.append({'name':path.name,'bytes':path.stat().st_size,'sha256':sha(path.read_bytes())})
    (directory/'input-directory.json').write_text(json.dumps({'entries':initial},indent=2)+'\n')
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
    receipt={'schema':1,'controller_error':controller_error,'case':'help','arguments':['/?'],'baseline':baseline,
        'target_sha256':sha(target.read_bytes()),'config_sha256':sha(config_path.read_bytes()),
        'original_mask_sha256':sha(image),'helper_status':result.returncode,
        'loaded_msvcrt_sha256':sha(loaded.read_bytes()) if loaded.exists() else None,
        'loaded_target_sha256':sha((directory/'loaded-target.bin').read_bytes()) if (directory/'loaded-target.bin').exists() else None,
        'helper_sha256':sha((directory/'reference-debugger.exe').read_bytes()),
        **paired,'build_dependency_hashes':dependency_hashes,
        'helper_source_sha256':sha((HERE/'reference-debugger.c').read_bytes()),
        'controller_source':controller_source,'helper_source':helper_source,
        'command':command,'environment':{k:env[k] for k in ('WINEPREFIX','WINEDLLOVERRIDES','INCLUDE','LIB','WINEPATH','TMP','TEMP')},
        'timing':{'setup_seconds':setup,'reference_execution_seconds':run_time},
        'files':{p.name:{'sha256':sha(p.read_bytes()),'bytes':p.stat().st_size} for p in directory.iterdir() if p.is_file()}}
    (directory/'capture.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'path':str(directory),'status':result.returncode,'loaded_msvcrt_sha256':receipt['loaded_msvcrt_sha256'],'timing':receipt['timing']}),flush=True)
    assert controller_error is None,'original decoder failed; raw evidence preserved: '+str(controller_error)
    assert result.returncode==0,'original capture failed; raw evidence preserved'
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
    args=parser.parse_args();capture(args.label,args.config,args.baseline)
