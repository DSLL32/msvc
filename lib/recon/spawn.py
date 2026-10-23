"""Explicit native fixture process boundary; never captures or launches originals.

The callback receives mode, pid, file, cwd, argv, environment. Returning None
allows the shim's real native spawn; returning an integer replays that exit code
after the callback has checked inputs and applied captured side effects. Returning
exactly {'status': int, 'stdout': bytes, 'stderr': bytes} also replays raw captured
streams through the target's logical stdout/stderr handles, synchronously before
spawn returns. Stream bytes are never text-translated; stdout precedes stderr
when both handles refer to one destination. Their combined size is at most
64 MiB. Status is always an integer from 0 to 255 (not bool). Raising rejects the
request and fails the target. Full environment stays in memory only;
adapters should retain only fixture-relevant values in their evidence.

A captured failure to start a process uses {'status': -1, 'errno': int}, where
errno is 1..4095; optional stdout/stderr byte strings default to empty. This
restores the caller's CRT errno before returning -1 and never launches a child.
Positive-status dictionaries still require exactly status/stdout/stderr. The
explicit RECON_SPAWN_STATE_PROFILE=msvcrt510-wait-success-v1 (1252/Last2)
or msvcrt510-cp0-wait-success-v1 (initial CP0/Last0) uses RSP2 with actual
outer-entry parent_state and environment_source='target-os'. It requires the
exact status/stdout/stderr/parent_state response; parent_state has before/after
errno/doserrno/last_error maps, exactly0/2/2 ->0/0/2 (1252) or
0/2/0 ->0/0/0 (initial CP0), and status0. Unknown states,
namespace, result, shapes or types reject; RSP1 behavior is unchanged. Bare
integer -1, unknown keys and invalid errno values are rejected.
"""
import os
import socket
import struct
import threading

MAX_STREAM_BYTES = 64*1024*1024


class SpawnBroker:
    def __init__(self, callback):
        self.callback = callback
        self.requests = []
        self.errors = []
        self._server, self._target = socket.socketpair()
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()

    def fileno(self):
        return self._target.fileno()

    def environment(self):
        return {'RECON_SPAWN_FD': str(self.fileno())}

    @staticmethod
    def _read(stream, count, eof=False):
        result = bytearray()
        while len(result) < count:
            data = stream.recv(count-len(result))
            if not data and eof and not result:
                return None
            if not data:
                raise ValueError('incomplete spawn request')
            result.extend(data)
        return bytes(result)

    @staticmethod
    def _decode(payload, state_profile=False):
        if len(payload) < 16:
            raise ValueError('short spawn request')
        mode, pid, argc, envc = struct.unpack_from('<iIII', payload)
        if argc > 4096 or envc > 16384:
            raise ValueError('oversized spawn lists')
        offset = 16
        state = None
        if state_profile:
            if len(payload) < 32:
                raise ValueError('short spawn state profile request')
            error, dos, last, source = struct.unpack_from('<IIII', payload, offset)
            offset += 16
            if mode != 0 or (error, dos, last, source) not in ((0, 2, 2, 2), (0, 2, 0, 2)):
                raise ValueError('unreviewed spawn request state/namespace')
            state = dict(errno=error, doserrno=dos, last_error=last)

        def string():
            nonlocal offset
            if offset+4 > len(payload):
                raise ValueError('missing spawn string length')
            length, = struct.unpack_from('<I', payload, offset)
            offset += 4
            if offset+length > len(payload):
                raise ValueError('incomplete spawn string')
            result = os.fsdecode(payload[offset:offset+length])
            offset += length
            if '\0' in result:
                raise ValueError('embedded NUL in spawn string')
            return result

        file, cwd = string(), string()
        argv = [string() for _ in range(argc)]
        environment = [string() for _ in range(envc)]
        if offset != len(payload):
            raise ValueError('trailing spawn request data')
        result = dict(mode=mode, pid=pid, file=file, cwd=cwd,
                      argv=argv, environment=environment)
        if state_profile:
            result.update(parent_state=state, environment_source='target-os')
        return result

    def _serve(self):
        magic = b'RSP1'
        try:
            while True:
                header = self._read(self._server, 8, eof=True)
                if header is None:
                    return
                magic, length = struct.unpack('<4sI', header)
                if magic not in (b'RSP1', b'RSP2') or length > 4*1024*1024:
                    raise ValueError('invalid spawn protocol header')
                state_profile = magic == b'RSP2'
                request = self._decode(self._read(self._server, length), state_profile)
                result = self.callback(request)
                streams = None
                failure_errno = None
                parent_effect = None
                if state_profile:
                    if type(result) is not dict or set(result) != {'status', 'stdout', 'stderr', 'parent_state'}:
                        raise ValueError('RSP2 requires complete typed success response')
                    parent_effect = result['parent_state']
                    if type(parent_effect) is not dict or set(parent_effect) != {'before', 'after'}:
                        raise ValueError('RSP2 requires exact before/after parent state')
                    for label in ('before', 'after'):
                        row = parent_effect[label]
                        if type(row) is not dict or set(row) != {'errno', 'doserrno', 'last_error'} or any(type(x) is not int for x in row.values()):
                            raise ValueError('RSP2 parent state shape/types')
                    if (parent_effect['before'] != request['parent_state'] or
                        parent_effect['after'] != dict(errno=0, doserrno=0, last_error=request['parent_state']['last_error']) or
                        type(result['status']) is not int or result['status'] != 0):
                        raise ValueError('unreviewed RSP2 result/state effect')
                    result = {k:v for k,v in result.items() if k != 'parent_state'}
                if isinstance(result, dict):
                    if type(result.get('status')) is int and result['status'] == -1:
                        if not {'status', 'errno'} <= set(result) <= {'status', 'errno', 'stdout', 'stderr'}:
                            raise ValueError('mock spawn failure requires status and errno with optional streams')
                        failure_errno = result['errno']
                        if type(failure_errno) is not int or not 1 <= failure_errno <= 4095:
                            raise ValueError('mock spawn errno must be an integer from 1 to 4095')
                        streams = (result.get('stdout', b''), result.get('stderr', b''))
                    else:
                        if set(result) != {'status', 'stdout', 'stderr'}:
                            raise ValueError('mock process response requires status, stdout and stderr')
                        streams = (result['stdout'], result['stderr'])
                    if any(type(data) is not bytes for data in streams):
                        raise ValueError('mock process streams must be bytes')
                    if sum(map(len, streams)) > MAX_STREAM_BYTES:
                        raise ValueError('mock process streams are too large')
                    result = result['status']
                    if result is None:
                        raise ValueError('mock process status must be an integer from 0 to 255')
                if failure_errno is None and result is not None and (type(result) is not int or not 0 <= result <= 255):
                    raise ValueError('mock process status must be an integer from 0 to 255')
                # Do not retain a copy of the host environment in receipts.
                self.requests.append({**{k:v for k,v in request.items() if k != 'environment'},
                                      'action': 'native' if result is None else 'replay',
                                      'status': result})
                if failure_errno is not None:
                    self.requests[-1]['errno'] = failure_errno
                action = 5 if state_profile else 4 if failure_errno is not None else 3 if streams is not None else int(result is not None)
                self._server.sendall(struct.pack('<4sii', magic, action, result or 0))
                if state_profile:
                    self._server.sendall(struct.pack('<6I', *(parent_effect[label][key] for label in ('before', 'after') for key in ('errno', 'doserrno', 'last_error'))))
                    self.requests[-1]['parent_state_after'] = dict(parent_effect['after'])
                if failure_errno is not None:
                    self._server.sendall(struct.pack('<I', failure_errno))
                if streams is not None:
                    self._server.sendall(struct.pack('<II', *map(len, streams)))
                    for data in streams:
                        self._server.sendall(data)
        except Exception as error:
            self.errors.append(str(error))
            try:
                self._server.sendall(struct.pack('<4sii', magic, 2, 0))
            except OSError:
                pass

    def close(self):
        self._target.close()
        self._thread.join(timeout=5)
        if self._thread.is_alive():
            self._server.shutdown(socket.SHUT_RDWR)
            self._thread.join(timeout=1)
            self.errors.append('spawn broker did not finish after target exit')
        self._server.close()

    def check(self):
        if self.errors:
            raise AssertionError('; '.join(self.errors))

    def __enter__(self):
        return self

    def __exit__(self, *exception):
        self.close()
        if exception[0] is None:
            self.check()
