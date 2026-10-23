"""Verify the encountered temporary-file suffix grammar in all three images."""
import hashlib
import json
from importlib.machinery import SourceFileLoader
import struct
from binary import PE
from rules import ELF


def decode(image, start):
    records=[]
    for index in range(8):
        pointer,per_input=struct.unpack('<II',image.read(start+8*index,8))
        assert per_input in (0,1)
        if index==7:
            assert (pointer,per_input)==(0,0),'temporary suffix sentinel differs'
        else:
            assert pointer
            records.append({'suffix':image.string(pointer),'per_input':per_input})
    return records


def verify(project,root):
    proof=json.loads((project/'cases/minimal-c/temporary-suffixes.json').read_text())
    original=PE(root/'toolchains/5.10/bin/CL.EXE')
    assert hashlib.sha256(original.data).hexdigest()==proof['target_sha256']
    assert hashlib.sha256(original.read(proof['start'],64)).hexdigest()==proof['record_bytes_sha256']
    expected=decode(original,proof['start'])
    assert expected==proof['records']
    identity=SourceFileLoader('cl_temporary_map',str(project/'scripts/identify-runtime.py')).load_module()
    public=identity.public_map(project/'build/cl.map')
    assert decode(PE(project/'build/cl.exe'),public['_temporary_suffixes'])==expected,'rebuilt cleanup suffix table differs'
    native=ELF(project/'build/native-trace/CL.EXE')
    assert decode(native,native.symbols['temporary_suffixes'])==expected,'native cleanup suffix table differs'
    return {'records':7,'sentinel':'NULL,0','decoded_sha256':hashlib.sha256(json.dumps(expected,sort_keys=True).encode()).hexdigest()}
