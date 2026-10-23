#!/usr/bin/env python3
"""Native-only rejection checks over immutable copies of the real main trial."""
import argparse
import json
from pathlib import Path
import re
import shutil
import tempfile
from accept import verify
from audit import audit_command,audit_hashes
from mask import load,sha
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--audit',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
a.original=a.original.resolve();a.native=a.native.resolve();a.audit=a.audit.resolve()
config=load()[0];verify(config,'main',a.original,a.native,a.audit)
results=[]
def change_raw(directory,name,edit):
 path=directory/name;path.write_text(edit(path.read_text()))
 receipt=json.loads((directory/'receipt.json').read_text());receipt['raw_sha256'][name]=sha(path.read_bytes());(directory/'receipt.json').write_text(json.dumps(receipt))
def missing_entry(o,n,g):change_raw(o,'events.txt',lambda s:s.replace('entry 010014a5\n',''))
def missing_native_entry(o,n,g):change_raw(n,'trace.txt',lambda s:s.replace('main\n',''))
def missing_frontier(o,n,g):change_raw(o,'events.txt',lambda s:re.sub(r'^trap .*\n','',s,flags=re.M))
def forged_native_pc(o,n,g):
 def edit(s):
  d=json.loads(s);d['function']='main';return json.dumps(d)
 change_raw(n,'state.json',edit)
def wrong_trial(o,n,g):
 for directory in (o,n):
  data=json.loads((directory/'configuration.json').read_text());data['functions'][0]['status']='accepted';raw=json.dumps(data).encode();(directory/'configuration.json').write_bytes(raw)
  receipt=json.loads((directory/'receipt.json').read_text());receipt['configuration_sha256']=sha(raw);(directory/'receipt.json').write_text(json.dumps(receipt))
def swapped_raw(o,n,g):(o/'events.txt').write_text('exit 1\n')
def changed_audit_object(o,n,g):
 d=json.loads((g/'receipt.json').read_text());d['execution_inputs_before']['main_object']['sha256']='0'*64;d['execution_inputs_after']=d['execution_inputs_before'];(g/'receipt.json').write_text(json.dumps(d))
def changed_residual(o,n,g):
 d=json.loads((g/'import-residual.json').read_text());d['qualified_imports']=[];(g/'import-residual.json').write_text(json.dumps(d))
for name,edit,expected in [('candidate-not-entered-original',missing_entry,'candidate was not entered'),('candidate-not-entered-native',missing_native_entry,'candidate was not entered'),('missing-frontier',missing_frontier,'missing/ambiguous raw original'),('forged-native-function',forged_native_pc,'GDB PC outside'),('both-stale-trial',wrong_trial,'stale/different candidate'),('raw-capture-swap',swapped_raw,'raw observation changed'),('copied-audit-wrong-object',changed_audit_object,'audit tool/PE'),('modified-residual',changed_residual,'audit tool/PE')]:
 with tempfile.TemporaryDirectory(prefix='main-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
  base=Path(temporary);o=base/'original';n=base/'native';g=base/'audit';shutil.copytree(a.original,o);shutil.copytree(a.native,n);shutil.copytree(a.audit,g)
  # Rebind only location fields for byte-identical scratch copies.
  receipt=json.loads((g/'receipt.json').read_text());receipt['command']=audit_command(config,g);receipt['execution_inputs_before']=receipt['execution_inputs_after']=audit_hashes(config,g);(g/'receipt.json').write_text(json.dumps(receipt))
  edit(o,n,g)
  try:verify(config,'main',o,n,g)
  except AssertionError as error:
   message=str(error)
   assert expected in message,(name,message)
   results.append(dict(name=name,rejected=True,reason=message))
  else:raise AssertionError(name+' accepted')
a.output.write_text(json.dumps(dict(schema=1,original=str(a.original.resolve()),native=str(a.native.resolve()),audit=str(a.audit.resolve()),cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
