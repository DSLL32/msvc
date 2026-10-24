#!/usr/bin/env python3
"""Bind the normal helper and later restore-only observation helper separately."""
import argparse
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(normal, auxiliary):
    old = (normal / 'dll-capture.c').read_text()
    new = (auxiliary / 'dll-capture.c').read_text()
    before = '                        BYTE tables[768];memset(tables,0xa5,sizeof(tables));write_mem(base+0xbe970,tables,sizeof(tables));\n'
    after = '                        BYTE *before,tables[768];before=(BYTE *)malloc(0xf69c);if(!before)fail("checksum pre-seed observer allocation");\n                        readmem(base+0xb9000,before,0xf69c);save_blob("checksum-entry-before-seed-BSS.bin",before,0xf69c);free(before);\n                        memset(tables,0xa5,sizeof(tables));write_mem(base+0xbe970,tables,sizeof(tables));\n'
    assert old.count(before) == 1 and old.replace(before, after) == new, 'controller delta exceeds exact pre-seed observation'
    guard = '                    if(getenv("C1XX_TEST_INIT_RESTORE")) {\n'
    assert guard + before in old and guard + after in new, 'observation moved outside restore-only condition'
    records = {}
    for label, directory, seed in (('normal', normal, None), ('restore_auxiliary', auxiliary, 'restore-nonzero')):
        receipt = json.loads((directory / 'receipt.json').read_text())
        trial = json.loads((directory / 'trial.json').read_text())
        assert trial['trial'] == '_initialize_checksums' and trial['test_seed'] == seed
        assert receipt['helper_sha256'] == sha(directory / 'dll-capture.exe')
        assert receipt['mode'] == 'masked' and receipt['exit'] == 125
        assert sha(directory / 'trial.json') == receipt['snapshot_inputs']['build/trial.json']
        for name in ('dll-capture.exe', 'dll-capture.c', 'locale-observer.inc', 'invoke-arguments.h', 'options-fields.h'):
            assert sha(directory / name) == receipt['artifacts'][name]['sha256']
        text = (directory / 'events.txt').read_text()
        assert ('checksum-seed ' in text) == bool(seed)
        if seed:
            assert (directory / 'checksum-entry-before-seed-BSS.bin').is_file()
        records[label] = dict(directory=str(directory.resolve()), original_receipt_sha256=sha(directory / 'receipt.json'), helper_sha256=receipt['helper_sha256'], helper_source_sha256=sha(directory / 'dll-capture.c'), trial_sha256=sha(directory / 'trial.json'), test_seed=seed, candidate_sha256=trial['candidate_sha256'], ranges_sha256=hashlib.sha256(json.dumps(trial['ranges'], sort_keys=True).encode()).hexdigest(), case_inputs=trial['case_inputs'])
    for name in ('locale-observer.inc', 'invoke-arguments.h', 'options-fields.h'):
        assert (normal / name).read_bytes() == (auxiliary / name).read_bytes(), 'other compiled controller input changed'
    assert records['normal']['ranges_sha256'] == records['restore_auxiliary']['ranges_sha256'] and records['normal']['case_inputs'] == records['restore_auxiliary']['case_inputs'], 'target mask or case data changed'
    return dict(kind='exact restore-only observation delta; normal helper remains its own immutable controller', controllers=records, normal_semantics_changed=False, restore_observation='reads 63132 target bytes before the already declared 768-byte data seed; no target write added', checker_sha256=sha(Path(__file__)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('normal', type=Path)
    parser.add_argument('auxiliary', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'refuse validation receipt overwrite'
    result = check(args.normal, args.auxiliary)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: exact restore-only observation; both original helper identities retained')
