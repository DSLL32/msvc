"""Focused read-only production-gate checks for this goto cleanup ownership path.

Uses the frozen argument pair and matching native executable. Synthetic metadata
changes cannot promote a function or invoke an original; unchanged input must pass.
"""
import argparse
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
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
    parser.add_argument('--receipt',type=Path,required=True)
    args=parser.parse_args()
    started=time.perf_counter()
    original=json.loads(args.proof.read_text())
    assert original['function']=='_walk_goto_cleanup@16'
    directory=Path(tempfile.mkdtemp(prefix='goto-cleanup-input-negatives-',dir=CAPTURE_ROOT))
    (directory/'functions.json').write_bytes((args.proof.parent/'original-configuration.json').read_bytes())
    executable=BINARY/'build/native/C1XX3232.EXE'
    assert sha(executable)==original['native_executable_sha256']
    (directory/'build/native').mkdir(parents=True)
    (directory/'build/native/C1XX3232.EXE').symlink_to(executable)
    spec=importlib.util.spec_from_file_location('goto_argument_preflight',BINARY/'scripts/advance-frontier.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    module.BINARY=directory
    checks=[]
    names=('control','missing-root','wrong-root','wrong-previous-offset','wrong-label-offset','short-previous-read','unreadable-expression','wrong-expression-prestate','wrong-caller-to','wrong-caller-label','missing-caller-expression','wrong-native-label-kind')
    for name in names:
        target=directory/name
        shutil.copytree(args.proof.parent,target)
        proof=copy.deepcopy(original)
        receipt=json.loads((target/'original-receipt.json').read_text())
        native=json.loads((target/'native-checkpoint.json').read_text())
        anchors={row['name']:row for row in receipt['diagnostic_observations']}
        events=(target/'original-events.txt').read_text()
        if name=='missing-root': anchors['label_tail']['name']='missing_label_tail'
        elif name=='wrong-root': anchors['label_tail']['value']+=4
        elif name in ('wrong-previous-offset','wrong-label-offset'):
            formal,anchor=('record','tail_previous') if name=='wrong-previous-offset' else ('label','tail_label')
            anchors[anchor]['original']+=4
            next(row for row in proof['formals'] if row['name']==formal)['binding']['original_anchor_address']+=4
        elif name=='short-previous-read': anchors['tail_previous']['read_bytes']=2
        elif name=='unreadable-expression': anchors['expression_prestate']['read_succeeded']=False
        elif name=='wrong-expression-prestate': anchors['expression_prestate']['value']=1
        elif name=='wrong-caller-to': native['caller_locals']['caller_to']['value']+=4
        elif name=='wrong-caller-label': native['caller_locals']['caller_label']['value']+=4
        elif name=='missing-caller-expression': native['caller_locals'].pop('caller_expression')
        elif name=='wrong-native-label-kind': native['tail_kind']=0x34
        # Keep generic diagnostic raw-value consistency so the dedicated owner
        # gate, rather than an unrelated stale hash, rejects mutated owners.
        base=len(json.loads((directory/'functions.json').read_text())['checkpoint_state'])
        for i,row in enumerate(receipt['diagnostic_observations']):
            events=re.sub(r'^state %d [0-9a-f]+$'%(base+i),'state %d %08x'%(base+i,row['value']),events,flags=re.M)
        (target/'original-events.txt').write_text(events)
        digest=sha(target/'original-events.txt')
        for key in ('ownership_entry_projection','first_entry_projection'):
            if key in receipt: receipt[key]['events_sha256']=digest
        (target/'original-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        (target/'native-checkpoint.json').write_text(json.dumps(native,indent=2)+'\n')
        for filename in proof['evidence_sha256']:
            proof['evidence_sha256'][filename]=sha(target/filename)
        (target/'validation.json').write_text(json.dumps(proof,indent=2)+'\n')
        sys.argv=['advance-frontier.py','--case',original['case'],'--candidate',original['function'],'--argument-proof',str(target/'validation.json'),'--check-arguments-only']
        output=io.StringIO()
        try:
            with contextlib.redirect_stdout(output): module.main()
        except (AssertionError,KeyError) as error:
            assert name!='control',str(error)
            checks.append(dict(test=name,result='held',reason=str(error)))
        else:
            assert name=='control','corrupted owner passed: '+name
            checks.append(dict(test=name,result='verified',output=output.getvalue().strip()))
    report=dict(schema=1,scope='Read-only production argument preflight using frozen current function pair; focused synthetic metadata corruptions, no original execution or promotion',proof=str(args.proof),proof_sha256=sha(args.proof),checker_sha256=sha(Path(__file__)),owner_validator_sha256=sha(BINARY/'scripts/goto_cleanup_inputs.py'),checks=checks,elapsed_seconds=time.perf_counter()-started)
    args.receipt.parent.mkdir(parents=True,exist_ok=True)
    args.receipt.write_text(json.dumps(report,indent=2)+'\n')
    print(args.receipt)


if __name__=='__main__': main()
