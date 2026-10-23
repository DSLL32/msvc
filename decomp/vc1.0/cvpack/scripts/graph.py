"""Bounded x86 CFG/callee/RET analysis, with declared switch and SEH roots."""
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM

def graph(pe, start, end, data=(), roots=(), switches=None):
    assert start < end
    switches=switches or {}
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    decoder.detail=True
    pending=[start,*roots]
    instructions={}
    owners={}
    edges=set()
    returns=set()
    while pending:
        address=pending.pop()
        if address in instructions: continue
        assert start<=address<end,'CFG target outside extent: %x'%address
        assert not any(a<=address<b for a,b in data),'CFG enters text data'
        assert address not in owners,'CFG enters operand at %x'%address
        available=min(15,end-address)
        decoded=list(decoder.disasm(pe.read(address,available),address,count=1))
        assert decoded,'invalid or truncated instruction at %x'%address
        instruction=decoded[0]
        following=address+instruction.size
        assert following<=end,'truncated instruction'
        for byte in range(address,following):
            assert byte not in owners,'overlapping instructions'
            assert not any(a<=byte<b for a,b in data),'instruction intersects text data'
            owners[byte]=address
        instructions[address]=instruction
        mnemonic=instruction.mnemonic
        operands=instruction.operands
        immediate=operands and operands[0].type==X86_OP_IMM
        target=operands[0].imm&0xffffffff if immediate else None
        if mnemonic.startswith('ret'):
            returns.add(target or 0)
            continue
        if mnemonic in ('int','int3','ud2','hlt'):
            continue
        if mnemonic in ('call','lcall'):
            edges.add(target if immediate else 'indirect')
        if mnemonic=='jmp':
            if immediate:
                if start<=target<end:pending.append(target)
                else:edges.add(target)
            else:
                assert address in switches,'undeclared indirect jump at %x'%address
                pending.extend(switches[address])
            continue
        if mnemonic.startswith('j') or mnemonic.startswith('loop'):
            assert immediate,'unknown conditional transfer'
            assert start<=target<end,'conditional target outside extent'
            pending.append(target)
        assert following<end,'extent ends before a terminal instruction at %x'%address
        pending.append(following)
    for a,b in data:
        assert a<b and not any(byte in owners for byte in range(a,b))
    return dict(edges=edges,returns=returns,instructions=instructions,
                covered=len(owners),end=max(a+i.size for a,i in instructions.items()))

def names(edges, identities):
    result=set()
    for address in edges:
        if address=='indirect':result.add(address)
        else:
            assert address in identities,'unresolved callee %x'%address
            result.add(identities[address])
    return result
