"""Opt-in PID-owned full DWORD terminal status beside POSIX's low eight bits."""
import os
import struct
import tempfile


class TerminalStatus:
    def __init__(self):
        self.file=tempfile.TemporaryFile()
        self.file.write(bytes(24)); self.file.flush()

    @property
    def fd(self):
        return self.file.fileno()

    def environment(self):
        return {'RECON_STATUS_FD':str(self.fd)}

    def read(self,pid,posix_status):
        self.file.seek(0)
        data=self.file.read()
        if len(data)!=24:
            raise ValueError('malformed terminal receipt size')
        magic,version,owner,state,code,reserved=struct.unpack('<6I',data)
        if magic!=0x31535452 or version!=1 or owner!=pid or state!=1 or reserved:
            raise ValueError('missing, malformed or wrong-owner terminal receipt')
        if posix_status!=(code&255):
            raise ValueError('logical DWORD/POSIX status mismatch')
        return code

    def close(self):
        self.file.close()

    def __enter__(self):
        return self

    def __exit__(self,*unused):
        self.close()
