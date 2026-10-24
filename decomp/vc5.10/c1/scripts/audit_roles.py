"""Separate the encountered parser's external table from code ownership.

This correction changes graph classification only. The original restored-byte
union and entry plan stay identical; captured receipts/configs remain immutable.
"""
import copy,json
from pathlib import Path
from provenance import PROJECT,sha
BASE=0x10600000
TABLE=[0x12988,0x12998]
NAME='_parse_internal_switch'

def current_config(old):
    corrected=copy.deepcopy(old)
    rows=[r for r in corrected['driver'] if r['name']==NAME]
    if rows:
        assert len(rows)==1
        row=rows[0];absolute=[BASE+x for x in TABLE]
        if absolute in row.get('code_chunks',[]):
            assert row.get('shared',[])==[]
            row['code_chunks'].remove(absolute);row['shared']=[absolute]
        else:
            assert row.get('shared')==[absolute]
        assert row['start']==BASE+0x12900 and row['end']==BASE+0x12984
        assert row['code_chunks']==[[BASE+0x304ad,BASE+0x30501]]
    return corrected

def expected_code(candidate):
    result={}
    for function in candidate['functions']:
        chunks=copy.deepcopy(function['chunks'])
        if function['name']==NAME:
            assert chunks==[[0x12900,0x12984],[0x304ad,0x30501],TABLE]
            chunks.remove(TABLE)
        for table in function.get('text_data_chunks',[]):
            assert table in chunks;chunks.remove(table)
        result[function['name']]=[[BASE+a,BASE+b] for a,b in chunks]
    for item in candidate.get('imports',[]):
        result[item['name']]=[[BASE+item['start_rva'],BASE+item['end_rva']]]
    return result

def text_bytes(candidate):
    return (16 if any(f['name']==NAME for f in candidate['functions']) else 0)+sum(b-a for f in candidate['functions']for a,b in f.get('text_data_chunks',[]))

def validate_config(directory,candidate,execution_inputs):
    key='decomp/vc5.10/c1/'+candidate['audit_config']
    captured=json.loads((directory/'functions.json').read_text())
    assert execution_inputs[key]==sha(directory/'functions.json')
    current=json.loads((PROJECT/candidate['audit_config']).read_text())
    assert current==current_config(captured), 'not the sole accepted parser data-classification correction'
    if current!=captured:
        # Equality above rejects any other changed function, ABI, extent, root,
        # candidate membership or data interval. The old candidate/range files
        # and helper are still checked by the surrounding execution protocol.
        assert current_config(current)==current
        execution_inputs=dict(execution_inputs);execution_inputs[key]=sha(PROJECT/candidate['audit_config'])
    if text_bytes(candidate):
        metadata=json.loads((PROJECT/candidate['audit_metadata']).read_text())
        assert metadata['functions'][NAME]['original']['text_data']==[[BASE+x for x in TABLE]]
        assert metadata['functions'][NAME]['original']['switches']=={'274802963':[274802970,274924743,274803056,274924763]}
    if candidate.get("isolated_locale_observers"):
        from scanner_policy import validate as validate_scanner_policy
        execution_inputs=validate_scanner_policy(directory,candidate,execution_inputs)
    return execution_inputs
