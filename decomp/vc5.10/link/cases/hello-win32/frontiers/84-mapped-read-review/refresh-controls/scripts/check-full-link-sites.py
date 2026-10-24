#!/usr/bin/env python3
"""Reject cached-site loss even when the aggregate import set is unchanged."""
import argparse
import copy
import hashlib
import importlib
import json
from pathlib import Path
import time
from binary import PE

BINARY=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--input-report',type=Path,required=True)
parser.add_argument('--report',type=Path,required=True)
args=parser.parse_args();start=time.perf_counter()
checker=importlib.import_module('full-link-preservation')
report=json.loads(args.input_report.read_text());rebuilt=PE(BINARY/'build/link.exe')
positive=checker.check_resolution(report,rebuilt);rejected=[]
callback,=report['register_import_inputs']['callback_preservation_contracts']
rebuilt_callback_site=str(callback['rebuilt']['site'])
def negative(name,mutate):
    changed=copy.deepcopy(report);mutate(changed)
    try:checker.check_resolution(changed,rebuilt)
    except (AssertionError,KeyError):rejected.append(name);return
    raise AssertionError('site mutation accepted: '+name)
def sites(r):return r['functions'][checker.OWNER]['register_imports']
negative('lost one cached fprintf while aggregate identity survives',lambda r:sites(r)['original']['resolved_register_calls'].pop(str(0x44488e)))
negative('wrong late cached identity',lambda r:sites(r)['original']['resolved_register_calls'][str(0x444915)].__setitem__('name','sprintf'))
negative('missing strict cached-call completeness',lambda r:sites(r).__setitem__('require_all_register_calls_resolved',False))
negative('omitted original callback preservation application',lambda r:sites(r)['original'].__setitem__('callback_preservation',{}))
negative('wrong applied callback slot',lambda r:sites(r)['rebuilt']['callback_preservation'][rebuilt_callback_site].__setitem__('slot',0))
negative('wrong applied callback witness digest',lambda r:sites(r)['original']['callback_preservation'][str(0x444795)]['evidence'].__setitem__('sha256','0'*64))
negative('extra opaque original callback',lambda r:sites(r)['original']['other_unresolved_calls'].append(0x444865))
negative('dropped provider preservation bit',lambda r:r['register_import_inputs']['preservation_contracts'][1].__setitem__('preserves',['edi']))
end=time.perf_counter()
args.report.write_text(json.dumps(dict(positive=positive,rejected=rejected,negative_count=len(rejected),input_report_sha256=hashlib.sha256(args.input_report.read_bytes()).hexdigest(),checker_sha256=hashlib.sha256((BINARY/'scripts/full-link-preservation.py').read_bytes()).hexdigest(),elapsed_seconds=end-start,execution='Native static validation only; no compiler, target execution or Wine.'),indent=2)+'\n')
print('Exact site gate passes; %d focused mutations rejected (%.3f s).'%(len(rejected),end-start))
