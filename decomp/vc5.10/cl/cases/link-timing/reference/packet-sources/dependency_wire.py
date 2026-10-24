"""CL's native-only DLL response transport, outside the application graph.

This socket has no original-executable or ordinary execution fallback. The case
controller owns semantic request checks and effects; a missing, unknown or extra
request closes the channel and fails the native callback.
"""
import hashlib
import socket
import struct
import threading

MAX_MESSAGE=2*1024*1024
BIND,LOAD,INVOKE,FREE=range(4)


def string(value):
    value=value.encode('latin1') if isinstance(value,str) else bytes(value)
    assert b'\0' not in value and len(value)<=32767
    return struct.pack('<I',len(value))+value


def configuration(case_hash,path,environment,os_environment,mbctype,ctype):
    """Data-only native host input. Shared code validates encountered CRT tables."""
    assert len(case_hash)==64 and all(c in '0123456789abcdef'for c in case_hash)
    assert len(environment)<=4096 and len(mbctype)==257 and len(ctype)==514
    assert len(os_environment)%2==0 and os_environment.endswith(b'\0\0\0\0')
    payload=b'RCH1'+struct.pack('<I',1)+case_hash.encode('ascii')+string(path)
    payload+=struct.pack('<I',len(environment))+b''.join(string(value)for value in environment)
    payload+=struct.pack('<I',len(os_environment)//2)+os_environment
    payload+=mbctype+ctype
    assert len(payload)<=MAX_MESSAGE
    return payload


class DllBroker:
    def __init__(self,config,case_hash,sequence,callback):
        assert sequence in ([BIND,LOAD,INVOKE],[BIND,LOAD,INVOKE,FREE])
        assert config[:4]==b'RCH1' and config[8:72]==case_hash.encode('ascii')
        self.config,self.case_hash,self.sequence,self.callback=config,case_hash,list(sequence),callback
        self.requests=[];self.payloads=[];self.attempts=[];self.errors=[];self.pid=None
        self._server,self._target=socket.socketpair()
        self._thread=threading.Thread(target=self._serve,daemon=True)
        self._thread.start()

    def fileno(self):return self._target.fileno()

    def environment(self):return {'CL_DEPENDENCY_FD':str(self.fileno())}

    @staticmethod
    def _read(stream,count,eof=False):
        data=bytearray()
        while len(data)<count:
            block=stream.recv(count-len(data))
            if not block and eof and not data:return None
            if not block:raise ValueError('incomplete CL DLL request')
            data.extend(block)
        return bytes(data)

    def _serve(self):
        try:
            while True:
                header=self._read(self._server,12,eof=True)
                if header is None:return
                magic,kind,length=struct.unpack('<4sII',header)
                if magic!=b'CLD1' or length<68 or length>MAX_MESSAGE:
                    raise ValueError('unknown CL DLL request framing')
                data=self._read(self._server,length)
                pid,=struct.unpack_from('<I',data)
                if not pid or (self.pid is not None and self.pid!=pid):raise ValueError('CL DLL request PID changed')
                self.pid=pid
                if data[4:68]!=self.case_hash.encode('ascii'):raise ValueError('CL DLL request uses a different response')
                if len(self.requests)>=len(self.sequence) or kind!=self.sequence[len(self.requests)]:
                    raise ValueError('missing, repeated or unordered CL DLL invocation')
                payload=data[68:]
                self.attempts.append((kind,pid,payload))
                if kind==BIND:
                    if payload!=self.config:raise ValueError('actual CL DLL configuration bytes differ')
                    result={'status':0,'effect':0,'errno':0,'doserrno':0,'last_error':0}
                else:
                    result=self.callback(kind,payload)
                    if set(result)!={'status','effect','errno','doserrno','last_error'}:
                        raise ValueError('incomplete CL DLL response')
                    if not all(type(value)is int for value in result.values()):
                        raise ValueError('untyped CL DLL response scalar')
                    if kind==INVOKE:
                        if result!={'status':0,'effect':1,'errno':0,'doserrno':2,'last_error':0}:
                            raise ValueError('unreviewed CL DLL Invoke result/state')
                    elif result!={'status':0,'effect':0,'errno':0,'doserrno':0,'last_error':0}:
                        raise ValueError('lifecycle hook attempted a target-state mutation')
                self.requests.append({'kind':kind,'pid':pid,'bytes':len(payload),
                                      'sha256':hashlib.sha256(payload).hexdigest()})
                self.payloads.append((kind,payload))
                self._server.sendall(struct.pack('<4s5I',b'CLR1',*(result[key]for key in
                    ('status','effect','errno','doserrno','last_error'))))
        except Exception as error:
            self.errors.append(repr(error))
            self._server.close()

    def check(self,pid):
        if self.errors:raise AssertionError('CL DLL response rejected: '+repr(self.errors))
        assert self.pid==pid and [r['kind']for r in self.requests]==self.sequence,'CL DLL invocation inventory differs'
        return list(self.requests)

    def close(self):
        self._target.close()
        try:self._server.shutdown(socket.SHUT_RDWR)
        except OSError:pass
        self._thread.join(timeout=1)
        self._server.close()
