"""Explicit native fixture process boundary; never captures or launches originals.

The callback receives mode, pid, file, cwd, argv, environment. Returning None
allows the shim's real native spawn; returning an integer replays that exit code
after the callback has checked inputs and applied captured side effects. Raising
rejects the request and fails the target. Full environment stays in memory only;
adapters should retain only fixture-relevant values in their evidence.
"""
import os
import socket
import struct
import threading


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
    def _decode(payload):
        if len(payload) < 16:
            raise ValueError('short spawn request')
        mode, pid, argc, envc = struct.unpack_from('<iIII', payload)
        if argc > 4096 or envc > 16384:
            raise ValueError('oversized spawn lists')
        offset = 16

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
        return dict(mode=mode, pid=pid, file=file, cwd=cwd,
                    argv=argv, environment=environment)

    def _serve(self):
        try:
            while True:
                header = self._read(self._server, 8, eof=True)
                if header is None:
                    return
                magic, length = struct.unpack('<4sI', header)
                if magic != b'RSP1' or length > 4*1024*1024:
                    raise ValueError('invalid spawn protocol header')
                request = self._decode(self._read(self._server, length))
                result = self.callback(request)
                if result is not None and (type(result) is not int or not 0 <= result <= 255):
                    raise ValueError('mock process status must be an integer from 0 to 255')
                # Do not retain a copy of the host environment in receipts.
                self.requests.append({**{k:v for k,v in request.items() if k != 'environment'},
                                      'action': 'native' if result is None else 'replay',
                                      'status': result})
                self._server.sendall(struct.pack('<4sii', b'RSP1', int(result is not None), result or 0))
        except Exception as error:
            self.errors.append(str(error))
            try:
                self._server.sendall(struct.pack('<4sii', b'RSP1', 2, 0))
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
