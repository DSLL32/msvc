#!/usr/bin/env python3
"""Recheck frozen original execution and an archived native completion, read-only.

Original input/mask/controller bytes remain frozen. A native-only recapture may
consume later shared shims; it must explicitly use the same frozen trial config.
No historical receipt is rewritten and no original executable is launched.
"""
import argparse
import json
from pathlib import Path
import runpy
import sys
import protocol
from protocol import HERE,sha
p=argparse.ArgumentParser();p.add_argument('--original',type=Path);p.add_argument('--native',type=Path)
a=p.parse_args();record=json.loads((HERE/'cases/help/native-recheck.json').read_text())
original=a.original or Path(record['original']);native=a.native or Path(record['native'])
configuration=json.loads((original/'functions.json').read_text())
assert configuration['trial_function']=='rc_format' and len(configuration['accepted'])==10
frozen_sha=sha(original/'functions.json');assert frozen_sha==record['configuration_sha256']
protocol.NATIVE_PROOF_IMAGE=native/'captured-native.elf'

def frozen_bind(snapshot,side):
    receipt=json.loads((snapshot/'receipt.json').read_text())
    saved=json.loads((snapshot/'candidate.json').read_text())
    assert saved['schema']==2
    assert sha(snapshot/'functions.json')==receipt['candidate_sha256']==frozen_sha
    assert json.loads((snapshot/'functions.json').read_text())==configuration
    assert saved['files']['decomp/vc5.10/rcdll/functions.json']==frozen_sha
    for name,info in receipt['files'].items():
        path=snapshot/name
        assert path.stat().st_size==info['bytes'] and sha(path)==info['sha256'],'changed raw capture '+name
    if side=='original':
        assert sha(snapshot/'debug-original.exe')==receipt['debugger_sha256']==saved['files']['decomp/vc5.10/rcdll/build/debug-original.exe']
    else:
        protocol.NATIVE_PROOF_IMAGE=snapshot/'captured-native.elf'
        assert sha(protocol.NATIVE_PROOF_IMAGE)==receipt['native_sha256']==saved['files']['decomp/vc5.10/rcdll/build/native/rcdll-host']
    source=json.loads((snapshot/'compiled-source.json').read_text())
    for row in source['contents'].values():
        import hashlib
        assert hashlib.sha256(row['text'].encode()).hexdigest()==row['sha256']
    return receipt
protocol.bind_current=frozen_bind
# Reuse the same rejection matrix, against copied raw artifacts, without changing
# live functions.json or blessing current binaries as old execution evidence.
sys.argv=[str(HERE/'scripts/check-trial.py'),'--original',str(original),'--native',str(native),'--frozen-trial','rc_format']
runpy.run_path(str(HERE/'scripts/check-trial.py'),run_name='__main__')
