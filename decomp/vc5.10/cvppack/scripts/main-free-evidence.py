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
    tables=bool(config.get('observe_type_table_frees'));assert bool(policy.get('type_table_cleanup'))==tables
    complete=bool(config.get('observe_source_index_complete'));assert bool(policy.get('source_index_complete'))==complete
    end=0x404776 if complete else 0x404771 if tables else 0x404763 if scratch else 0x40475e
    raw=pe.read(0x404719,end-0x404719)
    assert hashlib.sha256(raw).hexdigest()==policy['main_interval_sha256']
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True;instructions=list(decoder.disasm(raw,0x404719))
    assert sum(i.size for i in instructions)==len(raw) and instructions[-1].address==(0x404771 if complete else 0x40476c if tables else 0x40475e if scratch else 0x404759) and instructions[-1].op_str==('0x4069e0' if complete else '0x40f100' if tables else '0x40b140' if scratch else '0x405230')
    calls=[i for i in instructions if i.mnemonic=='call'];assert [i.address for i in calls]==[0x404726,0x404731,0x40473d,0x404749,0x404754,0x404759]+([0x40475e] if scratch else [])+([0x40476c] if tables else [])+([0x404771] if complete else [])
    allowed={0x415658,0x4161ac,0x41567c,0x415aa4,0x415674,0x415678}|({0x4154c2} if tables else set())
    assert all(i.mnemonic in (('mov','push','call','add','cmp','je') if tables else ('mov','push','call','add')) for i in instructions),'unproved cleanup control flow'
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
    type_prefix=None
    derived=bool(config.get('observe_derived_finalize_entry'))
    assert bool(policy.get('type_finalize_entry'))==derived
    if derived:
        prefix=pe.read(0x40b140,6)
        assert prefix==bytes.fromhex('56e81a2e0000') and hashlib.sha256(prefix).hexdigest()==policy['type_cleanup_prefix_sha256'],'unproved type cleanup continuation'
        type_prefix=dict(extent=[0x40b140,0x40b146],sha256=policy['type_cleanup_prefix_sha256'],no_released_payload_consumer=True,callee=0x40df60)
    type_cleanup=None
    if tables:
        body=pe.read(0x40b140,135)
        assert hashlib.sha256(body).hexdigest()==policy['type_cleanup_sha256']
        decoded=list(decoder.disasm(body,0x40b140))
        assert sum(i.size for i in decoded)==135 and decoded[-1].mnemonic=='ret'
        assert [i.address for i in decoded if i.mnemonic=='call']==[0x40b141,0x40b152,0x40b15e,0x40b16a,0x40b175,0x40b181,0x40b199,0x40b1ae]
        allowed={0x41530c,0x4161ac,0x412ce0,0x412ce8,0x412cfc,0x4132c4,0x4132bc,0x4132d0,0x4132b8,0x416270,0x41625c}
        for instruction in decoded:
            for operand in instruction.operands:
                if operand.type==X86_OP_MEM:
                    memory=operand.mem
                    assert not memory.base and not memory.index and memory.disp in allowed,'type cleanup dereferences released payload'
        paths=[]
        for row in policy['derived_zero_path']:
            raw=pe.read(row['start'],row['end']-row['start']);assert hashlib.sha256(raw).hexdigest()==row['sha256']
            paths.append(row)
        assert [(r['start'],r['end']) for r in paths]==[(0x40df60,0x40df8a),(0x40e198,0x40e1a0)]
        assert pe.read(0x40df74,8)==bytes.fromhex('66833d8032410000') and pe.read(0x40df84,6)==bytes.fromhex('0f860e020000'),'derived zero-count branch differs'
        type_cleanup=dict(extent=[0x40b140,0x40b1c7],sha256=policy['type_cleanup_sha256'],free_sites=[0x40b152,0x40b15e,0x40b16a,0x40b175,0x40b181],derived_zero_path=paths,requires_zero=['derived_type_count','type_pdb','type_pdb_types'],no_released_payload_consumer=True)
    source_prefix=None
    source_index=bool(config.get('observe_source_index_entry'))
    assert bool(policy.get('source_index_entry'))==source_index
    if source_index:
        prefix=policy['source_index_prefix'];assert (prefix['start'],prefix['end'])==(0x40f100,0x40f1d7)
        raw=pe.read(prefix['start'],prefix['end']-prefix['start']);assert hashlib.sha256(raw).hexdigest()==prefix['sha256']
        decoded=list(decoder.disasm(raw,prefix['start']))
        assert sum(i.size for i in decoded)==len(raw) and [i.address for i in decoded if i.mnemonic=='call']==[0x40f183,0x40f1d2]
        assert decoded[-1].mnemonic=='call' and decoded[-1].op_str=='0x40f0d0'
        source_prefix=dict(**prefix,source_root='ModuleInfo.source_lines/size',new_generation_owner='build_source_file_index',dynamic_operands_validator='scripts/source-index-state.py',dynamic_operands_validator_sha256=sha(BINARY/'scripts/source-index-state.py'))
    if complete:
        intervals=[];absolute=[]
        for key,extent in [('source_index_body',(0x40f100,0x40f3fa)),('append_source_name',(0x40f0d0,0x40f0f6))]:
            row=policy[key];assert (row['start'],row['end'])==extent
            body=pe.read(*[extent[0],extent[1]-extent[0]]);assert hashlib.sha256(body).hexdigest()==row['sha256']
            decoded=list(decoder.disasm(body,extent[0]));assert sum(i.size for i in decoded)==len(body)
            for instruction in decoded:
                for operand in instruction.operands:
                    if operand.type==X86_OP_MEM and not operand.mem.base and not operand.mem.index:
                        assert operand.mem.disp in {0x415368,0x415338,0x415634,0x415648,0x4161ac},'source index consumes/compares/escapes retired source root'
                        absolute.append(dict(site=instruction.address,address=operand.mem.disp,width=operand.size))
            intervals.append(row)
        source_prefix=dict(intervals=intervals,absolute_operands=absolute,new_family_validator_sha256=sha(BINARY/'scripts/source-index-complete.py'),no_released_root_consumer=True)
    provider=ROOT/'toolchains/5.10/bin/MSVCRT.DLL';crt=PE(provider)
    assert sha(provider)==policy['crt_sha256']
    evidence=BINARY/'cases/hello-debug-dll/import-preservation.json'
    witness=json.loads(evidence.read_text());assert witness['qualified_import']==dict(dll='msvcrt.dll',name='free')
    assert witness['provider']['sha256']==sha(provider) and witness['provider']['function'][0]==crt.base+policy['free_rva']
    provider_start,provider_end=witness['provider']['function'];assert hashlib.sha256(crt.read(provider_start,provider_end-provider_start)).hexdigest()==witness['provider']['function_sha256']
    return dict(policy_sha256=sha(path),main_interval=[0x404719,end],main_interval_sha256=policy['main_interval_sha256'],no_released_payload_consumer=True,scratch_cleanup=scratch_proof,type_cleanup_prefix=type_prefix,type_table_cleanup=type_cleanup,source_index_prefix=source_prefix,
        crt_sha256=sha(provider),free_rva=policy['free_rva'],ordinary_crt_witness_sha256=sha(evidence),validator_sha256=sha(BINARY/'scripts/main-free-evidence.py'),state_validator_sha256=sha(BINARY/'scripts/main-free-state.py'))
