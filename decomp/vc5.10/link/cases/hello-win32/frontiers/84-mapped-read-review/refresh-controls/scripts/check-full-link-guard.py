#!/usr/bin/env python3
"""Native static control: reject a captured COFF data alias before registration."""
import argparse
import hashlib
import importlib
import json
from pathlib import Path
import time
from binary import PE

BINARY=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path,required=True)
args=parser.parse_args()
start=time.perf_counter()
checker=importlib.import_module('full-link-preservation')
original=PE(BINARY.parents[2]/'toolchains/5.10/bin/LINK.EXE')
rebuilt=PE(BINARY/'build/link.exe');map_text=(BINARY/'build/link.map').read_text()
setup=time.perf_counter()
positive=checker.check_closed_slot(original,rebuilt,map_text)
alias=BINARY/'cases/hello-win32/full-link-guard-controls/data-alias.obj'
expected='loaded-data/address-taking slot alias is outside closed contract'
try:checker.check_closed_slot(original,rebuilt,map_text,[alias])
except AssertionError as error:
    assert str(error)==expected,str(error)
else:raise AssertionError('captured data alias was accepted')
end=time.perf_counter()
report=dict(positive=positive,negative=dict(case='captured original-compiler .data initializer taking image_write_headers address',input_sha256=hashlib.sha256(alias.read_bytes()).hexdigest(),rejected=True,reason=expected),elapsed_seconds=dict(setup=setup-start,checks=end-setup,total=end-start),execution='Native Python static checker only; reads cached PE/COFF evidence. No Wine, compiler or original execution.')
args.report.write_text(json.dumps(report,indent=2)+'\n')
print('Closed-slot guard passes; captured data alias rejected (%.3f s).'%(end-start))
