"""Negative metadata checks for the reached three-owner argument proof.

Use its frozen original configuration and current matching executable in scratch.
Only the read-only argument preflight runs; no Wine, replay or promotion occurs.
"""
import argparse
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import shutil
import sys
import tempfile
import time
from case_paths import BINARY, CAPTURE_ROOT


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof',type=Path)
    args=parser.parse_args()
    started=time.perf_counter()
    original=json.loads(args.proof.read_text())
    assert original['function']=='_declare_qualified_member@12'
    directory=Path(tempfile.mkdtemp(prefix='qualified-member-owner-negatives-',dir=CAPTURE_ROOT))
    (directory/'functions.json').write_bytes((args.proof.parent/'original-configuration.json').read_bytes())
    executable=BINARY/'build/native/C1XX3232.EXE'
    assert sha(executable)==original['native_executable_sha256']
    (directory/'build/native').mkdir(parents=True)
    (directory/'build/native/C1XX3232.EXE').symlink_to(executable)
    spec=importlib.util.spec_from_file_location('qualified_argument_preflight',BINARY/'scripts/advance-frontier.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    module.BINARY=directory
    results=[]
    for name in ('control','missing-anchor','swapped-anchors','wrong-native-owner'):
        target=directory/name
        shutil.copytree(args.proof.parent,target)
        proof=copy.deepcopy(original)
        receipt=json.loads((target/'original-receipt.json').read_text())
        if name=='missing-anchor':
            receipt['ownership_register_observations']=[row for row in receipt['ownership_register_observations'] if row['register']!='esi']
        if name=='swapped-anchors':
            anchors={row['register']:row for row in receipt['ownership_register_observations']}
            anchors['ebx']['value'],anchors['ebp']['value']=anchors['ebp']['value'],anchors['ebx']['value']
            raw=(target/'original-events.txt').read_text()
            import re
            raw=re.sub(r'^ownership-anchor (ebx|ebp) [0-9a-f]{8}$',lambda m:'ownership-anchor %s %08x'%(m[1],anchors[m[1]]['value']),raw,flags=re.M)
            (target/'original-events.txt').write_text(raw)
            receipt['ownership_entry_projection']['events_sha256']=sha(target/'original-events.txt')
        if name=='wrong-native-owner':
            native=json.loads((target/'native-checkpoint.json').read_text())
            native['caller_locals']['caller_symbol']['value']+=4
            (target/'native-checkpoint.json').write_text(json.dumps(native,indent=2)+'\n')
        (target/'original-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        for filename in proof['evidence_sha256']:
            proof['evidence_sha256'][filename]=sha(target/filename)
        (target/'validation.json').write_text(json.dumps(proof,indent=2)+'\n')
        sys.argv=['advance-frontier.py','--case',original['case'],'--candidate',original['function'],'--argument-proof',str(target/'validation.json'),'--check-arguments-only']
        output=io.StringIO()
        try:
            with contextlib.redirect_stdout(output):module.main()
        except AssertionError as error:
            assert name!='control',str(error)
            results.append(dict(test=name,result='held',reason=str(error)))
        else:
            assert name=='control','corrupted metadata passed: '+name
            results.append(dict(test=name,result='verified',output=output.getvalue().strip()))
    report=dict(scope='Read-only production argument preflight with frozen original configuration; synthetic corruptions, no promotion or original execution',proof=str(args.proof),proof_sha256=sha(args.proof),checks=results,elapsed_seconds=time.perf_counter()-started)
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(directory/'receipt.json')


if __name__=='__main__':main()
