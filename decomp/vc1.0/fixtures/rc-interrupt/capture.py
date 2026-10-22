#!/usr/bin/env python3
"""Analysis-only real-console runner, shared by capture and paired RC replay.

No callback injection or PID substitution. Raw proof and before-files are saved;
only relations involving OS-assigned IDs are checked, never normalized.
"""
from pathlib import Path
import hashlib
import importlib.machinery
import json
import re
import shutil
import subprocess
import threading

HERE = Path(__file__).resolve().parent
RC = HERE.parent.parent / 'rc'
wt = importlib.machinery.SourceFileLoader('interrupt_wine_tool', str(RC/'wine-tool.py')).load_module()
SCRATCH = Path.home()/'.ai-tmp/vc1.0-rc-interrupt'
LOCK = threading.Lock()


def environment(work):
    with LOCK:
        prefix = SCRATCH/'wine'
        env = wt.environment()
        env['WINEPREFIX'] = str(prefix)
        if not (prefix/'system.reg').exists():
            prefix.parent.mkdir(parents=True, exist_ok=True)
            with (SCRATCH/'wineboot.log').open('wb') as log:
                subprocess.run(['wineboot','-u'],env=env,stdout=log,stderr=log,check=True,timeout=90)
        drive = prefix/'dosdevices/t:'
        if not drive.is_symlink(): drive.symlink_to(wt.TC, target_is_directory=True)
        assert drive.resolve() == wt.TC.resolve()
        server = subprocess.run(['wineserver','-p'], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        assert server.returncode in (0,2)
    env.update(TEMP=wt.windows(work),TMP=wt.windows(work))
    return env


def execute(executable, arguments, work, stem, rvas):
    stem, work = Path(stem).resolve(), Path(work).resolve()
    before = Path(str(stem)+'.before')
    if before.exists(): shutil.rmtree(before)
    before.mkdir(parents=True)
    command = ['wine',str(RC/'build/interrupt-run.exe'),wt.windows(executable),arguments,
               wt.windows(str(stem)+'.stdout'),wt.windows(str(stem)+'.stderr'),
               wt.windows(str(stem)+'.events'),wt.windows(work)]
    command += ['%x'%v for v in rvas]
    command += [wt.windows(str(stem)+'.proof'),wt.windows(before),'ctrl-c']
    with Path(str(stem)+'.launcher').open('wb') as log:
        subprocess.run(command,env=environment(work),stdin=subprocess.DEVNULL,stdout=log,stderr=log,check=True,timeout=90)
    proof = Path(str(stem)+'.proof').read_text()
    events = Path(str(stem)+'.events').read_text()
    if 'signal=CTRL_C_EVENT' in proof:
        target = int(re.search(r'target_pid=(\d+)',proof)[1])
        controller = int(re.search(r'controller_pid=(\d+)',proof)[1])
        primary = int(re.search(r'primary_tid=(\d+)',proof)[1])
        assert set(map(int,re.findall(r'console_member=(\d+)',proof))) == {target,controller}
        assert 'group_semantics=console-local-broadcast event=0' in proof
        assert 'registration_return=TRUE' in proof and 'primary_suspended=1' in proof
        assert 'primary_resuspended=1 boundary=OS_callback_thread_creation' in proof
        assert 'control_exception=40010005' in proof and 'disposition=not_handled' in proof
        callback = re.search(r'callback pid=(\d+) tid=(\d+) argument=(\d+) handler_rva=([0-9a-f]+)',proof)
        assert callback and int(callback[1]) == target and int(callback[2]) != primary and int(callback[3]) == 0
        assert int(callback[4],16) == rvas[2]
        assert 'reached_handler=1 controller_breaks=0 controller_interrupts=1' in proof
        rcname, rdname = 'RC%06d'%target, 'RD%06d'%target
        observed = {p.name:p.read_bytes() for p in before.iterdir()}
        prepared = b'#line 1"INTERRUPT.RC"\r\n#line 1\r\n1 RCDATA BEGIN 7 END\r\n'
        preprocessed = ('#line 1 "%s"\r\n#line 1 "INTERRUPT.RC"\r\n#line 1 "INTERRUPT.RC"\r\n1 RCDATA BEGIN 7 END\r\n'%rcname).encode('ascii')
        assert observed == {'interrupt.rc':(HERE/'source/interrupt.rc').read_bytes(),
                            'interrupt.res':b'', rcname:prepared, rdname:preprocessed}
    return events


def record(path):
    data = path.read_bytes()
    return dict(path=str(path.relative_to(HERE)),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())


if __name__ == '__main__':
    import sys
    sys.path.insert(0,str(RC))
    from binary import PE,imports
    original = wt.TC/'bin/RC.EXE'
    pe = PE(original)
    cfg = json.loads((RC/'functions.json').read_text())
    assert hashlib.sha256(original.read_bytes()).hexdigest() == cfg['target']['sha256']
    functions = {r['name']:r['start']-pe.base for r in cfg['driver']}
    thunk = next(r['start']-pe.base for r in imports(pe) if r['name']=='import:SetConsoleCtrlHandler')
    work = SCRATCH/'independent-original'; stem = SCRATCH/'independent-original-capture'
    if work.exists():shutil.rmtree(work)
    shutil.copytree(HERE/'source',work)
    assert execute(original,'/x /fointerrupt.res interrupt.rc',work,stem,
                   [thunk,functions['_initialize_lexer@4'],functions['_interrupt_resource@4']]) == 'exit 1\n'
    assert {p.name:p.read_bytes() for p in work.iterdir()} == {'interrupt.rc':(HERE/'source/interrupt.rc').read_bytes()}
    assert Path(str(stem)+'.stdout').read_bytes() == Path(str(stem)+'.stderr').read_bytes() == b''
    print('Original: real Ctrl+C callback argument 0; exit 1; empty streams; exact cleanup.')
