"""Finite reviewed partial register-table ownership and state proof; no CFG discovery."""
import json
import re
from state import BINARY,TARGET,sha


def model(config):
    if not config.get('register_model'):return None
    raw=(BINARY/config['register_model']).read_bytes()
    assert sha(raw)==config['register_model_sha256'],'register model identity changed'
    value=json.loads(raw)
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert (value['actual_row_base'],value['array_end'],value['count'],value['stride'],value['flags_offset'])==(0x494758,0x495ec8,75,80,5)
    return value


def source_guard(root,value):
    for name,digest in value['source_files'].items():
        assert sha((root/name).read_bytes())==digest,'partial register source changed without contextual review'
    for path in (root/'src').rglob('*'):
        if path.suffix not in ('.c','.h')or str(path.relative_to(root))in value['source_files']:continue
        assert not re.search(r'\b(machine_registers|C2MachineRegister)\b',path.read_text()),'unmodeled register consumer or pointer escape'

def owner_guard(config,value):
    from data_evidence import reader
    read=reader(TARGET)
    for witness in value['actual_row_base_witnesses']:
        fields=witness['line'].split('\t');raw=bytes.fromhex(fields[1])
        assert read(witness['instruction'],len(raw))==raw,'register use witness no longer matches actual instructions'
        for record in config['driver']+config['support']:
            if record['name']not in config['accepted']+config.get('trial',[]):continue
            chunks=[(record['start'],record['end'])]+[(row['start'],row['end'])if isinstance(row,dict)else row for row in record.get('code_chunks',[])]
            if any(row[0]<=witness['instruction']<row[1]for row in chunks):
                assert record['name']in value['allowed_owners'],'unknown register field/escape consumer requires reconstruction first'

def verify(config):
    value=model(config)
    if value is None:return None
    from data_evidence import reader
    read=reader(TARGET)
    assert sha(read(0x428642,24))==value['helper_sha256'],'register reset body changed'
    original=read(0x494758,6000)
    assert sha(original)==value['original_table_sha256']
    assert list(original[5::80])==value['proven_initialized_flags']==[0]*75
    source_guard(BINARY,value)
    owner_guard(config,value)
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    rebuilt=reader(BINARY/'build/c2.exe')(maps['_machine_registers'],6000)
    assert rebuilt[5::80]==bytes(75),'rebuilt initialized register flags differ'
    return {'model_sha256':config['register_model_sha256'],'original_base':0x494758,'rebuilt_base':maps['_machine_registers'],'count':75,'stride':80,'flags_offset':5,'initialized_data_bytes':75,'unmodeled_bytes':5925,'helper_bytes':24,'known_instruction_sites':len(value['actual_row_base_witnesses']),'scope':'Only flag bytes; pending backing has no default/layout/content credit; unknown consumers held.'}


def compare_states(original,native,config,state,native_proof,proof):
    value=model(config)
    for directory in (original,native):
        assert (directory/'register-model.json').read_bytes()==(BINARY/config['register_model']).read_bytes()
        receipt=json.loads((directory/'receipt.json').read_text())
        before=(directory/'registers-before.bin').read_bytes();after=(directory/'registers-after.bin').read_bytes()
        assert len(before)==len(after)==6000,'register state extent differs'
        assert receipt['register_state_hashes']=={'registers-before.bin':sha(before),'registers-after.bin':sha(after)},'register raw provenance changed'
        flags=set(range(5,6000,80))
        assert all(before[i]==after[i]for i in range(6000)if i not in flags),'own unmodeled register bytes were modified'
        assert after[5::80]==bytes(v&~16 for v in before[5::80]),'register flag bit-clear behavior differs'
        assert before[5::80]==bytes(value['proven_initialized_flags']),'actual initialized register flags differ'
    assert (original/'registers-after.bin').read_bytes()[5::80]==(native/'registers-after.bin').read_bytes()[5::80]
    events=(original/'events.txt').read_text()
    assert re.findall(r'^register-state (before|after) base=00494758 bytes=6000 count=75 stride=80 flags=5$',events,re.M)==['before','after'],'missing actual original before/after observation'
    symbol,size=proof.symbols(BINARY/'build/native/c2',data=True)['machine_registers']
    assert size==6000
    expected={'base':symbol+int(native_proof['load_bias'],16),'bytes':6000,'count':75,'stride':80,'flags_offset':5,'source_symbol':'machine_registers','entry_before':True}
    assert state['register_state']==expected,'register live storage alias/extent differs'
    observed=re.findall(r'^C2-REGISTER-STATE (.*)$',(native/'gdb.stdout').read_text(),re.M)
    assert len(observed)==1 and json.loads(observed[0])==expected,'missing raw GDB register observation'
