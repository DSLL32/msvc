#!/usr/bin/env python3
"""Native/read-only negative checks for masking and fresh trial binding."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from accept import BINARY, bind_capture, trial_for, verify_frontier, audit_trial, sha
from mask import construct


def rejected(call):
    try:
        call()
    except (AssertionError, ValueError, subprocess.CalledProcessError):
        return
    raise AssertionError('negative unexpectedly accepted')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    args = parser.parse_args()
    current = json.loads((BINARY / 'functions.json').read_text())
    assert current['accepted'] == [] and current['review_gate'] == 'pending before first acceptance'
    bind_capture(args.original, current, 'original')
    bind_capture(args.native, current, 'native')
    verify_frontier(args.original, args.native, '_mainCRTStartup', current)
    checks = []
    def negative(name, call):
        rejected(call); checks.append(name)
    negative('unknown candidate', lambda:trial_for(current, '_invented'))
    negative('stub candidate', lambda:trial_for(current, '_main'))
    implemented = copy.deepcopy(current); implemented['driver'][0]['status'] = 'implemented'
    trial = trial_for(implemented, '_main')
    assert trial['accepted'] == ['_main'] and current['accepted'] == []
    negative('audit explicit candidate rejects actual stub graph', lambda:audit_trial(write_trial(trial)))
    wrong = copy.deepcopy(current); wrong['target']['sha256'] = '0' * 64
    negative('wrong target', lambda:construct(wrong))
    wrong = copy.deepcopy(current); wrong['accepted'] = ['_invented']
    negative('unknown accepted vertex', lambda:construct(wrong))
    wrong = copy.deepcopy(current); wrong['driver'].append(dict(name='_overlap',start=0x40de91,end=0x40de92))
    negative('overlapping function owners', lambda:construct(wrong))
    wrong = copy.deepcopy(current); wrong['driver'].append(dict(name='_iat_is_not_code',start=0x401000,end=0x401004))
    negative('IAT cannot be claimed as code', lambda:construct(wrong))
    wrong = copy.deepcopy(implemented); wrong['driver'][0]['pending_extent'] = True; wrong['accepted'] = ['_main']
    negative('pending extent cannot unmask', lambda:construct(wrong))
    wrong = copy.deepcopy(current); wrong['accepted'] = ['_main']
    negative('stub cannot unmask', lambda:construct(wrong))
    negative('nontrial original config', lambda:bind_capture(args.original, trial, 'original'))
    negative('nontrial native config', lambda:bind_capture(args.native, trial, 'native'))
    stale = copy.deepcopy(current); stale['review_gate'] = 'stale mutually matching snapshots'
    for kind, source in [('original',args.original),('native',args.native)]:
        directory = clone(source)
        data = (json.dumps(stale,indent=2)+'\n').encode()
        (directory/'functions.json').unlink(); (directory/'functions.json').write_bytes(data)
        receipt = json.loads((directory/'receipt.json').read_text())
        receipt['configuration_sha256'] = sha(data)
        receipt['files']['functions.json'] = dict(bytes=len(data),sha256=sha(data))
        (directory/'receipt.json').write_text(json.dumps(receipt))
        negative('mutually matching stale ' + kind + ' snapshot', lambda directory=directory,kind=kind:bind_capture(directory,current,kind))
    negative('empty expected frontier', lambda:verify_frontier(args.original,args.native,'',current))
    negative('wrong frontier', lambda:verify_frontier(args.original,args.native,'_main',current))
    for key, value in [('native_sha256','0'*64), ('rebuilt_sha256','0'*64),
                       ('actual_loaded_crt_verified',False), ('actual_loaded_target_hfile_verified',False)]:
        directory = clone(args.original)
        receipt = json.loads((directory / 'receipt.json').read_text()); receipt[key] = value
        (directory / 'receipt.json').write_text(json.dumps(receipt))
        negative('changed ' + key, lambda directory=directory:bind_capture(directory,current,'original'))
    report = dict(positive='both snapshots bind current bytes and match initial startup trap/empty streams',
                  negative_checks=checks, negative_count=len(checks), accepted_functions=0)
    (BINARY / 'cases/scaffold-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


def write_trial(trial):
    path = BINARY / 'build/scaffold-wrong-trial.json'
    path.write_text(json.dumps(trial,indent=2)+'\n')
    return path


def clone(source):
    directory = Path(tempfile.mkdtemp(prefix='negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/link'))
    for path in source.iterdir():
        if path.is_file():
            if path.name=='receipt.json':shutil.copyfile(path,directory/path.name)
            else:(directory/path.name).symlink_to(path)
    return directory


if __name__ == '__main__':main()
