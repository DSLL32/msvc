#!/usr/bin/env python3
"""Capture/verify two native RC paths without rewriting any captured stream."""
from pathlib import Path
import argparse
import hashlib
import importlib.machinery
import json
import re
import shutil
import subprocess

HERE=Path(__file__).resolve().parent
RC=HERE.parent.parent/'rc'
wt=importlib.machinery.SourceFileLoader('remaining_wine',str(RC/'wine-tool.py')).load_module()
SCRATCH=Path.home()/'.ai-tmp/vc1.0-rc-remaining'


def digest(data):return dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
def record(path):return dict(path=str(path.relative_to(HERE)),**digest(path.read_bytes()))


def verbose_bytes(pid):
    # Exact native writer output. Six-X _mktemp retains the six low decimal
    # PID digits; pinned fresh source directories exclude first-name collisions.
    digits=('%06d'%pid)[-6:].encode('ascii')
    prefix=(b'Microsoft (R) Windows 32 Resource Compiler, Version 3.10\r\n'
            b'Copyright (C) Microsoft Corp. 1985-1993.  All rights reserved.\r\n\r\n'
            b'Creating verbose.res\r\nRC: RCPP  -f RC')
    middle=b' -g RD'
    suffix=(b' -D RC_INVOKED -pc \\:/ -E \r\n\r\nVERBOSE-NAMES.RC.\r\n'
            b'Writing CUSTOM:ALPHA,\tlang:0x409,\tsize 4.\r\n'
            b'Writing RCDATA:BETA,\tlang:0x409,\tsize 3.\r\n'
            b'Writing CUSTOM:3,\tlang:0x409,\tsize 2')
    assert len(prefix)==161 and len(prefix)+6+len(middle)==173
    result=prefix+digits+middle+digits+suffix
    assert len(result)==349
    return result


def prepare():
    assert hashlib.sha256((wt.TC/'bin/RC.EXE').read_bytes()).hexdigest() == 'eb2e4bb5994303a37326cc60107ee07be01ca267daca1f9116e3149d9ea31591'
    assert hashlib.sha256((wt.TC/'bin/RCPP.EXE').read_bytes()).hexdigest() == 'a66382a8fa84432a1fc380cea85fcdb3305603e5656638f1d959cdb5d25ca6dd'
    SCRATCH.mkdir(exist_ok=True)
    env=wt.environment();env['WINEPREFIX']=str(SCRATCH/'wine');env['WINEPATH']=''
    if not (SCRATCH/'wine/system.reg').exists():
        with (SCRATCH/'wineboot.log').open('wb') as log:
            subprocess.run(['/usr/bin/wineboot','-u'],env=env,stdout=log,stderr=log,check=True,timeout=90)
    drive=SCRATCH/'wine/dosdevices/t:'
    if not drive.is_symlink():drive.symlink_to(wt.TC,target_is_directory=True)
    assert drive.resolve()==wt.TC.resolve()
    server=subprocess.run(['/usr/bin/wineserver','-p'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    assert server.returncode in (0,2)
    launch=SCRATCH/'launch';child_dir=SCRATCH/'preprocessor bin';missing=SCRATCH/'no-such-directory'
    for d in (launch,child_dir):d.mkdir(exist_ok=True)
    shutil.copyfile(wt.TC/'bin/RC.EXE',launch/'rc.exe')
    child=child_dir/'rcpp.exe'
    if not child.is_symlink():
        assert not child.exists();child.symlink_to(wt.TC/'bin/RCPP.EXE')
    assert child.resolve()==(wt.TC/'bin/RCPP.EXE').resolve()
    assert not (launch/'rcpp.exe').exists() and not missing.exists()
    path=';;'+wt.windows(missing)+';;"'+wt.windows(child_dir)+'";;'
    return env,launch,path


def capture(case,env,launch,path,recording):
    work=SCRATCH/('case-'+case)
    if work.exists():shutil.rmtree(work)
    shutil.copytree(HERE/'source',work)
    initial={p.name:p.read_bytes() for p in work.iterdir()}
    assert not any(p.startswith(('RC','RD')) for p in initial)
    stem=SCRATCH/case
    arguments=('/v /x /foverbose.res verbose-names.rc' if case=='verbose-names' else
               '/x /fopath.res path-fallback.rc')
    target=wt.TC/'bin/RC.EXE' if case=='verbose-names' else launch/'rc.exe'
    env=dict(env,TEMP=wt.windows(work),TMP=wt.windows(work))
    command=['/usr/bin/wine',str(RC/'build/remaining-trace.exe'),wt.windows(target),arguments,
             wt.windows(str(stem)+'.stdout'),wt.windows(str(stem)+'.stderr'),wt.windows(str(stem)+'.proof'),
             wt.windows(work),'' if case=='verbose-names' else path]
    with Path(str(stem)+'.launcher').open('wb') as log:
        subprocess.run(command,env=env,stdin=subprocess.DEVNULL,stdout=log,stderr=log,check=True,timeout=90)
    proof=Path(str(stem)+'.proof').read_text()
    pid=int(re.search(r'target_pid=(\d+)',proof)[1])
    exits=re.findall(r'exit pid=(\d+) status=(\d+)',proof)
    assert len(exits)==2 and exits[-1]==(str(pid),'0') and exits[0][1]=='0'
    assert 'exception ' not in proof and 'image=\\\\?\\T:\\bin\\RCPP.EXE' in proof
    digits=('%06d'%pid)[-6:]
    assert 'native_temporary_input=RC'+digits+'\n' in proof
    assert 'native_temporary_output=RD'+digits+'\n' in proof
    stdout=Path(str(stem)+'.stdout').read_bytes();stderr=Path(str(stem)+'.stderr').read_bytes()
    assert stderr==b''
    if case=='verbose-names':
        assert stdout==verbose_bytes(pid)
        assert all('function_rva=%08x'%r in proof for r in (0x8110,0xdb80))
        output='verbose.res';expected_size=180
    else:
        assert stdout==b'' and 'target_PATH='+path+'\n' in proof and 'PATH_parser_input='+path+'\n' in proof
        assert all('function_rva=%08x'%r in proof for r in (0xed20,0xed50,0xeef0))
        output='path.res';expected_size=68
    observed={p.name:p.read_bytes() for p in work.iterdir()}
    assert set(observed)==set(initial)|{output} and all(observed[n]==d for n,d in initial.items())
    assert len(observed[output])==expected_size
    dest=HERE/'cases'/case
    if recording:
        dest.mkdir(parents=True,exist_ok=True)
        for kind in ('stdout','stderr'):shutil.copyfile(Path(str(stem)+'.'+kind),dest/(kind+'.bin'))
        shutil.copyfile(Path(str(stem)+'.proof'),dest/'proof.log')
        shutil.copyfile(work/output,dest/output)
        (dest/'capture.json').write_text(json.dumps(dict(native_pid=pid,arguments=arguments,exit=0,
            source_files=sorted(initial),remaining_files=sorted(observed),product=output,
            product_digest=digest(observed[output]),stdout_digest=digest(stdout),stderr_digest=digest(stderr)),indent=2)+'\n')
    else:
        assert observed[output]==(dest/output).read_bytes()
        if case=='path-fallback':assert stdout==(dest/'stdout.bin').read_bytes()
    return dict(case=case,pid=pid,stdout_bytes=len(stdout),stderr_bytes=len(stderr),output_bytes=len(observed[output]),exit=0)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',action='store_true');args=parser.parse_args()
    if args.record:assert not (HERE/'manifest.json').exists(), 'Frozen fixture: verification only'
    env,launch,path=prepare()
    for case in ('verbose-names','path-fallback'):print(json.dumps(capture(case,env,launch,path,args.record)),flush=True)
