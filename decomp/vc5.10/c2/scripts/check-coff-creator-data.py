#!/usr/bin/env python3
"""Mutate the real rebuilt COFF creator's finite table and contextual operands."""
import argparse
import copy
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time
from coff_section_data import verify
from data_evidence import reader
from state import BINARY,sha


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();begin=time.monotonic();config=json.loads((BINARY/'build/trial.json').read_text());positive=verify(BINARY,config);passed=[]
    with tempfile.TemporaryDirectory(prefix='coff-creator-data-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        root=Path(folder);(root/'build').mkdir();(root/'src').mkdir()
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,root/'build'/name)
        shutil.copyfile(BINARY/'src/coff-output.c',root/'src/coff-output.c');path=root/'build/c2.exe';old=path.read_bytes();symbols={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(root/'build/c2.map').read_text(),re.M)};base=symbols['@create_coff_section@4'];rd=reader(path)
        def pos(address):
            pe=struct.unpack_from('<I',old,60)[0];opt=pe+24;image=struct.unpack_from('<I',old,opt+28)[0];section=opt+struct.unpack_from('<H',old,pe+20)[0]
            for i in range(struct.unpack_from('<H',old,pe+6)[0]):
                _,rva,size,at=struct.unpack_from('<IIII',old,section+40*i+8)
                if image+rva<=address<image+rva+size:return at+address-image-rva
            raise AssertionError('unbacked negative operand')
        cases=[('directive row redirected to data case',base+1251+5*4,struct.pack('<I',base+436)),('classmap kind10 remapped to code',base+1299+10,b'\0'),('default hole remapped to data case',base+1299+2,b'\1'),('directive flags cleared',base+670+12,struct.pack('<I',0)),('code flags changed',base+380+3,struct.pack('<I',0x60001020)),('xdata name changed to text name',base+762+3,rd(base+781+3,4)),('debug owner name field changed',base+649+5,b'\x14'),('default scalar owner shifted',base+975+2,struct.pack('<I',symbols['_coff_default_flags']+2)),('original initializedone scalar wrongly zeroed',symbols['_function_option_23'],bytes(4))]
        # Locate the actual code flag instruction, rather than guessing an offset.
        data=rd(base+321,115);pattern=b'\xc7\x45\xfc'+struct.pack('<I',0x60000020);assert data.count(pattern)==1;cases[4]=(cases[4][0],base+321+data.index(pattern)+3,cases[4][2])
        for name,address,value in cases:
            at=pos(address);changed=bytearray(old);assert changed[at:at+len(value)]!=value;changed[at:at+len(value)]=value;path.write_bytes(changed)
            try:verify(root,config)
            except AssertionError as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('finite creator operand mutation accepted: '+name)
            path.write_bytes(old)
        for name,change in [('classmap declared tail shortened',lambda r:r['rebuilt_text_data_offsets'][1].__setitem__(1,1319)),('external data ownership widened into padding',lambda r:r['shared'][0].__setitem__(0,0x41eeec)),('declared label identity altered',lambda r:r['rebuilt_dispatch_labels'].__setitem__(0,436))]:
            changed=copy.deepcopy(config);change(next(r for r in changed['driver']if r['name']=='@create_coff_section@4'))
            try:verify(root,changed)
            except AssertionError as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('finite creator metadata mutation accepted: '+name)
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'predicate_sha256':sha((BINARY/'scripts/coff_section_data.py').read_bytes()),'rebuilt_sha256':sha((BINARY/'build/c2.exe').read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
