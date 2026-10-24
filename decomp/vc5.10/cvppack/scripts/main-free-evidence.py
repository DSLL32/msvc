"""Static no-consumer interval and public CRT free identity, outside fixtures."""
import hashlib
import json
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM
from binary import PE
from state import BINARY,ROOT,sha


def prove(config):
    binding=config['main_free_policy'];path=BINARY/binding['path']
    assert sha(path)==binding['sha256'],'changed main free policy'
    policy=json.loads(path.read_text());target=ROOT/config['target']['path'];pe=PE(target)
    assert sha(target)==policy['target_sha256']
    scratch=bool(config.get('observe_widen_frees'))
    assert scratch==bool(policy.get('widen_scratch_cleanup'))
    end=0x404763 if scratch else 0x40475e
    raw=pe.read(0x404719,end-0x404719)
    assert hashlib.sha256(raw).hexdigest()==policy['main_interval_sha256']
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;instructions=list(decoder.disasm(raw,0x404719))
    assert sum(i.size for i in instructions)==len(raw) and instructions[-1].address==(0x40475e if scratch else 0x404759) and instructions[-1].op_str==('0x40b140' if scratch else '0x405230')
    calls=[i for i in instructions if i.mnemonic=='call'];assert [i.address for i in calls]==[0x404726,0x404731,0x40473d,0x404749,0x404754,0x404759]+([0x40475e] if scratch else [])
    allowed={0x415658,0x4161ac,0x41567c,0x415aa4,0x415674,0x415678}
    assert all(i.mnemonic in ('mov','push','call','add') for i in instructions),'unproved cleanup control flow'
    for i in instructions:
        for operand in i.operands:
            if operand.type==X86_OP_MEM:
                memory=operand.mem
                assert not memory.base and not memory.index and memory.disp in allowed,'cleanup dereferences/escapes released payload'
    scratch_proof=None
    if scratch:
        body=pe.read(0x405230,32)
        assert hashlib.sha256(body).hexdigest()==policy['scratch_interval_sha256']
        scratch_instructions=list(decoder.disasm(body,0x405230))
        assert sum(i.size for i in scratch_instructions)==32 and scratch_instructions[-1].mnemonic=='ret'
        assert [i.address for i in scratch_instructions if i.mnemonic=='call']==[0x40523d,0x405249]
        for instruction in scratch_instructions:
            assert instruction.mnemonic in ('mov','push','pop','call','add','ret')
            for operand in instruction.operands:
                if operand.type==X86_OP_MEM:
                    memory=operand.mem
                    assert not memory.base and not memory.index and memory.disp in {0x412578,0x41257c,0x4161ac},'scratch cleanup consumes released payload'
        scratch_proof=dict(extent=[0x405230,0x405250],sha256=policy['scratch_interval_sha256'],free_sites=[0x40523d,0x405249],no_released_payload_consumer=True)
    provider=ROOT/'toolchains/5.10/bin/MSVCRT.DLL';crt=PE(provider)
    assert sha(provider)==policy['crt_sha256']
    evidence=BINARY/'cases/hello-debug-dll/import-preservation.json'
    witness=json.loads(evidence.read_text());assert witness['qualified_import']==dict(dll='msvcrt.dll',name='free')
    assert witness['provider']['sha256']==sha(provider) and witness['provider']['function'][0]==crt.base+policy['free_rva']
    provider_start,provider_end=witness['provider']['function'];assert hashlib.sha256(crt.read(provider_start,provider_end-provider_start)).hexdigest()==witness['provider']['function_sha256']
    return dict(policy_sha256=sha(path),main_interval=[0x404719,end],main_interval_sha256=policy['main_interval_sha256'],no_released_payload_consumer=True,scratch_cleanup=scratch_proof,
        crt_sha256=sha(provider),free_rva=policy['free_rva'],ordinary_crt_witness_sha256=sha(evidence),validator_sha256=sha(BINARY/'scripts/main-free-evidence.py'),state_validator_sha256=sha(BINARY/'scripts/main-free-state.py'))
