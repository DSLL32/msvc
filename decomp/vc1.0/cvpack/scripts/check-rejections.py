#!/usr/bin/env python3
"""Meaningful negative checks for acceptance/identity failure paths."""
from graph import graph, names
from runtime import identify
from state import ROOT
from state import BINARY
from binary import PE
from audit import publics
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from pathlib import Path
import struct
import subprocess
import tempfile

class Bytes:
    def __init__(self,data):self.data=data
    def read(self,address,size):return self.data[address:address+size]

def reject(label, action):
    try:action()
    except (AssertionError,ValueError,IndexError,KeyError):
        print('rejected:',label)
        return
    raise AssertionError('accepted invalid '+label)

def main():
    reject('truncated RET cleanup',lambda:graph(Bytes(b'\xc2\x04\x00'),0,2))
    reject('truncated immediate',lambda:graph(Bytes(b'\xb8\0\0\0\0\xc3'),0,3))
    reject('missing terminal',lambda:graph(Bytes(b'\x90\xc3'),0,1))
    reject('branch into operand',lambda:graph(Bytes(b'\x75\x01\xb8\0\0\0\0\xc3'),0,8))
    reject('undeclared indirect jump',lambda:graph(Bytes(b'\xff\xe0'),0,2))
    reject('conditional outside function',lambda:graph(Bytes(b'\x75\x20\xc3'),0,3))
    reject('branch into text data',lambda:graph(Bytes(b'\xeb\0\xc3'),0,3,data=[(2,3)]))
    reject('unresolved application call',lambda:names({0x12345678},{}))
    positive=graph(Bytes(b'\x74\x01\x90\xc2\x08\x00'),0,6)
    assert positive['returns']=={8} and positive['end']==6
    # A tiny wrapper with identical nonrelocation bytes must fail when its
    # external `_main` target is mapped to the wrong application identity.
    target=ROOT/'toolchains/1.0-nt/bin/CVPACK.EXE'
    good=identify(target,{'_main':0x17000})
    bad=identify(target,{'_main':0x17001})
    assert any(r['name']=='_mainCRTStartup' and r['start']==0x2c260 for r in good['functions'])
    assert not any(r['name']=='_mainCRTStartup' and r['start']==0x2c260 for r in bad['functions'])
    print('rejected: CRT startup with incorrect relocated application target')
    # Mutate actual compiler output, leaving originals/source/build products intact.
    rebuilt=BINARY/'build/CVPACK.EXE'
    if rebuilt.is_file():
        pe=PE(rebuilt)
        symbols=publics()
        start=symbols['_main']
        end=min(address for address in symbols.values() if address>start)
        instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(start,end-start),start))
        call=next(i for i in instructions if i.mnemonic=='call' and i.op_str=='0x%x'%symbols['_parse_options'])
        terminal=next(i for i in reversed(instructions) if i.mnemonic=='ret')
        assert terminal.address+3<=end
        mutations={}
        image=bytearray(pe.data)
        struct.pack_into('<i',image,pe.offset(call.address)+1,symbols['_print_banner']-call.address-5)
        mutations['redirected application callee']=image
        image=bytearray(pe.data)
        image[pe.offset(terminal.address):pe.offset(terminal.address)+3]=b'\xc2\x04\x00'
        mutations['changed callee stack cleanup']=image
        image=bytearray(pe.data)
        image[pe.offset(symbols['_show_logo'])]^=1
        mutations['changed contextual global initializer']=image
        scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
        scratch.mkdir(exist_ok=True,parents=True)
        with tempfile.TemporaryDirectory(prefix='audit-rejections-',dir=scratch) as directory:
            for label,image in mutations.items():
                modified=Path(directory)/'modified.exe'
                modified.write_bytes(image)
                result=subprocess.run(['python3',str(BINARY/'scripts/audit.py'),'--canonical-only','--rebuilt',str(modified)],capture_output=True)
                assert result.returncode!=0,'accepted '+label
                assert b'AssertionError' in result.stderr,result.stderr
                print('rejected:',label)

if __name__=='__main__':main()
