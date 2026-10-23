#!/usr/bin/env python3
"""Independently validate a completed accepted trial without changing its index."""
import argparse
import copy
import json
from pathlib import Path
from accept import PROJECT,validate

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('candidate');parser.add_argument('receipt',type=Path)
args=parser.parse_args()
current=json.loads((PROJECT/'functions.json').read_text())
base=copy.deepcopy(current);assert args.candidate in base['accepted'];base['accepted'].remove(args.candidate)
assert validate(base,json.loads(args.receipt.read_text()),args.candidate,review_only=True)==current
assert current==json.loads((PROJECT/'functions.json').read_text())
print('Current accepted trial, fresh native execution, reused immutable original and full audits pass; index unchanged')
