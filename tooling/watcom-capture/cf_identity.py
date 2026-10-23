"""Thin CF byte identity verifier, not a code classifier or CFG discoverer.

Use only while every target thread is stopped. Actual stop/base provenance and
bounded memory reads belong to the transport; this module cannot establish them.
"""
from dataclasses import dataclass
import hashlib
import struct


class IdentityError(RuntimeError):
    pass


def sha(data):
    return hashlib.sha256(data).hexdigest()


@dataclass(frozen=True)
class Image:
    container_sha256: str
    header_offset: int
    load_offset: int
    loaded_size: int
    relocation_offset: int
    allocation_size: int
    entry_offset: int
    initialized: bytes
    fixups: tuple
    groups: int

    @classmethod
    def parse(cls,container,expected_sha256,expected_header):
        if not isinstance(container,bytes) or len(container)>16*1024*1024:
            raise IdentityError("invalid/bounded CF container")
        if sha(container)!=expected_sha256:
            raise IdentityError("immutable CF container hash mismatch")
        if len(container)<10 or container[:2]!=b"MZ":
            raise IdentityError("missing MZ wrapper")
        header=struct.unpack_from("<H",container,8)[0]*16
        if header<16 or header+24>len(container):
            raise IdentityError("CF header outside container")
        fields=struct.unpack_from("<6I",container,header)
        if tuple(expected_header)!=fields or fields[0]!=0x4643:
            raise IdentityError("CF header identity mismatch")
        _,load,size,relocation,allocation,entry=fields
        if (not 0<size<=8*1024*1024 or not size<=allocation<=16*1024*1024 or
                load<header+24 or load+size>len(container) or
                not 0<=entry<relocation<size):
            raise IdentityError("invalid CF image/header bounds")
        initialized=container[load:load+size]
        cursor=relocation;fixups=[];groups=0

        def word():
            nonlocal cursor
            if cursor+2>size:
                raise IdentityError("truncated CF relocation stream")
            value=struct.unpack_from("<H",initialized,cursor)[0];cursor+=2
            return value

        while True:
            count=word()
            if not count:break
            groups+=1
            if len(fixups)+count>200000:
                raise IdentityError("CF relocation budget exceeded")
            position=(word()<<16)|word()
            for index in range(count):
                if index:position+=word()
                if position+4>size:
                    raise IdentityError("CF relocation write outside loaded image")
                if position+4>relocation:
                    raise IdentityError("CF relocation overlaps its immutable stream")
                fixups.append(position)
        if cursor!=size:
            raise IdentityError("CF relocation terminator not at proved image end")
        return cls(expected_sha256,header,load,size,relocation,allocation,entry,
                   initialized,tuple(fixups),groups)

    def relocated(self,base):
        if not isinstance(base,int) or not 0<base<=0xffffffff or base+self.allocation_size>0x100000000:
            raise IdentityError("unproved/overflowing observed CF allocation base")
        expected=bytearray(self.initialized)
        # Stream order matters, including any repeated or overlapping write sites.
        # Relocated CONTENT can be outside allocation; only its write site is bounded.
        for offset in self.fixups:
            value=struct.unpack_from("<I",expected,offset)[0]
            struct.pack_into("<I",expected,offset,(value+base)&0xffffffff)
        return bytes(expected)

    def compare(self,actual,base):
        if not isinstance(actual,bytes) or len(actual)!=self.loaded_size:
            raise IdentityError("actual initialized CF snapshot size mismatch")
        expected=self.relocated(base)
        if actual!=expected:
            mismatch=next(index for index,(left,right) in enumerate(zip(actual,expected)) if left!=right)
            raise IdentityError("CF initialized byte mismatch at +0x%x: actual %02x expected %02x"%
                                (mismatch,actual[mismatch],expected[mismatch]))
        return {"container_sha256":self.container_sha256,
                "header_offset":self.header_offset,"load_offset":self.load_offset,
                "loaded_size":self.loaded_size,"allocation_size":self.allocation_size,
                "entry_offset":self.entry_offset,"observed_base":base,
                "relocation_offset":self.relocation_offset,"groups":self.groups,
                "fixup_count":len(self.fixups),"initialized_sha256":sha(self.initialized),
                "relocation_stream_sha256":sha(self.initialized[self.relocation_offset:]),
                "actual_relocated_sha256":sha(actual),"expected_relocated_sha256":sha(expected),
                "compared_initialized_bytes":len(actual),"code_ownership_inferred":False}
