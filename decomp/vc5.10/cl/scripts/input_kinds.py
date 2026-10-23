"""Verify the encountered extension/category/pass-list table in three images."""
import hashlib
import importlib.machinery
import json
from pathlib import Path
import struct
from binary import PE
from rules import ELF


def decode(image,start):
    rows=[];identities={}
    for i in range(11):
        extension,category,passes=struct.unpack('<IiI',image.read(start+12*i,12))
        if i==10:
            assert (extension,category,passes)==(0,-1,0),'wrong input-kind terminator'
            break
        assert extension and passes
        if passes not in identities:identities[passes]=len(identities)
        sequence=[]
        for j in range(6):
            value=struct.unpack('<i',image.read(passes+4*j,4))[0]
            assert 0<=value<=5,'invalid compiler pass'
            sequence.append(value)
            if value==5:break
        else:raise AssertionError('unterminated pass list')
        rows.append({'extension':image.string(extension),'category':category,
            'pass_list_identity':identities[passes],'passes':sequence})
    return rows


def decode_passes(image,start):
    rows=[]
    for i in range(5):
        words=struct.unpack('<9I',image.read(start+36*i,36))
        assert not words[1] and not any(words[5:]),'unexpected initialized compiler-pass pointer'
        rows.append({'name':image.string(words[0])if words[0]else None,
            'environment':image.string(words[2])if words[2]else None,
            'prefix':struct.pack('<I',words[3]).split(b'\0')[0].decode('ascii'),
            'flags':words[4],'path':None,'module':None,'invoke':None,'abort':None,'options':None})
    return rows


def verify(project,root):
    proof=json.loads((project/'cases/minimal-c/input-kinds.json').read_text())
    original_path=root/'toolchains/5.10/bin/CL.EXE';original=PE(original_path)
    assert hashlib.sha256(original.data).hexdigest()==proof['target_sha256']
    assert hashlib.sha256(original.read(proof['start'],132)).hexdigest()==proof['record_bytes_sha256']
    expected=decode(original,proof['start'])
    assert expected==proof['records'],'original typed input records differ'
    identity=importlib.machinery.SourceFileLoader('cl_input_kind_map',str(project/'scripts/identify-runtime.py')).load_module()
    public=identity.public_map(project/'build/cl.map')
    rebuilt=PE(project/'build/cl.exe');native=ELF(project/'build/native-trace/CL.EXE')
    assert decode(rebuilt,public['_input_kinds'])==expected,'rebuilt input-kind fields/pass lists/aliases differ'
    assert decode(native,native.symbols['input_kinds'])==expected,'native input-kind fields/pass lists/aliases differ'
    passes=proof['compiler_passes'];expected_passes=decode_passes(original,passes['start'])
    assert expected_passes==passes['records']
    assert decode_passes(rebuilt,public['_compiler_passes'])==expected_passes,'rebuilt compiler-pass records differ'
    assert decode_passes(native,native.symbols['compiler_passes'])==expected_passes,'native compiler-pass records differ'
    return {'records':10,'terminator':'NULL,-1,NULL','pass_list_identities':4,
        'decoded_sha256':hashlib.sha256(json.dumps(expected,sort_keys=True).encode()).hexdigest(),
        'compiler_passes_sha256':hashlib.sha256(json.dumps(expected_passes,sort_keys=True).encode()).hexdigest()}
