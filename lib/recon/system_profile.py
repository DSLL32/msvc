"""Encode an explicitly captured Win32 hardware environment; no host queries.

The reference SHA binds the owning capture/environment receipt. Adapters validate
that provenance and consumed fields. Volatile free-memory bytes are one captured
sample, not an invariant or permission to normalize new target decisions.
"""
import re
import struct
from pathlib import Path


def encode_profile(reference_sha256, system_info_bytes, memory_status_bytes):
    if not isinstance(reference_sha256,str) or not re.fullmatch('[0-9a-f]{64}',reference_sha256):
        raise ValueError('invalid system profile reference SHA256')
    if not isinstance(system_info_bytes,bytes) or len(system_info_bytes)!=36 or not isinstance(memory_status_bytes,bytes) or len(memory_status_bytes)!=32:
        raise ValueError('invalid Win32 system profile structure size')
    page=struct.unpack_from('<I',system_info_bytes,4)[0]
    if not page or page&(page-1) or struct.unpack_from('<I',memory_status_bytes)[0]!=32:
        raise ValueError('invalid captured system profile shape')
    return b'RSE1'+reference_sha256.encode('ascii')+system_info_bytes+memory_status_bytes


def write_profile(path, reference_sha256, system_info_bytes, memory_status_bytes):
    Path(path).write_bytes(encode_profile(reference_sha256,system_info_bytes,memory_status_bytes))
