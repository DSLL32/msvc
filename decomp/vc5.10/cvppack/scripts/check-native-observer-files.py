#!/usr/bin/env python3
"""Offline artifact/phase controls for the derived next-block observer binding."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import time
from state import BINARY,sha
spec=importlib.util.spec_from_file_location('native_observers',BINARY/'scripts/native-observer-files.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
pair=BINARY/'cases/hello-debug-dll/evidence/0113-next-hash-block-observed';capture=pair/'native-eof';config=json.loads((capture/'configuration.json').read_text());start=time.perf_counter();baseline=module.bind(capture,config);controls=[]
files=['receipt.json','state.json','configuration.json','gdb-state.py','commands.gdb',*module.EXTRAS]
with tempfile.TemporaryDirectory(prefix='cvpack-observer-binding-',dir=Path.home()/'.ai-tmp')as temporary:
 root=Path(temporary)
 def reset():
  for name in files:shutil.copyfile(capture/name,root/name)
 def rewrite(name,mutate):
  value=json.loads((root/name).read_text());mutate(value);(root/name).write_text(json.dumps(value,indent=2)+'\n')
 def refresh_receipt(name):
  def update(receipt):
   for row in receipt['outputs']:
    if row['name']==name:row.update(sha256=sha(root/name),bytes=(root/name).stat().st_size)
  rewrite('receipt.json',update)
 def reject(name,mutate,reason):
  reset();mutate()
  try:module.bind(root,config)
  except (AssertionError,FileNotFoundError,KeyError)as error:
   assert reason in str(error),(name,str(error));controls.append(dict(name=name,diagnostic=str(error)))
  else:raise AssertionError('accepted '+name)
 reset();assert module.bind(root,config)==baseline
 reject('missing-executed-module',lambda:(root/'hash-chain-gdb.py').unlink(),'')
 reject('changed-executed-module',lambda:(root/'hash-chain-gdb.py').write_bytes((root/'hash-chain-gdb.py').read_bytes()+b'\n# changed\n'),'reviewed source')
 reject('missing-first-next-configuration',lambda:(root/'first-next-block-config.json').unlink(),'')
 reject('wrong-first-next-exact-trial',lambda:rewrite('first-next-block-config.json',lambda v:v.update(observe_next_hash_output_block=True)),'exact phase configuration')
 reject('missing-first-next-state',lambda:(root/'first-next-block-state.json').unlink(),'')
 reject('swapped-other-physical-phase',lambda:shutil.copyfile(capture/'first-address-callback-state.json',root/'first-next-block-state.json'),'actual captured phase')
 def coherent_anchor_mutation():
  rewrite('first-next-block-state.json',lambda v:v.update(pc=v['pc']+1))
  rewrite('state.json',lambda v:v['hash_chain_entry_phase'].update(pc=v['hash_chain_entry_phase']['pc']+1));refresh_receipt('state.json')
 reject('coherent-refreshed-state-wrong-physical-anchor',coherent_anchor_mutation,'physical/typed anchor')
 reject('changed-actual-install-command',lambda:(root/'commands.gdb').write_text((root/'commands.gdb').read_text().replace('helper.install(', 'other.install(')),'actual commands')
 reject('new-inventory-version-with-missing-files',lambda:rewrite('receipt.json',lambda v:v.update(observer_inventory=2)),'current receipt omits extras')
 def partial_inventory():
  rewrite('receipt.json',lambda v:v['outputs'].append(dict(name=module.EXTRAS[0],bytes=(root/module.EXTRAS[0]).stat().st_size,sha256=sha(root/module.EXTRAS[0]))))
 reject('partial-extra-output-inventory',partial_inventory,'incomplete receipt extra inventory')
 reset()
 def current_inventory(value):
  value['observer_inventory']=2
  value['outputs'] += [dict(name=name,bytes=(root/name).stat().st_size,sha256=sha(root/name))for name in module.EXTRAS]
 rewrite('receipt.json',current_inventory);prospective=module.bind(root,config);assert prospective['receipt_inventory']==2
 # This is a synthetic inventory-schema control, never a relabeled capture.
 (root/'hash-chain-gdb.py').write_bytes((root/'hash-chain-gdb.py').read_bytes()+b'\n# changed\n');refresh_receipt('hash-chain-gdb.py')
 try:module.bind(root,config)
 except AssertionError as error:assert 'reviewed source'in str(error);controls.append(dict(name='refreshed-output-hash-wrong-module',diagnostic=str(error)))
 else:raise AssertionError('accepted changed reviewed module')
result=dict(schema=1,offline_only=True,negative_count=len(controls),controls=controls,legacy_binding=baseline,positive_prospective_inventory2=True,qualification='Original/native receipts untouched; synthetic inventory2 is a schema control only. Current retained EOF capture actually used legacy inventory1.',seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/native-observer-files.py'))
(pair/'observer-file-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(controls=len(controls),seconds=result['seconds'])))
