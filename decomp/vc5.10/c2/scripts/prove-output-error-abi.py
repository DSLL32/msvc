#!/usr/bin/env python3
"""Fixed named original/Win32/native callback ABI bytes; no graph discovery."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess
from state import BINARY, TARGET, sha
from data_evidence import reader, cstring


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    spec=importlib.util.spec_from_file_location('c2_elf_proof',BINARY/'scripts/native-proof.py');elf=importlib.util.module_from_spec(spec);spec.loader.exec_module(elf)
    binary=BINARY/'build/native/c2';data=binary.read_bytes();symbols=elf.symbols(binary,True)
    def native(at,size):
        pos=struct.unpack_from('<I',data,28)[0];width,count=struct.unpack_from('<HH',data,42);found=[]
        for i in range(count):
            kind,fileoff,address,_,filesize,_,flags,_=struct.unpack_from('<IIIIIIII',data,pos+i*width)
            if kind==1 and address<=at and at+size<=address+filesize:found.append(data[fileoff+at-address:fileoff+at-address+size])
        assert len(found)==1;return found[0]
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    win=reader(BINARY/'build/c2.exe');actual=reader(TARGET);logical='_writer_callback_48511c';name='writer_callback_48511c';callee=maps['@'+name+'@12'];writer=maps['@write_output_bytes@8'];slot=maps['_output_error_handler']
    # Original and canonical /Od caller have the same finite21B ABI sequence.
    def call_contract(read,site,expected_slot):
        raw=read(site-15,21);assert raw[0]==0x68 and raw[5]==0xba and raw[10]==0xb9
        assert int.from_bytes(raw[1:5],'little')==3254 and int.from_bytes(raw[11:15],'little')==14
        assert raw[15:]==b'\xff\x15'+struct.pack('<I',expected_slot)
        filename=int.from_bytes(raw[6:10],'little');assert cstring(read,filename)==r'E:\utc\src\\P2\coffemit.c'
        return {'call':site,'slot':expected_slot,'line':3254,'error':14,'filename':filename,'filename_text':cstring(read,filename),'bytes':raw.hex(),'sha256':sha(raw),'normal_cleanup':'calleeRET4, no caller argument pop'}
    original=call_contract(actual,0x469bf0,0x4999f4);rebuilt=call_contract(win,writer+0x59,slot)
    stub=win(callee,25);assert stub==bytes.fromhex('558bec83ec085356578955f8894dfccc5f5e5b8be55dc20400')
    nstart,nsize=symbols[name];ncode=native(nstart,nsize);assert ncode.count(b'\xcc')==1 and ncode[-3:]==b'\xc2\x04\0'
    assert ncode[0x16:0x1c]==bytes.fromhex('8955f0894dec')and ncode[0x37:0x43]==bytes.fromhex('8b4dec8b55f0894df88955f4')
    # Exact current native PIC call sequence: ECX14, EDX literal address,
    # oneDWORDline at ESP, indirect call through the same four-byte global.
    wstart,wsize=symbols['write_output_bytes'];got=int(re.findall(r'^([0-9a-f]+) [a-zA-Z] _GLOBAL_OFFSET_TABLE_$',subprocess.check_output(['nm','--defined-only',str(binary)],text=True),re.M)[0],16)
    code=native(wstart+0x8b,32)
    assert code[:8]==bytes.fromhex('8b45e0b90e000000')and code[8:10]==b'\x8d\x90'
    literal=got+struct.unpack_from('<i',code,10)[0];assert cstring(native,literal)==original['filename_text']
    assert code[14:25]==bytes.fromhex('c70424b60c000089c3ff90')
    assert got+struct.unpack_from('<i',code,25)[0]==symbols['output_error_handler'][0]
    assert code[29:]==bytes.fromhex('83ec04') # final E9 begins next basic block; stack reservation restored after callee pop.
    observed=elf.verify_callsite(binary,'write_output_bytes',0x8b,code)
    a.output.write_text(json.dumps({'target_sha256':sha(TARGET.read_bytes()),'win32_sha256':sha((BINARY/'build/c2.exe').read_bytes()),'map_sha256':sha((BINARY/'build/c2.map').read_bytes()),'native_sha256':sha(data),'original':original,'rebuilt':rebuilt,'rebuilt_stub':{'start':callee,'bytes':stub.hex(),'int3_offset':15,'ret4_offset':22,'body_credit':0},'native_stub':{'start':nstart,'size':nsize,'sha256':sha(ncode),'int3_offset':67,'ret4_offset':94,'register_formals':'ECX/EDX saved around entry tracer then restored into genuine formals; stack line EBP+8','body_credit':0},'native_caller':dict(observed,slot=symbols['output_error_handler'][0],literal=literal,got=got),'scope':'Fixed ABI instruction windows only. Actualerrorcall is UNREACHED in numeric case; callbackremains namedINT3/masked/no bodycredit. Bothoriginal54Bpaths RET4; targetignores EDX/line without erasing actualcallerinputs. No wrapper or fake register convention.','inputs':{str(p.relative_to(BINARY)):sha(p.read_bytes())for p in [BINARY/'src/coff-output.c',BINARY/'src/coff-output.h',BINARY/'src/stubs.c',BINARY/'scripts/writer_evidence.py',Path(__file__),BINARY/'cases/analysis/output-error-abi.json',BINARY/'cases/analysis/object-output-initialization.json']}},indent=2)+'\n')

if __name__=='__main__':main()
