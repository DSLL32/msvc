#!/usr/bin/env python3
"""Verify only the contextual descriptor rows named by the lexer source."""
import json,re,struct
from pathlib import Path
from importlib.util import spec_from_file_location,module_from_spec
HERE=Path(__file__).resolve().parents[1];ROOT=HERE.parents[2]
spec=spec_from_file_location('character_pe',HERE/'scripts/check-characters.py');pe=module_from_spec(spec);spec.loader.exec_module(pe)
def verify():
    original=ROOT/'toolchains/5.10/bin/RCDLL.DLL';rebuilt=HERE/'build/RCDLL.DLL';mapping=HERE/'build/rcdll.map';source=HERE/'src/token-info.c'
    old_base,old=pe.pe(original);base,new=pe.pe(rebuilt)
    symbols=re.findall(r'^\s*[0-9a-f]+:[0-9a-f]+\s+_rc_token_info\s+([0-9a-f]+)\s',mapping.read_text(),re.M|re.I);assert len(symbols)==1
    address=int(symbols[0],16)-base;rows=[]
    for text,kind,index in re.findall(r'\{L"([^"]*)",(\d+)\}, /\* token(\d+) \*/',source.read_text()):
        kind,index=int(kind),int(index);pointer=struct.unpack('<I',old(0x153d0+index*8,4))[0];value=old(0x153d4+index*8,1)[0];linked=struct.unpack('<I',new(address+index*8,4))[0];actual=new(address+index*8+4,1)[0]
        assert value==actual==kind
        expected=(text+'\0').encode('utf-16le');assert old(pointer-old_base,len(expected))==new(linked-base,len(expected))==expected
        rows.append(dict(index=index,kind=kind,description=text,original_rva=hex(pointer-old_base)))
    assert len(rows)==44 and len(set(row['index']for row in rows))==44
    return dict(status='pass',original_sha256=pe.sha(original),rebuilt_sha256=pe.sha(rebuilt),map_sha256=pe.sha(mapping),source_sha256=pe.sha(source),rows=rows,scope='Only44 explicitly named descriptor rows; other storage is unavailable. Only kind BYTE at offset4; three trailing padding bytes unclaimed. No original adjacent data or pointer identity is inferred.')
if __name__=='__main__':print(json.dumps(verify(),indent=2))
