#!/usr/bin/env python3
"""DRAFT explicit Wine coherence probe. Must receive command review before use."""
import hashlib
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import time

ROOT=Path('/root/msvc2')
WORK=Path('/root/.ai-tmp/msvc2/watcom-capture/win32-ipc-proposal')
sys.path.insert(0,str(ROOT/'tooling/watcom-capture'))
from ipc import Client, ProtocolError

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def windows(path):return 'S:\\'+str(Path(path).relative_to('/root/.ai-tmp/msvc2')).replace('/','\\')
def main():
    plan=json.loads((WORK/'plan.json').read_text())
    assert sha(plan['source'])==plan['source_sha256']
    assert sha(plan['link_response'])==plan['link_response_sha256']
    env=os.environ.copy();env.update(plan['environment_updates'])
    for key in plan['environment_remove']:env.pop(key,None)
    assert sha(ROOT/'toolchains/watcom10.0a/binnt/WCC386.EXE')=='8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d'
    assert sha(ROOT/'toolchains/watcom10.0a/binb/WCC386.EXE')=='c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b'
    assert sha(ROOT/'toolchains/watcom10.0a/.patched/WLINK.EXE')=='509c749b2258029a25a0071e34fa13b76a2fd4b267591a2bb12e9a7be5fd665b'
    def private_file(path,data):
        descriptor=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        os.fchmod(descriptor,0o600)
        with os.fdopen(descriptor,'wb') as stream:stream.write(data)
    snapshots=WORK/'snapshots';snapshots.mkdir(mode=0o700)
    sources=[Path(plan['source']),Path(__file__).resolve(),Path(plan['link_response']),
             ROOT/'tooling/watcom-capture/ipc.py',WORK/'plan.json']
    source_snapshots={}
    for source in sources:
        destination=snapshots/source.name
        private_file(destination,source.read_bytes())
        source_snapshots[str(source)]={'snapshot':str(destination),'sha256':sha(destination)}
    environment_bytes=(json.dumps(dict(sorted(env.items())),indent=2)+'\n').encode()
    environment_path=snapshots/'host-environment.json'
    private_file(environment_path,environment_bytes)
    build=WORK/'build';build.mkdir(exist_ok=True)
    records=[];children=[];began=time.monotonic()
    receipt={'scope':plan['scope'],'plan_sha256':sha(WORK/'plan.json'),
             'source_sha256':sha(plan['source']),'runner_sha256':sha(__file__),
             'ipc_sha256':sha(ROOT/'tooling/watcom-capture/ipc.py'),'commands':records,
             'immutable_source_snapshots':source_snapshots,
             'host_environment':{'private_path':str(environment_path),
                                 'sha256':sha(environment_path),'mode':'0600'}}
    def save():
        receipt['elapsed_seconds']=time.monotonic()-began
        (WORK/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    def run_build(name,command):
        start=time.monotonic()
        complete=subprocess.run(command,cwd=WORK,env=env,input=b'',capture_output=True,timeout=30)
        (WORK/(name+'.stdout')).write_bytes(complete.stdout)
        (WORK/(name+'.stderr')).write_bytes(complete.stderr)
        records.append({'name':name,'command':command,'status':complete.returncode,
                        'elapsed_seconds':time.monotonic()-start,
                        'stdout_sha256':hashlib.sha256(complete.stdout).hexdigest(),
                        'stderr_sha256':hashlib.sha256(complete.stderr).hexdigest()})
        save();assert complete.returncode==0,(name,complete.stdout,complete.stderr)
    def launch(channel,mode,deadline=1500):
        command=['/usr/bin/wine',windows(build/'ipc-win32-peer.exe'),windows(channel),mode,str(deadline),'400']
        child=subprocess.Popen(command,cwd=WORK,env=env,stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        children.append(child)
        return child,command,time.monotonic()
    def finish(name,owned,status):
        child,command,start=owned
        out,err=child.communicate(timeout=12)
        (WORK/(name+'.stdout')).write_text(out)
        (WORK/(name+'.stderr')).write_text(err)
        records.append({'name':name,'command':command,'status':child.returncode,
                        'elapsed_seconds':time.monotonic()-start,'stdout':out,'stderr':err,
                        'actual_owned_process_terminated':child.poll() is not None})
        save();assert child.returncode==status,(name,child.returncode,out,err)
        assert 'status=%d '%status in out and 'pointer_bits=32' in out,(name,out)
        assert not err,(name,err)
    def make_client(name):
        return Client(WORK/('channel-'+name+'.bin'),seconds=10,heartbeat_seconds=.01)
    try:
        run_build('compile',plan['compile']);run_build('link',plan['link'])
        receipt['artifacts']={p.name:sha(p) for p in build.iterdir() if p.is_file()}
        assert {'ipc-win32-peer.exe','ipc-win32-peer.obj','ipc-win32-peer.map'}<=set(receipt['artifacts'])
        save()
        for mode in ('normal','partial'):
            with make_client(mode) as client:
                child=launch(client.path,mode)
                for i in range(64 if mode=='normal' else 1):
                    payload=bytes((j+i)&255 for j in range(4096 if i%2 else 17))
                    start=time.monotonic();reply=client.request(1,payload)
                    assert reply==bytes(value^0xa5 for value in payload)
                    if mode=='partial':assert time.monotonic()-start>=.045
                assert client.request(2)==b''
                receipt[mode+'_messages']=client.records
                finish(mode,child,0)
        with make_client('skip') as client:
            child=launch(client.path,'skip')
            try:client.request(1,b'skip')
            except ProtocolError as error:
                assert 'skipped/replayed' in str(error)
                receipt['skip_error']=str(error)
            else:raise AssertionError('skipped reply accepted')
            client.stop.set();client.thread.join();finish('skip',child,74)
        with make_client('initial-zero') as client:
            client.stop.set();client.thread.join();client.lease.set(0)
            finish('initial-zero',launch(client.path,'normal'),75)
        owner_source=("import os,sys;from pathlib import Path;"
                      "sys.path.insert(0,sys.argv[1]);from ipc import Client;"
                      "c=Client(sys.argv[2],seconds=10,heartbeat_seconds=.01);"
                      "print('READY',flush=True);sys.stdin.readline();"
                      "assert c.request(1,b'owner-live')==bytes(v^0xa5 for v in b'owner-live');"
                      "print('PROVED',flush=True);sys.stdin.readline();os._exit(0)")
        owner_channel=WORK/'channel-owner-exit.bin'
        owner=subprocess.Popen([sys.executable,'-c',owner_source,str(ROOT/'tooling/watcom-capture'),str(owner_channel)],
                               stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        children.append(owner)
        def line():
            assert select.select([owner.stdout],[],[],12)[0],'controller handshake timeout'
            return owner.stdout.readline().strip()
        assert line()=='READY';child=launch(owner_channel,'normal')
        owner.stdin.write('ping\n');owner.stdin.flush();assert line()=='PROVED'
        owner.stdin.write('exit\n');owner.stdin.flush();owner.communicate(timeout=3)
        assert owner.returncode==0
        finish('actual-owner-process-exit',child,74)
        with make_client('global') as client:
            finish('global-deadline-with-live-heartbeat',launch(client.path,'normal',250),73)
        receipt['all_owned_processes_terminated']=all(c.poll() is not None for c in children)
        assert receipt['all_owned_processes_terminated'];receipt['passed']=True;save()
        print('PASS: six Win32 IPC-only probes; no application acceptance')
    finally:
        # Failures stop only our Popen-owned processes; never stop a Wine server.
        # A forced stop is recorded as failed proof, not successful helper termination.
        for child in children:
            if child.poll() is None:
                child.kill();child.communicate(timeout=3)
                receipt['forced_owned_process_stop']=True
        save()

if __name__=='__main__':main()
