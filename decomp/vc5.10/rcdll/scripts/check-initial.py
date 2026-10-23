#!/usr/bin/env python3
"""Validate actual initial raw frontier pair; optionally exercise rejection paths."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import tempfile
from protocol import initial_pair,sha

p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--negative',action='store_true')
a=p.parse_args();result=initial_pair(a.original,a.native)
checks=[]
if a.negative:
    scratch=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll'
    with tempfile.TemporaryDirectory(prefix='initial-rejections-',dir=scratch)as directory:
        directory=Path(directory)
        def rejected(label,mutate):
            old=directory/(label+'-original');new=directory/(label+'-native')
            shutil.copytree(a.original,old);shutil.copytree(a.native,new)
            mutate(old,new)
            try:initial_pair(old,new)
            except (AssertionError,KeyError):checks.append(label)
            else:raise AssertionError('accepted invalid evidence '+label)
        def stale(old,new):
            c=json.loads((new/'candidate.json').read_text());c['files']['decomp/vc5.10/rcdll/functions.json']='0'*64;(new/'candidate.json').write_text(json.dumps(c))
        rejected('stale-candidate',stale)
        def wrong_trial(old,new):
            c=json.loads((old/'functions.json').read_text());c['trial_function']='RC';(old/'functions.json').write_text(json.dumps(c))
        rejected('wrong-trial-snapshot',wrong_trial)
        def raw_missing(old,new):
            f=old/'events.txt';f.write_text('\n'.join(line for line in f.read_text().splitlines()if not line.startswith('frontier '))+'\n')
            r=json.loads((old/'receipt.json').read_text());r['files']['events.txt']={'bytes':f.stat().st_size,'sha256':sha(f)};(old/'receipt.json').write_text(json.dumps(r))
        rejected('raw-original-frontier-missing-even-with-updated-receipt',raw_missing)
        def native_missing(old,new):
            f=new/'snapshot.json';f.write_text('{"kind":"frontier","rva":null,"function":null,"executed_byte":"cc"}')
            r=json.loads((new/'receipt.json').read_text());r['files']['snapshot.json']={'bytes':f.stat().st_size,'sha256':sha(f)};(new/'receipt.json').write_text(json.dumps(r))
        rejected('raw-native-frontier-missing-even-with-updated-receipt',native_missing)
        def changed_raw(old,new):
            f=old/'globals.bin';f.write_bytes(b'changed')
        rejected('raw-global-state-tampered',changed_raw)
        def wrong_module(name):
            def mutate(old,new):
                f=old/name;f.write_bytes(b'wrong loaded module')
                r=json.loads((old/'receipt.json').read_text());r['files'][name]={'bytes':f.stat().st_size,'sha256':sha(f)}
                r['actual_target_verified']=r['actual_crt_verified']=True
                (old/'receipt.json').write_text(json.dumps(r))
            return mutate
        rejected('wrong-loaded-target-with-refreshed-receipt',wrong_module('loaded-target.bin'))
        rejected('wrong-loaded-crt-with-refreshed-receipt',wrong_module('loaded-crt.bin'))
        def wrong_native_function(old,new):
            f=new/'snapshot.json';raw=json.loads(f.read_text());raw['function']='RC';f.write_text(json.dumps(raw))
            r=json.loads((new/'receipt.json').read_text());r['files']['snapshot.json']={'bytes':f.stat().st_size,'sha256':sha(f)};(new/'receipt.json').write_text(json.dumps(r))
        rejected('wrong-native-snapshot-function-with-refreshed-receipt',wrong_native_function)
        def wrong_native_pc(old,new):
            f=new/'snapshot.json';raw=json.loads(f.read_text());raw['raw_pc']+=1;f.write_text(json.dumps(raw))
            r=json.loads((new/'receipt.json').read_text());r['files']['snapshot.json']={'bytes':f.stat().st_size,'sha256':sha(f)};(new/'receipt.json').write_text(json.dumps(r))
        rejected('wrong-native-snapshot-pc-with-refreshed-receipt',wrong_native_pc)
result['rejections']=checks
print(json.dumps(result,indent=2))
