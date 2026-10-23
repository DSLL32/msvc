#!/usr/bin/env python3
"""Recheck the frozen0049 boundary after correcting its word-reporting error.

No binaries are executed. Historical source, raw captures and acceptance remain
unchanged; current checks bind the old config, controller and archived ELF.
"""
import argparse,hashlib,json,runpy,sys
from pathlib import Path
import protocol
from protocol import HERE,sha
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
record=HERE/'cases/resources-rich-trailing-space/acceptance-0049.json';history=json.loads(record.read_text());configuration=history['configuration']
assert configuration['trial_function']=='rc_read_number'and len(configuration['accepted'])==48
original=Path(history['original']);native=Path(history['native']);frozen_sha=sha(original/'functions.json')
assert sha(original/'receipt.json')==history['proof']['original_receipt_sha256']and sha(native/'receipt.json')==history['proof']['native_receipt_sha256']
assert json.loads((original/'functions.json').read_text())==configuration
protocol.NATIVE_PROOF_IMAGE=native/'captured-native.elf'
def frozen_bind(snapshot,side):
    receipt=json.loads((snapshot/'receipt.json').read_text());saved=json.loads((snapshot/'candidate.json').read_text())
    assert saved['schema']==2 and sha(snapshot/'functions.json')==receipt['candidate_sha256']==frozen_sha
    assert json.loads((snapshot/'functions.json').read_text())==configuration
    assert saved['files']['decomp/vc5.10/rcdll/functions.json']==frozen_sha
    for name,info in receipt['files'].items():
        path=snapshot/name;assert path.stat().st_size==info['bytes']and sha(path)==info['sha256'],'changed raw capture '+name
    if side=='original':assert sha(snapshot/'debug-original.exe')==receipt['debugger_sha256']==saved['files']['decomp/vc5.10/rcdll/build/debug-original.exe']
    else:
        protocol.NATIVE_PROOF_IMAGE=snapshot/'captured-native.elf';assert sha(protocol.NATIVE_PROOF_IMAGE)==receipt['native_sha256']==saved['files']['decomp/vc5.10/rcdll/build/native/rcdll-host']
    for row in json.loads((snapshot/'compiled-source.json').read_text())['contents'].values():assert hashlib.sha256(row['text'].encode()).hexdigest()==row['sha256']
    return receipt
protocol.bind_current=frozen_bind
sys.argv=[str(HERE/'scripts/check-rich-pair.py'),'--stage','read-number','--original',str(original),'--native',str(native),'--output',str(a.output)]
runpy.run_path(str(HERE/'scripts/check-rich-pair.py'),run_name='__main__')
proof=json.loads(a.output.read_text());assert proof['number_parser']['next_hidden_inputs']['current_word']==34
proof['correction']=dict(history=str(record.relative_to(HERE)),history_sha256=sha(record),old_reported_word=32,raw_derived_word=34,reason='Previous proof constructed32 instead of deriving from the already-validated full raw source/cursor. Current source_cursor_word reads both actual original and native buffers.',old_evidence_unchanged=True,execution='No original or native executable run; archived raw evidence/ELF rechecked.',validation_versions=protocol.validation_versions())
a.output.write_text(json.dumps(proof,indent=2)+'\n')
