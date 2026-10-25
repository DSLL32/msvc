"""Bounded x86 shared-map command channel; no target execution operations.

This module is the Linux client half only. A passing native peer test does not
establish Windows InterlockedExchange/debug-event behavior.
"""
import ctypes
import mmap
import os
from pathlib import Path
import platform
import struct
import threading
import time

MAGIC=0x50434757
VERSION=1
MAP_SIZE=16384
MAX_PAYLOAD=4096
PAYLOAD=256
COMMAND_SEQ=12
REPLY_SEQ=16
LEASE_SEQ=20
COMMAND_OP=24
COMMAND_LEN=28
REPLY_STATUS=32
REPLY_LEN=36
MAX_SEQUENCE=1000000


def reply_wait(start):
    """Brief active handoff, then yield; long/debugger waits sleep normally.

    Only called inside a live synchronous request. Header, sequence, lease and
    absolute deadline checks remain in its surrounding loop on every pass.
    """
    elapsed = time.monotonic() - start
    if elapsed >= 0.002:
        time.sleep(0.001)
        return 'sleep'
    if elapsed >= 0.00005:
        os.sched_yield()
        return 'yield'
    return 'spin'


class ProtocolError(RuntimeError):
    pass


class AtomicU32:
    """An actual aligned ctypes DWORD load/store on supported x86 hosts."""
    def __init__(self,buffer,offset):
        if platform.machine().lower() not in {"x86_64","amd64","i386","i486","i586","i686"}:
            raise ProtocolError("shared-map protocol requires native x86")
        self.cell=ctypes.c_uint32.from_buffer(buffer,offset)
        if offset%4 or ctypes.addressof(self.cell)%4:
            self.cell=None
            raise ProtocolError("unaligned atomic DWORD")

    def get(self):
        return self.cell.value

    def set(self,value):
        if not isinstance(value,int) or not 0<=value<=0xffffffff:
            raise ProtocolError("atomic DWORD overflow")
        self.cell.value=value

    def close(self):
        self.cell=None


class Client:
    def __init__(self,path,seconds=30,heartbeat_seconds=0.05):
        if not 0<seconds<=300 or not 0<heartbeat_seconds<=0.5:
            raise ProtocolError("invalid protocol deadline/heartbeat")
        self.path=Path(path)
        descriptor=os.open(self.path,os.O_RDWR|os.O_CREAT|os.O_EXCL,0o600)
        os.fchmod(descriptor,0o600)
        self.file=os.fdopen(descriptor,"w+b")
        self.file.truncate(MAP_SIZE)
        self.map=mmap.mmap(self.file.fileno(),MAP_SIZE,access=mmap.ACCESS_WRITE)
        struct.pack_into("<III",self.map,0,MAGIC,VERSION,MAP_SIZE)
        self.command=AtomicU32(self.map,COMMAND_SEQ)
        self.reply=AtomicU32(self.map,REPLY_SEQ)
        self.lease=AtomicU32(self.map,LEASE_SEQ)
        self.lease.set(1)
        self.sequence=0
        self.deadline=time.monotonic()+seconds
        self.owner=os.getpid()
        self.closed=False
        self.lock=threading.Lock()
        self.failed=False
        self.records=[]
        self.wait_counts={'spin':0,'yield':0,'sleep':0}
        self.stop=threading.Event()
        self.heartbeat_error=None

        def heartbeat():
            value=1
            while not self.stop.wait(heartbeat_seconds):
                value+=1
                if value>MAX_SEQUENCE:
                    self.heartbeat_error="heartbeat sequence exhausted"
                    return
                self.lease.set(value)
        self.thread=threading.Thread(target=heartbeat,name="watcom-capture-lease",daemon=True)
        self.thread.start()

    def _header(self):
        if struct.unpack_from("<III",self.map,0)!=(MAGIC,VERSION,MAP_SIZE):
            raise ProtocolError("shared-map immutable header changed")

    def request(self,operation,payload=b""):
        if not self.lock.acquire(blocking=False):
            raise ProtocolError("concurrent protocol client")
        try:
            return self._request(operation,payload)
        finally:
            self.lock.release()

    def _request(self,operation,payload):
        if (self.closed or self.failed or os.getpid()!=self.owner or self.heartbeat_error):
            raise ProtocolError("closed, failed or foreign protocol client")
        if (not isinstance(operation,int) or not 1<=operation<=0xffff or
                not isinstance(payload,bytes) or len(payload)>MAX_PAYLOAD):
            raise ProtocolError("invalid operation/payload size")
        try:
            self._header()
        except ProtocolError:
            self.failed=True
            raise
        if self.command.get()!=self.sequence or self.reply.get()!=self.sequence:
            self.failed=True
            raise ProtocolError("stale or unsolicited protocol sequence")
        if self.sequence>=MAX_SEQUENCE or time.monotonic()>=self.deadline:
            self.failed=True
            raise ProtocolError("command sequence/deadline exhausted")
        self.sequence+=1
        expected=self.sequence
        start=time.monotonic()
        try:
            self.map[PAYLOAD:PAYLOAD+len(payload)]=payload
            struct.pack_into("<II",self.map,COMMAND_OP,operation,len(payload))
            self.command.set(expected)  # Publish only after complete payload.
            while True:
                self._header()
                if time.monotonic()>=self.deadline or self.heartbeat_error:
                    raise ProtocolError("broker response deadline/lease failure")
                current=self.reply.get()
                if current==expected:
                    status,size=struct.unpack_from("<II",self.map,REPLY_STATUS)
                    if size>MAX_PAYLOAD:
                        raise ProtocolError("oversized response")
                    response=bytes(self.map[PAYLOAD:PAYLOAD+size])
                    if (self.reply.get()!=expected or self.command.get()!=expected or
                            struct.unpack_from("<II",self.map,REPLY_STATUS)!=(status,size)):
                        raise ProtocolError("response changed during consumption")
                    self._header()
                    if time.monotonic()>=self.deadline:
                        raise ProtocolError("response arrived after global deadline")
                    self.records.append({"sequence":expected,"operation":operation,
                                         "request_bytes":len(payload),"response_bytes":size,
                                         "status":status,"elapsed_seconds":time.monotonic()-start})
                    if status:
                        raise ProtocolError("broker rejected command with status %d"%status)
                    return response
                if current!=expected-1:
                    raise ProtocolError("skipped/replayed response sequence")
                self.wait_counts[reply_wait(start)] += 1
        except BaseException:
            self.failed=True
            raise

    def close(self):
        if self.closed:
            return
        if not self.lock.acquire(blocking=False):
            raise ProtocolError("cannot close an outstanding request")
        try:
            self.stop.set();self.thread.join(timeout=1)
            if self.thread.is_alive():
                raise ProtocolError("heartbeat did not stop")
            self.command.close();self.reply.close();self.lease.close()
            self.map.close();self.file.close();self.closed=True
        finally:
            self.lock.release()

    def __enter__(self):
        return self

    def __exit__(self,*_):
        self.close()
