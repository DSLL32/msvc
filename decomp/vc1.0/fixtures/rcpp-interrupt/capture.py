#!/usr/bin/env python3
"""Real Ctrl+C during a native, empty named-pipe ReadFile; no callback injection."""
from pathlib import Path
import hashlib
import importlib.machinery
import json
import re
import shutil
import subprocess
import threading
import time

HERE = Path(__file__).resolve().parent
RCPP = HERE.parent.parent/'rcpp'
wt = importlib.machinery.SourceFileLoader('rcpp_interrupt_wine', str(RCPP/'scripts/wine-tool.py')).load_module()
SCRATCH = wt.PREFIX.parent/'interrupt'
LOCK = threading.Lock()
ARGUMENTS = '-ef T:\\bin\\c1.err -f interrupt.rc -g result.i -E'


def environment(label, work):
    # Separate wineservers isolate the identically named pipes during paired
    # runs, preserving identical input/argv/output bytes without PID rewriting.
    assert re.fullmatch('[a-z0-9-]+', label)
    with LOCK:
        prefix = SCRATCH/('wine-'+label)
        env = wt.environment()
        env['WINEPREFIX'] = str(prefix)
        if not (prefix/'system.reg').exists():
            prefix.parent.mkdir(parents=True,exist_ok=True)
            with (SCRATCH/('wineboot-'+label+'.log')).open('wb') as log:
                subprocess.run(['wineboot','-u'],env=env,stdout=log,stderr=log,check=True,timeout=120)
        drive = prefix/'dosdevices/t:'
        if not drive.is_symlink(): drive.symlink_to(wt.TC,target_is_directory=True)
        assert drive.resolve() == wt.TC.resolve()
        result = subprocess.run(['wineserver','-p'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        assert result.returncode in (0,2)
    env.update(TEMP=wt.windows(work),TMP=wt.windows(work))
    return env


def execute(executable, arguments, work, stem, rvas, label):
    stem,work=Path(stem).resolve(),Path(work).resolve()
    before=Path(str(stem)+'.before')
    if before.exists(): shutil.rmtree(before)
    before.mkdir(parents=True)
    command=['wine',str(RCPP/'build/interrupt-run.exe'),wt.windows(executable),arguments,
             wt.windows(str(stem)+'.stdout'),wt.windows(str(stem)+'.stderr'),
             wt.windows(str(stem)+'.events'),wt.windows(work)]
    command += ['%x'%value for value in rvas]
    command += [wt.windows(str(stem)+'.proof'),wt.windows(before),'ctrl-c']
    try:
        with Path(str(stem)+'.launcher').open('wb') as log:
            subprocess.run(command,env=environment(label,work),stdin=subprocess.DEVNULL,
                           stdout=log,stderr=log,check=True,timeout=90)
    except Exception:
        failed=SCRATCH/'failed-attempts'/('%s-%d'%(label,time.time_ns()))
        failed.mkdir(parents=True)
        for path in stem.parent.glob(stem.name+'.*'):
            if path.is_file(): shutil.copyfile(path,failed/path.name)
            elif path.is_dir(): shutil.copytree(path,failed/path.name)
        shutil.copytree(work,failed/'work')
        raise
    proof=Path(str(stem)+'.proof').read_text()
    events=Path(str(stem)+'.events').read_text()
    if 'signal=CTRL_C_EVENT' in proof:
        target=int(re.search(r'target_pid=(\d+)',proof)[1])
        controller=int(re.search(r'controller_pid=(\d+)',proof)[1])
        primary=int(re.search(r'primary_tid=(\d+)',proof)[1])
        assert set(map(int,re.findall(r'console_member=(\d+)',proof))) == {target,controller}
        assert 'group_semantics=console-local-broadcast event=0' in proof
        assert 'registration_return=TRUE' in proof
        assert 'native_ReadFile ' in proof and 'type=PIPE requested=' in proof
        assert 'boundary=blocked_pipe_ReadFile primary_suspended=1' in proof and 'available=0' in proof
        assert ('primary_resuspended=1 boundary=OS_callback_thread_creation' in proof or
                'primary_resuspended=1 boundary=ReadFile_return_barrier' in proof)
        assert 'control_exception=40010005' in proof and 'disposition=not_handled' in proof
        callback=re.search(r'callback pid=(\d+) tid=(\d+) argument=(\d+) handler_rva=([0-9a-f]+)',proof)
        assert callback and int(callback[1])==target and int(callback[2])!=primary and int(callback[3])==0
        assert int(callback[4],16)==rvas[2]
        assert int(callback[2]) in set(map(int,re.findall(r'target_thread=(\d+)',proof)))
        assert 'reached_handler=1 controller_breaks=0 controller_interrupts=1' in proof
        assert {p.name:p.read_bytes() for p in before.iterdir()} == {
            'interrupt.rc':(HERE/'source/interrupt.rc').read_bytes(),'result.i':b''}
        if events=='exit 1\n': assert 'cleanup pipe_disconnected=1 output_exclusive_open=1' in proof
    return events


def tree(directory):
    return {p.relative_to(directory).as_posix():p.read_bytes() for p in directory.rglob('*') if p.is_file()}


def capture():
    import sys
    sys.path.insert(0,str(RCPP/'scripts'))
    from reference import imports
    from binary import PE
    manifest=json.loads((HERE/'manifest.json').read_text())
    for item in manifest['source_inputs']+manifest['capture_evidence']+manifest['analysis_tooling']:
        data=(HERE/item['path']).read_bytes()
        assert len(data)==item['bytes'] and hashlib.sha256(data).hexdigest()==item['sha256']
    cfg=json.loads((RCPP/'functions.json').read_text())
    original=wt.ROOT/cfg['target']['path'];pe=PE(original)
    assert hashlib.sha256(original.read_bytes()).hexdigest()==cfg['target']['sha256']
    thunks={f['name']:f['start']-pe.base for f in imports(pe)}
    handler=next(f['start']-pe.base for f in cfg['functions'] if f['name']=='_rc_control_handler@4')
    rvas=[thunks['import:SetConsoleCtrlHandler'],thunks['import:ReadFile'],handler]
    outputs=[]
    for number in range(2):
        work=SCRATCH/('capture-work-%d'%number);stem=SCRATCH/('capture-%d'%number)
        if work.exists():shutil.rmtree(work)
        shutil.copytree(HERE/'source',work)
        events=execute(original,ARGUMENTS,work,stem,rvas,'baseline')
        assert events=='exit 1\n',events
        assert 'signal=CTRL_C_EVENT' in Path(str(stem)+'.proof').read_text()
        streams={name:Path(str(stem)+'.'+name).read_bytes()for name in ['stdout','stderr']}
        assert streams=={'stdout':b'','stderr':b''}
        outputs.append((tree(work),streams))
    assert outputs[0]==outputs[1]
    assert outputs[0][0]=={'interrupt.rc':(HERE/'source/interrupt.rc').read_bytes(),
                           'result.i':(HERE/'oracle/result.i').read_bytes()}
    assert outputs[0][1]=={name:(HERE/'oracle'/(name+'.bin')).read_bytes() for name in ['stdout','stderr']}
    print('Two original captures agree:',{name:len(data)for name,data in outputs[0][0].items()})
    return rvas,outputs[0]


if __name__=='__main__': capture()
