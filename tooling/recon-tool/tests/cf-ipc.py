#!/usr/bin/env python3
"""Native x86 shared-map probe. Does not launch Wine or a reconstruction."""
import argparse
import atexit
import ctypes
import hashlib
import json
import mmap
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import time

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"watcom"))

from ipc import AtomicU32, Client, MAP_SIZE, MAX_PAYLOAD, ProtocolError


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def expect_error(action,contains):
    try:
        action()
    except ProtocolError as error:
        assert contains in str(error),(contains,str(error))
        return str(error)
    raise AssertionError("protocol violation was accepted: "+contains)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    scratch=Path.home()/".ai-tmp/msvc2/watcom-capture"
    scratch.mkdir(parents=True,exist_ok=True)
    results=[];children=[]
    def cleanup():
        # Only Popen-owned authored probe processes. Never kill a process group/server.
        for child in children:
            if child.poll() is None:
                child.kill()
            child.communicate(timeout=2)
    atexit.register(cleanup)
    total=time.monotonic()
    with tempfile.TemporaryDirectory(prefix="ipc-",dir=scratch) as temporary:
        work=Path(temporary);binary=work/"ipc-peer"
        command=["gcc","-m32","-O0","-g","-Wall","-Wextra","-Werror","-o",str(binary),str(root/"tests/cf-ipc-peer.c")]
        started=time.monotonic()
        build=subprocess.run(command,capture_output=True,text=True,timeout=20)
        build_seconds=time.monotonic()-started
        assert build.returncode==0,build.stderr
        binary_hash=sha(binary)

        def peer(client,mode="normal",deadline=1500,lease=250):
            child=subprocess.Popen([str(binary),str(client.path),mode,str(deadline),str(lease)],
                                   stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            children.append(child)
            return child

        def reap(child,status):
            stdout,stderr=child.communicate(timeout=3)
            assert child.returncode==status,(child.returncode,stdout,stderr)
            assert not stderr,stderr
            assert "pointer_bits=32" in stdout,stdout
            assert child.poll() is not None
            return stdout.strip()

        def record(name,action):
            started=time.monotonic();details=action()
            results.append({"name":name,"elapsed_seconds":time.monotonic()-started,
                            "details":details})

        counter=0
        def client(seconds=2):
            nonlocal counter
            counter+=1
            return Client(work/("channel-%d"%counter),seconds=seconds,heartbeat_seconds=.01)

        def coherence(mode):
            with client() as c:
                assert stat.S_IMODE(c.path.stat().st_mode)==0o600
                child=peer(c,mode)
                for i in range(64 if mode=="normal" else 1):
                    payload=bytes((j+i)&255 for j in range(MAX_PAYLOAD if i%2 else 17))
                    before=time.monotonic();response=c.request(1,payload)
                    assert response==bytes(value^0xa5 for value in payload)
                    if mode=="partial":assert time.monotonic()-before>=.045
                assert c.request(2)==b""
                return {"peer":reap(child,0),"messages":len(c.records)}
        record("python64_native_i386_coherence",lambda:coherence("normal"))
        record("partial_payload_unpublished",lambda:coherence("partial"))

        for mode,message in [("skip","skipped/replayed"),("oversized","oversized response"),
                             ("header","immutable header")]:
            def bad_reply(mode=mode,message=message):
                with client() as c:
                    child=peer(c,mode)
                    error=expect_error(lambda:c.request(1,b"test"),message)
                    c.stop.set();c.thread.join()
                    return {"error":error,"peer":reap(child,94 if mode=="header" else 74)}
            record("reject_"+mode+"_reply",bad_reply)

        def late():
            with client(.10) as c:
                child=peer(c,"late")
                error=expect_error(lambda:c.request(1,b"late"),"deadline")
                # A late reply must never make a failed channel reusable.
                time.sleep(.20)
                expect_error(lambda:c.request(2),"failed")
                c.stop.set();c.thread.join()
                return {"error":error,"peer":reap(child,74)}
        record("late_reply_sticky_failure",late)

        def global_deadline():
            with client() as c:
                child=peer(c,deadline=80,lease=250)
                return reap(child,73)
        record("heartbeat_cannot_extend_peer_deadline",global_deadline)

        def heartbeat_death():
            with client() as c:
                child=peer(c)
                c.stop.set();c.thread.join()
                return reap(child,74)
        record("owner_heartbeat_death",heartbeat_death)

        for value in (0,1000001):
            def initial_lease(value=value):
                with client() as c:
                    c.stop.set();c.thread.join();c.lease.set(value)
                    child=peer(c)
                    return reap(child,75)
            record("reject_initial_lease_%d"%value,initial_lease)

        def parent_death():
            path=work/"orphan-channel"
            source=("import os,sys,time;sys.path.insert(0,sys.argv[1]);"
                    "from ipc import Client;c=Client(sys.argv[2],seconds=2);"
                    "print('ready',flush=True);time.sleep(.15);os._exit(0)")
            owner=subprocess.Popen([sys.executable,"-c",source,str(root/"watcom"),str(path)],
                                   stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            children.append(owner)
            assert owner.stdout.readline().strip()=="ready"
            child=subprocess.Popen([str(binary),str(path),"normal","1500","250"],
                                   stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            children.append(child)
            owner.communicate(timeout=2);assert owner.returncode==0
            return reap(child,74)
        record("actual_controller_process_exit",parent_death)

        def replay():
            with client() as c:
                child=peer(c)
                c.request(1,b"one");c.request(1,b"two")
                c.command.set(1)
                return reap(child,76)
        record("reject_old_published_command_replay",replay)

        for field,value,status in [(12,2,76),(28,MAX_PAYLOAD+1,77),(20,0,75)]:
            def corrupt(field=field,value=value,status=status):
                with client() as c:
                    # Start peer before mutation and establish that it is alive via one round trip.
                    child=peer(c);c.request(1,b"ready")
                    if field==28:
                        import struct
                        struct.pack_into("<I",c.map,28,value)
                        c.command.set(2)
                    elif field==12:c.command.set(3)
                    else:c.stop.set();c.thread.join();c.lease.set(value)
                    return reap(child,status)
            record("reject_mutated_field_%d"%field,corrupt)

        def client_denials():
            with client() as c:
                errors=[expect_error(lambda:c.request(1,b"x"*(MAX_PAYLOAD+1)),"invalid"),
                        expect_error(lambda:c.request(0),"invalid")]
                c.lock.acquire()
                try:errors.append(expect_error(lambda:c.request(1),"concurrent"))
                finally:c.lock.release()
                c.reply.set(1)
                errors.append(expect_error(lambda:c.request(1),"stale"))
                errors.append(expect_error(lambda:c.request(1),"failed"))
                return errors
        record("client_schema_concurrency_stale_denials",client_denials)

        def no_wrap():
            from ipc import MAX_SEQUENCE
            with client() as c:
                c.sequence=MAX_SEQUENCE
                c.command.set(MAX_SEQUENCE);c.reply.set(MAX_SEQUENCE)
                error=expect_error(lambda:c.request(1),"exhausted")
                assert c.command.get()==MAX_SEQUENCE
                return error
        record("reject_command_sequence_wrap_before_publish",no_wrap)

        def header_sticky():
            import struct
            with client() as c:
                original=bytes(c.map[:12]);struct.pack_into("<I",c.map,0,0)
                error=expect_error(lambda:c.request(1),"header")
                c.map[:12]=original
                expect_error(lambda:c.request(1),"failed")
                return error
        record("restored_header_cannot_revive_failed_channel",header_sticky)

        def unaligned():
            with mmap.mmap(-1,MAP_SIZE) as area:
                return expect_error(lambda:AtomicU32(area,1),"unaligned")
        record("reject_unaligned_publication",unaligned)
        sources={str(path.relative_to(root)):sha(path) for path in
                 [root/"watcom/ipc.py",Path(__file__),root/"tests/cf-ipc-peer.c"]}
        receipt={"scope":"Native Python64/i386 Linux shared-map protocol only; no Win32 or application acceptance",
                 "python_pointer_bits":ctypes.sizeof(ctypes.c_void_p)*8,"sources":sources,
                 "build":{"command":command,"status":build.returncode,"elapsed_seconds":build_seconds,
                          "binary_sha256":binary_hash},"controls":results,
                 "elapsed_seconds":time.monotonic()-total,
                 "all_owned_children_terminated":all(child.poll() is not None for child in children)}
        assert receipt["python_pointer_bits"]==64
        assert receipt["all_owned_children_terminated"]
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(receipt,indent=2)+"\n")
    print("PASS: %d native IPC controls in %.3fs (build %.3fs); no Wine"%
          (len(results),receipt["elapsed_seconds"],build_seconds))
    cleanup();atexit.unregister(cleanup)


if __name__=="__main__":
    main()
