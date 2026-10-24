"""Bound the whole original timer bodies and their consumed-state scope."""
import hashlib
import json
from pathlib import Path
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_OP_IMM
from binary import PE
from import_residuals import qualified_slots

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
sha=lambda data:hashlib.sha256(data).hexdigest()

def proof():
    path=ROOT/'toolchains/5.10/bin/CL.EXE';pe=PE(path)
    assert sha(pe.data)=='aa15ed605515693292c539827ab1d8e08ce2ed5b4e1d9ac2c7f033903e6844dd'
    machine=Cs(CS_ARCH_X86,CS_MODE_32);machine.detail=True
    specifications=[('start_timer',0x4053fd,13),('stop_timer',0x40540a,139),('link_pass',0x40426b,342)]
    bodies={};decoded={}
    for name,address,length in specifications:
        raw=pe.read(address,length);instructions=list(machine.disasm(raw,address))
        assert sum(i.size for i in instructions)==length and instructions[-1].mnemonic=='ret'
        decoded[name]=instructions
        bodies[name]={'address':address,'bytes':length,'sha256':sha(raw),
            'instructions':[{'address':i.address,'bytes':i.bytes.hex(),'mnemonic':i.mnemonic,'operands':i.op_str}for i in instructions]}
    imports=qualified_slots(pe)
    assert imports[0x40a2a4]=={'dll':'msvcrt.dll','name':'_ftime'}
    assert not any(v['name']in('_tzname','_timezone','_daylight')for v in imports.values())
    direct=[];indirect=[];reads=[]
    for instruction in decoded['stop_timer']:
        if instruction.mnemonic=='call':
            operand=instruction.operands[0]
            (direct if operand.type==X86_OP_IMM else indirect).append(operand.imm if operand.type==X86_OP_IMM else operand.mem.disp)
        for operand in instruction.operands:
            if operand.type==X86_OP_MEM and operand.mem.base==0 and operand.mem.index==0:
                reads.append((instruction.address,operand.mem.disp,operand.size))
    assert direct==[0x405069,0x405890,0x404fee,0x405069]
    assert indirect==[0x40a2a4]
    assert reads==[(0x40540f,0x4095d0,4),(0x405431,0x40a2a4,4),
        (0x405437,0x40979c,2),(0x40543e,0x40968c,2),(0x405445,0x409688,4),(0x40544c,0x409798,4)]
    strings={str(address):pe.string(address)for address in(0x4084f4,0x4084fc,0x408500)}
    assert strings=={str(0x4084f4):'%d.%ss\n',str(0x4084fc):'00',str(0x408500):'time(%s)='}
    section=next(s for s in pe.sections if s['name']=='.text');text=pe.read(pe.base+section['rva'],section['virtual_size'])
    references={}
    for address,expected in [(0x409798,[0x4053fe,0x40544e]),(0x40979c,[0x40543a]),
            (0x409688,[0x40542d,0x405447]),(0x40968c,[0x405441]),(0x40a2a4,[0x405404,0x405433])]:
        needle=struct.pack('<I',address);offset=0;found=[]
        while True:
            offset=text.find(needle,offset)
            if offset<0:break
            found.append(pe.base+section['rva']+offset);offset+=1
        assert found==expected;references[str(address)]=found
    return {'kind':'Whole original timer/caller consumed-state proof; analysis only, zero function credit',
        'target':{'path':str(path),'sha256':sha(pe.data)},'bodies':bodies,'literal_operands':strings,
        'qualified_import':{'slot':0x40a2a4,**imports[0x40a2a4]},'absolute_text_reference_inventory':references,
        'defined_output':'Full _timeb time, millitm, timezone and dstflag remain strict; observed padding preserved',
        'consumed_timer_fields':[{'offset':0,'bytes':4,'type':'signed time_t'},{'offset':4,'bytes':2,'type':'unsigned short millitm'}],
        'pointer_escape':'Static timeb addresses are passed only to _ftime; start_timer and stop_timer return void. stop_timer passes only calculated seconds and its local decimal buffer to write_message. No timezone pointer enters CL.',
        'case_scope':'Existing object-only LINK path, link_pass calls start_timer/execute_pass/stop_timer; no frontend/final_pass. Timer buffers have only the exact absolute references above; neither target imports nor current reached code access _tzname.',
        'unmatched_provider_state':'Original static Greenwich labels are retained with their ownership/raw bytes. Native PST/PDT labels are NOT matched or reseeded. They are unconsumed by the whole timer/caller/reached path; a later actual reader reopens this boundary.',
        'FP_and_errors':'Actual fresh _ftime pre/post error and FP state must still match; this analysis does not authorize arbitrary clock/provider state normalization.'}

if __name__=='__main__':print(json.dumps(proof(),indent=2))
