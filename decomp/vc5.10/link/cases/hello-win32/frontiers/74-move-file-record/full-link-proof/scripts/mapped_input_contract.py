"""Finite, original-body-bound BYTE input proof; this grants no accepted code."""
import functools
import hashlib
import json
from pathlib import Path
from binary import PE


@functools.lru_cache(maxsize=1)
def verify():
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    from capstone.x86 import X86_OP_MEM,X86_REG_ESP
    root=Path(__file__).resolve().parents[4]
    target=root/'toolchains/5.10/bin/LINK.EXE'
    proof=json.loads((Path(__file__).resolve().parents[1]/'cases/hello-win32/mapped-open-byte-input.json').read_text())
    assert hashlib.sha256(target.read_bytes()).hexdigest()==proof['target_sha256']=='04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579'
    body=PE(target).read(0x40c880,394)
    assert hashlib.sha256(body).hexdigest()==proof['body_sha256']
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    instructions=list(decoder.disasm(body,0x40c880))
    assert sum(i.size for i in instructions)==394 and instructions[-1].address==0x40ca07
    stack=[(i.address,i.mnemonic,o.size,o.mem.disp)for i in instructions for o in i.operands if o.type==X86_OP_MEM and o.mem.base==X86_REG_ESP]
    expected=[(0x40c880,'mov',1,12),(0x40c890,'mov',4,24),(0x40c8ad,'mov',1,28),(0x40c8cb,'mov',4,20),(0x40c8fe,'mov',4,20),(0x40c920,'mov',4,32),(0x40c977,'mov',1,28),(0x40c9ee,'mov',4,32),(0x40c9f2,'mov',4,20)]
    assert stack==expected
    assert proof['complete_esp_memory_operands']==[list(x)for x in expected]
    assert [(i.address,i.op_str)for i in instructions if i.mnemonic=='ret']==[(0x40c9da,'0x10'),(0x40c9eb,'0x10'),(0x40ca07,'0x10')]
    # False-created continuation forwards record,0,0,size, not the physical third word.
    assert PE(target).read(0x40c9ee,19).hex()=='8b5424208b442414526a006a0050e80f000000'
    assert proof['incoming_slot_escape'] is False and proof['incoming_created_consumption']=='low BYTE only; upper 24 bits unobserved'
    return proof
