"""Temporary narrow IAT identity residual; function graph discovery stays in Rust.

Linear operand inspection is valid here only because these two small RC functions
and embedded CRT helpers have no in-function jump tables or other text data.
The slot-set check also covers import data, which --check-imports does not.
The separate cached-register residual below covers only the two observed fixed
callee-saved-register idioms; it is not general dataflow or CFG discovery.
"""
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_OP_MEM,CS_OP_IMM,CS_OP_REG,CS_GRP_JUMP
from binary import u32


def slots(pe):
    result={};rva=u32(pe.data,pe.optional+104)
    if not rva:return result
    p=pe.base+rva
    for _ in range(1024):
        names,_,_,dll,iat=struct.unpack('<IIIII',pe.read(p,20))
        if not dll:break
        library=pe.string(pe.base+dll).lower()
        for n in range(10000):
            item=struct.unpack('<I',pe.read(pe.base+names+4*n,4))[0]
            if not item:break
            name='ordinal_'+str(item&0xffff) if item&0x80000000 else pe.string(pe.base+item+2)
            result[pe.base+iat+4*n]=(library,name)
        p+=20
    return result


def identities(pe,start,end):
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    imports=slots(pe);found=set()
    for ins in decoder.disasm(pe.read(start,end-start),start):
        for operand in ins.operands:
            if operand.type==CS_OP_MEM and not operand.mem.base and not operand.mem.index:
                address=operand.mem.disp&0xffffffff
            elif operand.type==CS_OP_IMM:address=operand.imm&0xffffffff
            else:continue
            if address in imports:found.add(imports[address])
    return sorted(found)


def cached_import(pe,start,end,logical):
    """Check only RC's observed callee-saved IAT caches, with raw instruction proof.

    A single absolute-IAT MOV precedes every register call. Linear inspection of
    this complete text-data-free function proves no intervening full/partial-register write through its restore;
    called Win32/CRT/RC functions preserve EBX/ESI under their recovered ABIs.
    This does not resolve arbitrary cached pointers, callback targets or aliases.
    Static call multiplicity can differ because the compiler rotates a loop.
    """
    register,identity = {
        '_rc_main':('esi',('kernel32.dll','SetConsoleCtrlHandler')),
        '_rc_diagnostic@12':('ebx',('msvcrt.dll','strchr')),
    }[logical]
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    instructions=list(decoder.disasm(pe.read(start,end-start),start))
    imports=slots(pe);loads=[];calls=[]
    for ins in instructions:
        operands=ins.operands
        if ins.mnemonic=='mov' and len(operands)==2 and operands[0].type==CS_OP_REG and ins.reg_name(operands[0].reg)==register:
            mem=operands[1]
            if mem.type==CS_OP_MEM and not mem.mem.base and not mem.mem.index:
                loads.append((ins,mem.mem.disp&0xffffffff))
        if ins.mnemonic=='call' and len(operands)==1 and operands[0].type==CS_OP_REG:
            assert ins.reg_name(operands[0].reg)==register,('Unexpected register call',logical,ins.address,ins.op_str)
            calls.append(ins)
    assert len(loads)==1 and len(calls)>=2,('Missing/ambiguous cached import',logical)
    load,slot=loads[0]
    assert imports.get(slot)==identity,('Cached import load identity',logical,slot,imports.get(slot))
    assert load.address<calls[0].address,('Call precedes cache load',logical)
    aliases={'esi':{'esi','si'},'edi':{'edi','di'},'ebx':{'ebx','bx','bl','bh'}}[register]
    restores=[ins for ins in instructions if ins.mnemonic=='pop' and len(ins.operands)==1 and ins.operands[0].type==CS_OP_REG and ins.reg_name(ins.operands[0].reg)==register]
    assert len(restores)==1 and restores[0].address>calls[-1].address,('Missing/early cache register restore',logical)
    live_begin,live_end=load.address+load.size,restores[0].address
    for ins in instructions:
        if live_begin<=ins.address<live_end:
            _,writes=ins.regs_access()
            assert not aliases.intersection(ins.reg_name(r) for r in writes),('Cached register/alias overwritten',logical,ins.address,ins.mnemonic,ins.op_str)
        if ins.group(CS_GRP_JUMP):
            assert len(ins.operands)==1 and ins.operands[0].type==CS_OP_IMM,('Unsupported indirect branch in cache residual',logical,ins.address)
            target=ins.operands[0].imm&0xffffffff
            assert not (live_begin<=target<live_end and not live_begin<=ins.address<live_end),('Branch bypasses cache load/restore',logical,ins.address,target)
    def record(ins):return {'va':ins.address,'bytes':ins.bytes.hex(),'instruction':ins.mnemonic+' '+ins.op_str}
    return {'identity':list(identity),'register':register,'slot':slot,'load':record(load),'calls':[record(ins) for ins in calls],
            'preserved_interval':[live_begin,live_end], 'restore':record(restores[0]), 'aliases':sorted(aliases),
            'instruction_evidence':[record(ins) for ins in instructions if load.address<=ins.address<=live_end],
            'qualification':'narrow linear callee-saved-register residual, not Rust import qualification or general dataflow'}
