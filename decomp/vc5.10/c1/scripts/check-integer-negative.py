#!/usr/bin/env python3
"""Affected numeric/public-ABI/whole-loop controls against immutable captures."""
import argparse,copy,json,re,tempfile,time
from pathlib import Path
from integer_state import validate_loop
from integer_policy import derive,validate as validate_policy,PACKET
from provenance import PROJECT,sha

class Overlay:
 def __init__(self,base,files):self.base,self.files=base,files
 def __truediv__(self,name):return self.files.get(name,self.base/name)
 def resolve(self):return self.base.resolve()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 c=json.loads((a.original/'candidate.json').read_text());started=time.perf_counter();validate_loop(a.original,a.native,c);controls=[]
 def reject(label,scope,operation):
  try:operation()
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(dict(scope=scope,control=label))
 def native_json(label,name,change):
  value=json.loads((a.native/name).read_text());change(value)
  with tempfile.TemporaryDirectory(prefix='integer-control-',dir=a.native.parent)as tmp:
   path=Path(tmp)/name;path.write_text(json.dumps(value));n=Overlay(a.native,{name:path})
   reject(label,'native',lambda:validate_loop(a.original,n,c))
 def raw(label,name,offset):
  value=bytearray((a.original/name).read_bytes());value[offset]^=1
  with tempfile.TemporaryDirectory(prefix='integer-control-',dir=a.original.parent)as tmp:
   path=Path(tmp)/name;path.write_bytes(value);o=Overlay(a.original,{name:path})
   reject(label,'original',lambda:validate_loop(o,a.native,c))
 for label,name,change in [
  ('missing-third-integer','integer-entries.json',lambda x:x.pop()),
  ('integer-formal-omission','integer-entries.json',lambda x:x[1]['formal_inventory'].clear()),
  ('integer-input-identity','integer-entries.json',lambda x:x[1]['args'].__setitem__('value',0)),
  ('integer-input-NUL','integer-entries.json',lambda x:x[0]['input'].__setitem__(-1,1)),
  ('integer-result','integer-returns.json',lambda x:x[2].__setitem__('eax',1)),
  ('integer-return-cleanup','integer-returns.json',lambda x:x[0].__setitem__('esp',0)),
  ('integer-callee-save','integer-returns.json',lambda x:x[1]['preserved'].__setitem__('edi',0)),
  ('public-call-omission','integer-api-entries.json',lambda x:x.pop()),
  ('public-input-slot','integer-api-entries.json',lambda x:x[0]['words'].__setitem__(1,0)),
  ('public-provider-PC','integer-native-providers.json',lambda x:x['isspace'].__setitem__('address',x['isspace']['address']+1)),
  ('public-provider-hash','integer-native-providers.json',lambda x:x['isspace'].__setitem__('provider_sha256','0'*64)),
  ('public-PLT-binding','integer-api-entries.json',lambda x:x[1]['physical_import'].__setitem__('got',0)),
  ('public-fake-target-formals','integer-api-entries.json',lambda x:x[0].__setitem__('formal_inventory',[])),
  ('public-isspace-predicate','integer-api-returns.json',lambda x:x[0].__setitem__('eax',1)),
  ('public-isdigit-predicate','integer-api-returns.json',lambda x:x[1].__setitem__('eax',0)),
  ('public-atol-exact','integer-api-returns.json',lambda x:x[2].__setitem__('eax',2)),
  ('public-argument-slot-after','integer-api-returns.json',lambda x:x[2].__setitem__('argument_slot_after',0)),
  ('public-error-preservation','integer-api-returns.json',lambda x:x[0]['errors'].__setitem__(0,1)),
  ('public-FP-preservation','integer-api-returns.json',lambda x:x[0]['fp'].__setitem__('sw',0)),
  ('public-MB-domain','integer-api-entries.json',lambda x:x[0]['locale'].__setitem__('mb_cur_max',2)),
  ('seventh-action-omission','option-action-entries.json',lambda x:x.pop()),
  ('later-processor-write','option-action-returns.json',lambda x:x[3]['compiler_state']['option_state']['initialized'].__setitem__('processor',0)),
  ('later-inline-write','option-action-returns.json',lambda x:x[5]['compiler_state']['option_state']['initialized'].__setitem__('inline_level',1)),
  ('final-nologo-write','option-action-returns.json',lambda x:x[6]['compiler_state']['option_state']['bss'].__setitem__('suppress_banner',0)),
  ('final-NULL-result','argument-cursor-returns.json',lambda x:x[-1].__setitem__('eax',1)),
  ('final-NULL-matcher','option-match-returns.json',lambda x:x[-1].__setitem__('eax',1)),
  ('resolver-formal-omission','state.json',lambda x:x['formal_inventory'].pop()),
  ('resolver-size','state.json',lambda x:x['args'].__setitem__('size',519)),
  ('resolver-input-identity','state.json',lambda x:x['args'].__setitem__('input',0)),
  ('resolver-output-ownership','state.json',lambda x:x['args'].__setitem__('output',0)),
  ('resolver-physical-trap','state.json',lambda x:x.__setitem__('pc',x['pc']-1)),
 ]:native_json(label,name,change)
 seq=int(re.findall(r'^trap seq=(\d+) rva=0001335b ',(a.original/'events.txt').read_text(),re.M)[0])
 for label,name,offset in [
  ('original-integer-result','integer-1-return-registers.bin',12),
  ('original-public-input','integer-0-api-isspace-entry-registers.bin',44),
  ('original-isdigit-mask','integer-0-api-isdigit-return-registers.bin',12),
  ('original-atol-result','integer-1-api-atol-return-registers.bin',12),
  ('original-public-FP','locale-integer-0-api-isspace-return-fp.bin',4),
  ('original-public-errors','locale-integer-0-api-isspace-return-errors.bin',0),
  ('original-public-table','locale-integer-0-api-isdigit-entry-ctype.bin',100),
  ('original-processor-write','actions-action-return-3-target-data.bin',0xf84),
  ('original-NULL-callback','actions-outer-return-7-registers.bin',12),
  ('original-resolver-CRT-state','state-%03d.bin'%seq,0x90),
  ('original-provider-isspace','integer-provider-isspace-000.bin',0),
 ]:raw(label,name,offset)
 packet=json.loads(PACKET.read_text());old=json.loads(Path(packet['old_field_map']['path']).read_text());current=json.loads((PROJECT/c['field_map']).read_text());derive(old,current,packet)
 for label,change in [
  ('additional-field-layout',lambda x:x['modeled_bss'][0].__setitem__('native_offset',4)),
  ('original-storage-alias',lambda x:x['modeled_bss'][0].__setitem__('original_rva',0)),
  ('initialized-default',lambda x:x['modeled_initialized'][0].__setitem__('default',0)),
  ('extra-consumer-frontier',lambda x:x['command_line'].__setitem__('unknown_semantic_consumer_frontier',0)),
  ('literal-tail-credit',lambda x:x['value_copy'].__setitem__('unmodeled_tail_bytes',0)),
  ('unknown-state-credit',lambda x:x['multibyte_scanner'].__setitem__('unknown_initialized_bytes',0)),
  ('allocation-size',lambda x:x.__setitem__('allocation_bytes',1)),
 ]:
  changed=copy.deepcopy(current);change(changed);reject(label,'field-map derivation',lambda:derive(old,changed,packet))
 for directory in (a.original,a.native):
  inputs=json.loads((directory/'audit-binding.json').read_text())['inputs'];derived=validate_policy(directory,c,inputs);key='decomp/vc5.10/c1/'+c['field_map']
  assert derived==dict(inputs,**{key:sha(PROJECT/c['field_map'])})
  for label,name in [('candidate-set','candidate.json'),('accepted-mask-config','functions.json')]+([('restored-byte-union','ranges.txt')]if directory==a.original else[]):
   content=(directory/name).read_bytes()+b' '
   with tempfile.TemporaryDirectory(prefix='integer-policy-control-',dir=directory.parent)as tmp:
    path=Path(tmp)/name;path.write_bytes(content);d=Overlay(directory,{name:path});reject(label,directory.name,lambda:validate_policy(d,c,inputs))
 result=dict(status='all affected controls rejected',count=len(controls),seconds=time.perf_counter()-started,controls=controls,execution_receipts=dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json')),validators={str(Path(__file__)):sha(Path(__file__)), 'scripts/integer_state.py':sha(PROJECT/'scripts/integer_state.py'),'scripts/integer_policy.py':sha(PROJECT/'scripts/integer_policy.py')},policy_packet_sha256=sha(PACKET))
 a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))

if __name__=='__main__':main()
