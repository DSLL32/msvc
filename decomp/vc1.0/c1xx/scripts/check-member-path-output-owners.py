"""Production-preflight negatives for the exact caller-owned output locals.

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
    assert original['function'] in ('_resolve_member_pointer_path@12','_extended_to_double','_double_to_extended')
    formals=[row for row in original['formals'] if row['binding']['kind']=='caller-owned-pointer']
    assert len(formals)==(2 if original['function']=='_resolve_member_pointer_path@12' else 1) and all(row['binding']['original_register']=='esp' for row in formals)
    registers=['esp']
    directory=Path(tempfile.mkdtemp(prefix='member-path-output-negatives-',dir=CAPTURE_ROOT))
    (directory/'functions.json').write_bytes((args.proof.parent/'original-configuration.json').read_bytes())
    executable=BINARY/'build/native/C1XX3232.EXE'
    assert sha(executable)==original['native_executable_sha256']
    (directory/'build/native').mkdir(parents=True)
    (directory/'build/native/C1XX3232.EXE').symlink_to(executable)
    spec=importlib.util.spec_from_file_location('member_path_output_preflight',BINARY/'scripts/advance-frontier.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    module.BINARY=directory
    results=[]
    for name in ('control','missing-anchor','wrong-offset','swapped-output-addresses','wrong-native-owner','read-output-prestate'):
        if name=='swapped-output-addresses' and len(formals)==1:continue
        target=directory/name
        shutil.copytree(args.proof.parent,target)
        proof=copy.deepcopy(original)
        receipt=json.loads((target/'original-receipt.json').read_text())
        if name=='missing-anchor':
            receipt['ownership_register_observations']=[row for row in receipt['ownership_register_observations'] if row['register']!=registers[0]]
        if name=='wrong-offset':
            next(row for row in proof['formals'] if row['binding']['kind']=='caller-owned-pointer')['binding']['original_stack_offset']+=4
        if name=='swapped-output-addresses':
            native=json.loads((target/'native-checkpoint.json').read_text())
            left=native['caller_locals']['caller_virtual_base'];right=native['caller_locals']['caller_displacement']
            left['address'],right['address']=right['address'],left['address']
            (target/'native-checkpoint.json').write_text(json.dumps(native,indent=2)+'\n')
        if name=='read-output-prestate':
            native=json.loads((target/'native-checkpoint.json').read_text())
            native['caller_locals'][formals[0]['binding']['native_owner_field']]['value']=0
            (target/'native-checkpoint.json').write_text(json.dumps(native,indent=2)+'\n')
        if name=='wrong-native-owner':
            native=json.loads((target/'native-checkpoint.json').read_text())
            native['caller_locals'][formals[0]['binding']['native_owner_field']]['local']='wrong_local'
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
