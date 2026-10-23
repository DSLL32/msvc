#!/usr/bin/env python3
"""Check the encountered directive dictionary as typed rows, not image data."""
import hashlib,json,re,struct
from pathlib import Path
from importlib.util import spec_from_file_location,module_from_spec
HERE=Path(__file__).resolve().parents[1];ROOT=HERE.parents[2]
spec=spec_from_file_location('lexical_pe',HERE/'scripts/check-characters.py');pe=module_from_spec(spec);spec.loader.exec_module(pe)
def verify():
    original=ROOT/'toolchains/5.10/bin/RCDLL.DLL';rebuilt=HERE/'build/RCDLL.DLL';mapfile=HERE/'build/rcdll.map';mapping=mapfile.read_text()
    old_base,old=pe.pe(original);base,new=pe.pe(rebuilt)
    def address(name):
        rows=re.findall(r'^\s*[0-9a-f]+:[0-9a-f]+\s+'+re.escape(name)+r'\s+([0-9a-f]+)\s',mapping,re.M|re.I);assert len(rows)==1;return int(rows[0],16)-base
    def wide(read,rva):
        raw=bytearray()
        for i in range(32):
            word=read(rva+2*i,2);raw.extend(word)
            if word==b'\0\0':return bytes(raw)
        raise AssertionError('unterminated directive suffix')
    groups=old(0x14a50,29);assert new(address('_rc_directive_groups'),29)==groups
    assert groups[0]==0 and groups[-1]==13 and all(a<=b for a,b in zip(groups,groups[1:]))
    table=address('_rc_directive_names');rows=[]
    for i in range(13):
        pointer=struct.unpack('<I',old(0x14a18+i*4,4))[0];linked=struct.unpack('<I',new(table+i*8,4))[0]
        suffix=wide(old,pointer-old_base);assert wide(new,linked-base)==suffix
        code=old(0x14a70+i,1)[0];assert new(table+i*8+4,1)[0]==code
        rows.append(dict(index=i,original_rva=hex(pointer-old_base),rebuilt_rva=hex(linked-base),suffix=suffix.decode('utf-16le')[:-1],code=code))
    return dict(status='pass',original_sha256=pe.sha(original),rebuilt_sha256=pe.sha(rebuilt),map_sha256=pe.sha(mapfile),source_sha256=pe.sha(HERE/'src/directive-names.c'),group_table=dict(original_rva='0x14a50',rebuilt_rva=hex(address('_rc_directive_groups')),bytes=29,first_character=95,last_character=122,sentinel_character=123,sha256=hashlib.sha256(groups).hexdigest()),names=rows,scope='Only29 group boundaries,13 suffix pointers/texts and13 result bytes. Rebuilt row padding is not compared; original adjacent high-index data is unavailable and native source holds first>z.')
if __name__=='__main__':print(json.dumps(verify(),indent=2))
