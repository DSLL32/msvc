#!/usr/bin/env python3
"""Narrow static label-refinement positives and forbidden-delta controls."""
import copy,json,tempfile,time
from pathlib import Path
import scanner_policy as policy
from provenance import PROJECT,sha

started=time.perf_counter();packet=json.loads(policy.PACKET.read_text());old=json.loads((PROJECT/packet['old_field_map']['path']).read_text());current=json.loads((PROJECT/packet['new_field_map']['path']).read_text())
policy.derive(old,current,packet);checked=[]
for row in packet['originals']:
 directory=Path(row['path']);assert sha(directory/'receipt.json')==row['receipt_sha256']
 candidate=json.loads((directory/'candidate.json').read_text());inputs=json.loads((directory/'audit-binding.json').read_text())['inputs']
 result=policy.validate(directory,candidate,inputs)
 key='decomp/vc5.10/c1/'+candidate['field_map'];assert result[key]==packet['new_field_map']['sha256']and{key:value for key,value in result.items()if key!='decomp/vc5.10/c1/'+candidate['field_map']}=={key:value for key,value in inputs.items()if key!='decomp/vc5.10/c1/'+candidate['field_map']}
 checked.append(sha(directory/'receipt.json'))
labels=['wrong-old-frontier','wrong-new-frontier','third-allowed-path','other-policy-key','field-default','field-original-alias','field-native-offset','field-extent','extra-known-field','unknown-size','helper-extent','extra-array','changed-candidate','changed-config','changed-mask','changed-helper','changed-rebuilt','changed-source','changed-target','changed-provider','changed-code-byte','changed-memory-scope']
with tempfile.TemporaryDirectory(prefix='scanner-policy-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
 for label in labels:
  a,b,p=copy.deepcopy(old),copy.deepcopy(current),copy.deepcopy(packet)
  if label=='wrong-old-frontier':a['value_copy']['hard_frontier_rva']+=1
  elif label=='wrong-new-frontier':b['command_line']['unknown_semantic_consumer_frontier']+=1
  elif label=='third-allowed-path':p['allowed_paths'].append(['policy'])
  elif label=='other-policy-key':b['policy']='relaxed'
  elif label=='field-default':b['modeled_initialized'][-1]['default'][0]^=1
  elif label=='field-original-alias':b['modeled_initialized'][-1]['original_rva']-=1
  elif label=='field-native-offset':b['modeled_initialized'][-1]['native_offset']+=1
  elif label=='field-extent':b['modeled_initialized'][-1]['bytes']-=1
  elif label=='extra-known-field':b['modeled_initialized'].append(copy.deepcopy(b['modeled_initialized'][-1]))
  elif label=='unknown-size':b['multibyte_scanner']['unknown_initialized_bytes']-=1
  elif label=='helper-extent':b['multibyte_scanner']['initializer_chunks'][-1][-1]+=1
  elif label=='extra-array':b['multibyte_scanner']['initialized_bytes']+=1
  elif label.startswith('changed-'):
   name=label[8:]
   if name=='source':p['source_hashes']['src/scanner.c']='0'*64
   elif name=='code-byte':p['semantic_code_chunks'][0]['bytes']='00'+p['semantic_code_chunks'][0]['bytes'][2:]
   elif name=='memory-scope':p['semantic_memory']['unknown_consumers_reached']=True
   else:p[{'candidate':'candidate_sha256','config':'config_sha256','mask':'ranges_sha256','helper':'helper_sha256','rebuilt':'rebuilt_sha256','target':'target_sha256','provider':'provider_sha256'}[name]]='0'*64
  else:raise AssertionError(label)
  try:
   policy.derive(a,b,p)
   # Transport/source deltas are separately rejected by the concrete binding.
   changed=Path(temporary)/'packet.json';changed.write_text(json.dumps(p));original_packet=policy.PACKET;policy.PACKET=changed
   try:policy.validate(directory,candidate,inputs)
   finally:policy.PACKET=original_packet
  except(AssertionError,ValueError,KeyError):continue
  raise AssertionError('accepted forbidden refinement '+label)
print(json.dumps(dict(status='PASS',kind='validator-only two-label refinement; no process execution',original_receipts=checked,fresh_negative_controls=len(labels),rejected=labels,seconds=time.perf_counter()-started,packet_sha256=sha(policy.PACKET),old_field_map_sha256=packet['old_field_map']['sha256'],new_field_map_sha256=packet['new_field_map']['sha256']),indent=2))
