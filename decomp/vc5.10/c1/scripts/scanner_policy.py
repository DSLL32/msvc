"""Exactly two validator-policy labels advance; execution evidence stays frozen."""
import copy,json
from pathlib import Path
from provenance import PROJECT,ROOT,sha
from allocation_imports import pe_read

PATHS=(('value_copy','hard_frontier_rva'),('command_line','unknown_semantic_consumer_frontier'))
PACKET=PROJECT/'cases/minimal/scanner-policy-refinement.json'

def derive(old,current,packet):
 assert packet['kind']=='validator-only scanner frontier refinement; no execution-input changes'
 assert packet['allowed_paths']==[list(path)for path in PATHS]
 assert packet['old_rva']==0x12f08 and packet['new_rva']==0x12f3e
 expected=copy.deepcopy(old)
 for parent,key in PATHS:
  assert expected[parent][key]==0x12f08;expected[parent][key]=0x12f3e
 assert current==expected,'any field/layout/extent/default/alias or extra policy delta is forbidden'
 assert current['multibyte_scanner']['known_initialized_bytes']==2348 and current['multibyte_scanner']['unknown_initialized_bytes']==4200
 assert current['multibyte_scanner']['initializer_chunks']==[[0x12f08,0x12f3e],[0x307c0,0x307d2]]
 return current

def validate(directory,candidate,inputs):
 key='decomp/vc5.10/c1/'+candidate['field_map'];current=PROJECT/candidate['field_map']
 if inputs[key]==sha(current):return inputs
 packet=json.loads(PACKET.read_text());old=PROJECT/packet['old_field_map']['path']
 assert inputs[key]==sha(old)==packet['old_field_map']['sha256']and sha(current)==packet['new_field_map']['sha256']
 derive(json.loads(old.read_text()),json.loads(current.read_text()),packet)
 receipt=json.loads((directory/'receipt.json').read_text());assert receipt['kind']=='explicit-original-capture'and receipt['mode']=='masked'and receipt['exit']==125
 assert sha(directory/'candidate.json')==packet['candidate_sha256']==sha(PROJECT/'cases/minimal/candidate.json')
 assert sha(directory/'functions.json')==packet['config_sha256']==sha(PROJECT/candidate['audit_config'])
 assert sha(directory/'ranges.txt')==packet['ranges_sha256']
 assert receipt['helper_sha256']==packet['helper_sha256']==sha(PROJECT/'build/dll-capture.exe')
 assert receipt['rebuilt_sha256']==packet['rebuilt_sha256']==sha(PROJECT/'build/c1.dll')
 assert receipt['source_hashes']==packet['source_hashes']
 assert all(sha(PROJECT/path)==digest for path,digest in packet['source_hashes'].items())
 assert receipt['target_sha256']==packet['target_sha256']==sha(directory/'loaded-target.bin')
 assert receipt['canonical_crt_sha256']==packet['provider_sha256']==sha(directory/'loaded-msvcrt.bin')
 for row in packet['semantic_code_chunks']:
  assert pe_read(directory/'loaded-target.bin',row['start'],row['end']-row['start'])==bytes.fromhex(row['bytes'])
 assert packet['semantic_memory']==dict(target_read_write=[[0x5e008,0x5e208]],other_target_reads='only canonical function/data IAT slots5d148/5d094',external='qualified MSVCRT._setmbcp and full witnessed257-byte _mbctype; no original literal tail or unmodeled initialized state',unknown_consumers_reached=False)
 assert candidate['expected_frontier']['rva']==0x12f3e and candidate['functions'][-1]['chunks']==[[0x12f08,0x12f3e],[0x307c0,0x307d2]]
 result=dict(inputs);result[key]=sha(current);return result
