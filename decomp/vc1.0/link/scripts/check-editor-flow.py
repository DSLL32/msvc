#!/usr/bin/env python3
"""Captured-only host guards for ordered native image mutation workflows."""
import copy,importlib.util,json,struct,tempfile
from pathlib import Path
spec=importlib.util.spec_from_file_location('editor_native_checker',Path(__file__).with_name('check-native.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
fixture,phase_name=m.CASES['editor-bind']
metadata=json.loads((fixture/'manifest.json').read_text());parser=m.parser_module(fixture,metadata)
item=next(r for r in metadata['phases'][phase_name]['inputs'] if r.get('from_case'))
expected=m.captured(fixture/item['path'],item)
image=bytearray(expected);pe,optional=parser.layout(image)
struct.pack_into('<I',image,pe+8,struct.unpack_from('<I',image,pe+8)[0]+17)
struct.pack_into('<I',image,optional+64,parser.checksum(image))
assert bytes(image)!=expected
with tempfile.TemporaryDirectory(prefix='link-editor-flow-',dir=m.SCRATCH/'vc1.0/link/native') as folder:
 work=Path(folder);(work/'client.exe').write_bytes(image)
 completed={'editor-rebase':dict(work=work,outputs={'client.exe':dict(sha256=m.sha(image))})}
 state={};actual=m.preceding_input(item,fixture,parser,completed,state)
 assert actual==image and actual!=expected and state['native_input_flow'][0]['source_case']=='editor-rebase'
 negatives=[]
 def reject(name,action):
  try:action()
  except AssertionError:negatives.append(name)
  else:raise AssertionError('Invalid flow accepted: '+name)
 reject('missing_predecessor',lambda:m.preceding_input(item,fixture,parser,{},{}))
 changed=bytearray(image);changed[1024]^=1;(work/'client.exe').write_bytes(changed)
 reject('changed_after_validation',lambda:m.preceding_input(item,fixture,parser,completed,{}))
 struct.pack_into('<I',changed,optional+64,parser.checksum(changed))
 (work/'client.exe').write_bytes(changed);completed['editor-rebase']['outputs']['client.exe']['sha256']=m.sha(changed)
 reject('different_serialized_input',lambda:m.preceding_input(item,fixture,parser,completed,{}))
 assert m.ordered_cases(['editor-release'])==['editor-rebase','editor-bind','editor-release']
 reject('unknown_dependency',lambda:m.ordered_cases(['unknown-editor-stage']))
 # An in-memory registry cycle is rejected before any executable can launch.
 original=m.CASES['editor-rebase'];fake=work/'cyclic';fake.mkdir()
 cyclic=copy.deepcopy(metadata);cyclic['phases'][phase_name]['inputs'][0]['from_case']={'case':'editor-bind','output_name':'client.exe'}
 (fake/'manifest.json').write_text(json.dumps(cyclic));m.CASES['editor-rebase']=(fake,phase_name)
 reject('dependency_cycle',lambda:m.ordered_cases(['editor-release']))
 m.CASES['editor-rebase']=original
 print(json.dumps(dict(status='pass',positive='Actual independently valid timestamp variant passed through unchanged, distinct from frozen input',rejected=negatives,seconds='unmeasured; host-only guards'),indent=2))
