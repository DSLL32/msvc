#!/usr/bin/env python3
"""Bounded actual-PE controls for the two encountered typed pointer arrays."""
import argparse
import json
from pathlib import Path
import re
import struct
import tempfile
import time
from state import BINARY, TARGET, sha
from writer_evidence import verify_reuse_array


def reject_mutation(path,address,count,output):
    data=bytearray(path.read_bytes());header=struct.unpack_from('<I',data,60)[0];opt=header+24
    base=struct.unpack_from('<I',data,opt+28)[0];sections=opt+struct.unpack_from('<H',data,header+20)[0]
    changed=False
    for i in range(struct.unpack_from('<H',data,header+6)[0]):
        at=sections+i*40;virtual,rva,raw,file=struct.unpack_from('<IIII',data,at+8)
        if base+rva<=address and address+count*4<=base+rva+virtual:
            if address+count*4<=base+rva+raw:
                offset=file+address-base-rva
                assert data[offset:offset+count*4]==bytes(count*4)
                data[offset]=1;reason='defined file bytes changed from zero'
            else:
                assert address>=base+rva+raw
                struct.pack_into('<I',data,at+8,address+count*4-base-rva-1)
                reason='virtual zero tail shortened below last pointer byte'
            changed=True;break
    assert changed,'actual array not wholly owned by one actual PE section'
    assert bytes(data)!=path.read_bytes();output.write_bytes(data)
    try:verify_reuse_array(output,address,count)
    except AssertionError:return reason
    raise AssertionError('pointer initialization mutation accepted')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();start=time.monotonic()
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    checks=[(TARGET,0x49e6e0,8),(TARGET,0x497d00,16),(BINARY/'build/c2.exe',maps['_coff_section_buckets'],16)]
    for path,address,count in checks:verify_reuse_array(path,address,count)
    verify_reuse_array(BINARY/'build/c2.exe',maps['_coff_reuse_sections'],8)
    passed=[]
    with tempfile.TemporaryDirectory(prefix='coff-pointer-init-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(path,address,count)in enumerate(checks):
            reason=reject_mutation(path,address,count,Path(folder)/(str(i)+'.exe'))
            passed.append({'path':str(path),'target_sha256':sha(path.read_bytes()),'address':hex(address),'entries':count,'rejected':reason})
    try:verify_reuse_array(TARGET,0x497d00,9)
    except AssertionError:passed.append({'rejected':'unjustified nine-entry bound'})
    else:raise AssertionError('unjustified pointer-array count accepted')
    a.output.write_text(json.dumps({'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'predicate_sha256':sha((BINARY/'scripts/writer_evidence.py').read_bytes()),'scope':'Only actual eight-entry reuse array and sixteen-entry section buckets; target/realMAP addresses plus compiletime64B declaration remain separate owner evidence.'},indent=2)+'\n')

if __name__=='__main__':main()
