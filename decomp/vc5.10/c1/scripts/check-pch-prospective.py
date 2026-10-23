#!/usr/bin/env python3
"""Validate only the bounded pending-tail raw input; cannot promote functions."""
import argparse,importlib.util,json
from pathlib import Path
spec=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'))
trial=importlib.util.module_from_spec(spec);spec.loader.exec_module(trial)
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();result=trial.check(args.original,args.native,prospective_tail_chain=True)
try:trial.check(args.original,args.native)
except AssertionError:pass
else:raise AssertionError('normal acceptance checker accepted prospective evidence')
assert result['promotion'] is False
print(json.dumps(result,indent=2))
