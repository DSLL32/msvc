"""Focused tests of the exact generated-object linkage input gate, no execution."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import time
from generated_linkage_owner import validate


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof',type=Path)
    parser.add_argument('--receipt',type=Path,required=True)
    args=parser.parse_args()
    started=time.perf_counter()
    proof=json.loads(args.proof.read_text())
    assert proof['function']=='_create_generated_object@8'
    receipt=json.loads((args.proof.parent/'original-receipt.json').read_text())
    native=json.loads((args.proof.parent/'native-checkpoint.json').read_text())
    original={row['name']:row for row in receipt['diagnostic_observations']}
    checks=[]
    for name in ('control','missing-depth','negative-depth','wrong-slot','short-slot-read','wrong-default-value','wrong-native-slot'):
        anchors,state=copy.deepcopy(original),copy.deepcopy(native)
        if name=='missing-depth': del anchors['linkage_depth']
        elif name=='negative-depth': anchors['linkage_depth']['value']=0xffffffff
        elif name=='wrong-slot': anchors['current_linkage']['original']-=4
        elif name=='short-slot-read': anchors['current_linkage']['read_bytes']=2
        elif name=='wrong-default-value': anchors['current_linkage']['value']=1
        elif name=='wrong-native-slot': state['observed_linkage_slot']+=4
        try: validate(proof['function'],anchors,state)
        except (AssertionError,KeyError) as error:
            assert name!='control',str(error)
            checks.append(dict(test=name,result='held',reason=str(error)))
        else:
            assert name=='control',name+' unexpectedly passed'
            checks.append(dict(test=name,result='verified'))
    report=dict(schema=1,scope='Same function-specific owner validator called by both production argument gates; synthetic receipt corruptions only, no original execution/promotion',proof=str(args.proof),proof_sha256=sha(args.proof),checker_sha256=sha(Path(__file__)),validator_sha256=sha(Path(__file__).with_name('generated_linkage_owner.py')),checks=checks,elapsed_seconds=time.perf_counter()-started)
    args.receipt.parent.mkdir(parents=True,exist_ok=True)
    args.receipt.write_text(json.dumps(report,indent=2)+'\n')
    print(args.receipt)


if __name__=='__main__': main()
