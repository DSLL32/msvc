"""Prepare encountered-only native module resources; never run Wine or a PE helper.

Adapters must verify source/hash/offset/language/encoding evidence before encoding.
Entries have kind='string' or 'message', id, language=1033, text (ASCII str/bytes).
The owner SHA is the registered logical original target, not a transport copy.
"""
import re
import struct
from pathlib import Path


def encode_resources(target_sha256, entries):
    if not isinstance(target_sha256,str) or not re.fullmatch('[0-9a-f]{64}',target_sha256):
        raise ValueError('invalid resource owner SHA256')
    entries=list(entries)
    if len(entries)>4096:
        raise ValueError('too many encountered entries')
    result=bytearray(b'RMR1'+struct.pack('<I',len(entries))+target_sha256.encode('ascii'))
    seen=set()
    for entry in entries:
        kind={'string':1,'message':2}.get(entry['kind'])
        identity=(kind,entry['id'],entry['language'])
        if kind is None or not 0<=entry['id']<=0xffffffff or entry['language']!=1033 or identity in seen:
            raise ValueError('invalid or duplicate encountered resource identity')
        seen.add(identity)
        text=entry['text']
        if isinstance(text,str):
            text=text.encode('ascii')
        if not isinstance(text,bytes) or len(text)>65535 or any(not c or c>127 for c in text):
            raise ValueError('only observed ASCII logical text is supported')
        result.extend(struct.pack('<IIII',kind,entry['id'],entry['language'],len(text)))
        result.extend(text)
    if len(result)>1048576:
        raise ValueError('resource provider exceeds limit')
    return bytes(result)


def write_resources(path,target_sha256,entries):
    Path(path).write_bytes(encode_resources(target_sha256,entries))
