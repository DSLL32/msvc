"""Explicit one-label validation derivation; execution metadata stays immutable."""
import copy,hashlib,json
from provenance import PROJECT,sha

PACKET=PROJECT/'cases/minimal/path-fold-layout-refinement.json'

def digest(value):return hashlib.sha256((json.dumps(value,sort_keys=True,separators=(',',':'))+'\n').encode()).hexdigest()

def derive(old,current,packet):
 assert packet['kind']=='validator-only duplicated BSS-size label correction; execution inputs unchanged'
 assert packet['allowed_path']==['command_line','native_bss_size']and packet['old_value']==1860 and packet['new_value']==2372
 expected=copy.deepcopy(old);assert expected['command_line']['native_bss_size']==1860;expected['command_line']['native_bss_size']=2372
 assert expected==current,'any other field/default/layout/ABI/policy delta is forbidden'
 assert old['native_bss_size']==2372 and old['path_fold']['bytes']==512 and old['path_fold']['native_offset']==1860
 row=[row for row in old['modeled_bss']if row['name']=='path_fold'];assert len(row)==1 and row[0]['native_offset']==1860 and row[0]['bytes']==512 and row[0]['c_type']=='WORD[256]'and row[0]['native_field']=='c1_bss.path_fold'
 assert packet['derived_field_map_semantic_sha256']==digest(expected)
 return current

def command_line_model(fields):
 if fields['command_line']['native_bss_size']==fields['native_bss_size']:return fields['command_line']
 packet=json.loads(PACKET.read_text());path=PROJECT/packet['original_field_map']['path']
 assert sha(path)==packet['original_field_map']['sha256'];old=json.loads(path.read_text());assert old==fields
 current=copy.deepcopy(fields);current['command_line']['native_bss_size']=2372
 derive(old,current,packet)
 return current['command_line']

def validate_native_schema(native,binary_size):
 schema=json.loads((native/'grouped-layout.json').read_text());bss=schema['c1_bss'];row=bss['fields']['path_fold'];node=row['schema']
 assert binary_size==bss['size']==2372 and row['offset']==1860 and node['kind']=='array'and node['size']==512 and node['count']==256
 assert node['element']['size']==2 and node['element']['signed']is False

def validate_bound_inputs(original,native):
 packet=json.loads(PACKET.read_text());old=packet['original_field_map']['sha256']
 for directory in (original,native):
  receipt=json.loads((directory/'receipt.json').read_text());row=packet['captures'][directory.name]
  assert row['path']==str(directory.resolve())and row['receipt_sha256']==sha(directory/'receipt.json')
  for key in ('candidate_sha256','functions_sha256','rebuilt_sha256'):assert row[key]==receipt[key]
  assert sha(directory/'candidate.json')==row['candidate_sha256']and sha(directory/'functions.json')==row['functions_sha256']
  binding=json.loads((directory/'audit-binding.json').read_text())
  assert binding['inputs']['decomp/vc5.10/c1/cases/minimal/field-map.json']==old
  if receipt['kind']=='explicit-original-capture':
   assert row['ranges_sha256']==sha(directory/'ranges.txt')and row['helper_sha256']==receipt['helper_sha256']
  else:assert receipt['native_sha256']==packet['physical_proof']['native_elf_sha256']
 return dict(packet_sha256=sha(PACKET),original_field_map_sha256=old,derived_field_map_semantic_sha256=packet['derived_field_map_semantic_sha256'],allowed_path=packet['allowed_path'])
