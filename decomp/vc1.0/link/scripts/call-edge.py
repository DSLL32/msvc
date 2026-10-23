"""Caller-qualified direct-import observation guard; reads original bytes only."""
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM

def validate(context,callee,caller,original,native_caller):
    assert context and caller and native_caller, 'Missing caller/callee observation'
    assert context['eip']-1==callee, 'Unexpected original callee'
    assert context.get('stack'), 'Missing original return address'
    address=context['stack'][0]
    assert caller['implementation']=='C' and caller['start']<address<=caller['end'], 'Return address outside accepted caller extent'
    logical=caller['name'][1:].split('@')[0]
    assert native_caller==logical, 'Native caller identity differs'
    instruction=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(original.read(address-5,5),address-5))
    assert len(instruction)==1 and instruction[0].mnemonic=='call' and instruction[0].size==5, 'Expected direct original call'
    assert int(instruction[0].op_str,16)==callee, 'Original direct call targets another thunk'
    return dict(caller=logical,original_caller_extent=[caller['start'],caller['end']],
                original_return=address,original_call=address-5,callee=callee)
