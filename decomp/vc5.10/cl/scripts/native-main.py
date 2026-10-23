#!/usr/bin/env python3
"""Native Linux-only GDB capture at the still-trapping CL main function."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import shutil
import time
from state import vector,environment,project
from build_inputs import verify_native_build

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def capture(original,directory):
    original=Path(original);directory=Path(directory)
    assert not directory.exists(),'fresh native capture directory required'
    directory.mkdir(parents=True)
    shutil.copyfile(original/'config.json',directory/'config.json')
    binary_before=sha(PROJECT/'build/native-trace/CL.EXE')
    rebuilt_before=sha(PROJECT/'build/cl.exe')
    build_path=Path(str(PROJECT/'build/native-trace/CL.EXE')+'.build.json')
    build_bytes=build_path.read_bytes()
    build=json.loads(build_bytes)
    verify_native_build(build,binary_before,PROJECT)
    (directory/'native-build.json').write_bytes(build_bytes)
    (directory/'status_reader.py').write_bytes(subprocess.check_output(['git','show',build['shared_revision']+':lib/recon/status.py'],cwd=ROOT))
    argv=vector(original/'argv.bin')[1]
    profile_path=PROJECT/'cases/help/entry-profile.json'
    profile=json.loads(profile_path.read_text())
    assert profile['arguments']==argv[1:] and profile['id']=='help-canonical-msvcrt-v1'
    assert sha(Path(profile['evidence']['path']))==profile['evidence']['sha256']
    if (original/'mktemp-entry-teb.bin').exists():
        from mktemp_state import original as temporary_state
        assert temporary_state(original)['entry']==profile['state'],'unknown original entry profile'
    shutil.copyfile(profile_path,directory/'entry-profile.json')
    variables=environment(vector(original/'environment.bin')[1])
    assert variables==profile['environment'],'unknown consumed entry-profile inputs'
    env=os.environ.copy()
    for key,value in variables.items():
        if value is None:env.pop(key,None)
        else:env[key]=value
    env['RECON_PROGRAM_PATH']=(original/'pgmptr.bin').read_bytes()[:-1].decode('latin1')
    env['CL_ENTRY_PROFILE']=profile['id']
    temporary=None
    if (original/'mktemp-before.bin').exists():
        from mktemp_state import original as temporary_state
        temporary=temporary_state(original)
        reader=subprocess.check_output(['git','show',build['shared_revision']+':lib/recon/mktemp.py'],cwd=ROOT)
        (directory/'mktemp_reader.py').write_bytes(reader)
        (directory/'mktemp-expected.json').write_text(json.dumps(temporary,indent=2)+'\n')
        call=temporary['calls'][0]
        env['RECON_MKTEMP_TEMPLATE']=call['template']
        env['RECON_MKTEMP_SUFFIX']=call['result'][-6:]
        env['RECON_MKTEMP_PRESTATE']=','.join(str(call['before'][name])for name in ('errno','doserrno','last_error'))
    resource_before=None
    if (PROJECT/'build/resources.bin').exists():
        shutil.copyfile(PROJECT/'build/resources.bin',directory/'resources.bin')
        env['RECON_RESOURCE_PROVIDER']=str(directory/'resources.bin')
        resource_before=sha(directory/'resources.bin')
    # Freeze the actual GDB controller/schema bytes used for this execution.
    for name in ('gdb_capture.py','gdb_entry.py','gdb_mktemp.py','native_status.py','frontier_state.py'):
        shutil.copyfile(PROJECT/'scripts'/name,directory/name)
    script='import sys\nsys.path.insert(0,'+repr(str(directory))+')\nfrom gdb_capture import capture\ncapture('+repr(str(directory))+')\n'
    (directory/'gdb-capture.py').write_text(script)
    (directory/'gdb-entry.py').write_text('import sys\nsys.path.insert(0,'+repr(str(directory))+')\nfrom gdb_entry import capture\ncapture('+repr(str(directory))+')\nfrom gdb_mktemp import install\ninstall('+repr(str(directory))+')\n')
    # argv[0] is supplied as captured Win32 data while the executable is ELF.
    commands=['/usr/bin/gdb','-q','-nx','-batch','-ex','set pagination off',
        '-ex','set exec-wrapper /usr/bin/env',
        '-ex',f'set args /?',
        '-ex','tbreak *main',
        '-ex',f'run > {directory}/stdout.bin 2> {directory}/stderr.bin',
        '-ex','source '+str(directory/'gdb-entry.py'),
        '-ex','continue',
        '-ex','source '+str(directory/'gdb-capture.py'),'--args',str(PROJECT/'build/native-trace/CL.EXE')]
    # GDB normally chooses ELF path as argv[0]. Use its native exec wrapper to
    # exec the same ELF with exactly the captured Win32 program-name argument.
    wrapper=directory/'exec-wrapper.py'
    config=json.loads((directory/'config.json').read_text())
    names=[r['native_symbol'] for r in config['driver']if r['name'] in config['accepted']]
    (directory/'trace-filter.txt').write_text(''.join(name+'\n'for name in names))
    (directory/'trace-seen.txt').write_text('')
    env['RECON_TRACE_FILTER']=str(directory/'trace-filter.txt');env['RECON_TRACE_SEEN']=str(directory/'trace-seen.txt')
    receipt=''
    if temporary is not None:
        receipt='fd=os.open('+repr(str(directory/'mktemp-receipt.bin'))+',os.O_CREAT|os.O_TRUNC|os.O_WRONLY,0o600)\nos.ftruncate(fd,568)\nos.set_inheritable(fd,True)\nos.environ["RECON_MKTEMP_FD"]=str(fd)\n'
    status='fd=os.open('+repr(str(directory/'terminal-status.bin'))+',os.O_CREAT|os.O_TRUNC|os.O_RDWR,0o600)\nos.ftruncate(fd,24)\nos.set_inheritable(fd,True)\nos.environ["RECON_STATUS_FD"]=str(fd)\n'
    wrapper.write_text('#!/usr/bin/python3\nimport os,sys\nfd=os.open('+repr(str(directory/'trace.txt'))+',os.O_CREAT|os.O_TRUNC|os.O_WRONLY,0o600)\nos.set_inheritable(fd,True)\nos.environ["RECON_TRACE_FD"]=str(fd)\n'+status+receipt+'os.execv(sys.argv[1],'+repr(argv)+')\n')
    wrapper.chmod(0o755)
    commands[commands.index('set exec-wrapper /usr/bin/env')]='set exec-wrapper '+str(wrapper)
    started=time.perf_counter()
    result=subprocess.run(commands,env=env,cwd=directory,capture_output=True,stdin=subprocess.DEVNULL,timeout=25)
    elapsed=time.perf_counter()-started
    (directory/'debugger.txt').write_bytes(result.stdout+result.stderr)
    error=None;projection=None
    try:
        assert (directory/'native-entry-errors.json').is_file(),'native entry state capture failed'
        assert binary_before==sha(PROJECT/'build/native-trace/CL.EXE') and rebuilt_before==sha(PROJECT/'build/cl.exe'),'paired executable changed during native capture'
        assert build_bytes==build_path.read_bytes(),'native build receipt changed during capture'
        verify_native_build(build,binary_before,PROJECT)
        if resource_before is not None:assert resource_before==sha(directory/'resources.bin'),'native resource input changed during capture'
        assert (b'SIGTRAP' in result.stdout and b'NATIVE_FRONTIER_SYMBOL=' in result.stdout) or b'NATIVE_COMPLETION_STATUS=0\n' in result.stdout,'native did not reach a frontier or successful completion'
        projection=project('native',directory)
        (directory/'state.json').write_text(json.dumps(projection,indent=2)+'\n')
    except Exception as failure:
        error=repr(failure);(directory/'controller-error.txt').write_text(error+'\n')
    (directory/'capture.json').write_text(json.dumps({'platform':'native','native_sha256':binary_before,
        'rebuilt_sha256':rebuilt_before,'config_sha256':sha(directory/'config.json'),
        'native_build':{'path':str(directory/'native-build.json'),'sha256':sha(directory/'native-build.json')},
        'controller_sha256':sha(Path(__file__)),'gdb_status':result.returncode,'seconds':elapsed,'controller_error':error,
        'files':{p.name:{'sha256':sha(p),'bytes':p.stat().st_size}for p in directory.iterdir()if p.is_file()}},indent=2)+'\n')
    assert error is None,'native capture failed; raw receipt preserved: '+str(error)
    print(json.dumps({'directory':str(directory),'seconds':elapsed,'state':projection['fields']}))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True);parser.add_argument('--directory',type=Path,required=True)
    args=parser.parse_args();capture(args.original,args.directory)
