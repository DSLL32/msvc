"""Focused corruptions of the reached default-expression owner receipt.

Calls the same validator used by both production argument gates; no original,
native execution, promotion or recursive expression inspection occurs.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import time
from default_argument_owner import validate


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('proof',type=Path)
    parser.add_argument('--receipt',type=Path,required=True)
    args=parser.parse_args()
    started=time.perf_counter()
    proof=json.loads(args.proof.read_text())
    assert proof['function']=='_append_default_call_argument@8'
    directory=args.proof.parent
    original=json.loads((directory/'original-receipt.json').read_text())
    checkpoint=json.loads((directory/'native-checkpoint.json').read_text())
    config=json.loads((directory/'original-configuration.json').read_text())
    raw=(directory/'original-events.txt').read_text()
    checks=[]
    for name in ('control','missing-field','wrong-field-offset','short-field-read','unreadable-field','wrong-owner-register','wrong-native-owner','wrong-native-field','wrong-actual-argument'):
        receipt,native=copy.deepcopy(original),copy.deepcopy(checkpoint)
        formal=copy.deepcopy(next(row for row in proof['formals'] if row['name']=='expression'))
        anchors={row['name']:row for row in receipt['diagnostic_observations']}
        events=raw
        words={int(i):int(value,16) for i,value in re.findall(r'^argument (\d+) ([0-9a-f]+)$',events,re.M)}
        if name=='missing-field': del anchors['default_expression']
        elif name=='wrong-field-offset': anchors['default_expression']['original']-=4
        elif name=='short-field-read': anchors['default_expression']['read_bytes']=2
        elif name=='unreadable-field': anchors['default_expression']['read_succeeded']=False
        elif name=='wrong-owner-register':
            owner=next(row for row in receipt['ownership_register_observations'] if row['register']=='ebx')
            owner['value']+=4
            events=re.sub(r'^ownership-anchor ebx [0-9a-f]+$','ownership-anchor ebx %08x'%owner['value'],events,flags=re.M)
        elif name=='wrong-native-owner': native['caller_locals']['caller_argument']['local']='parameters'
        elif name=='wrong-native-field': native['caller_locals']['caller_expression']['value']+=4
        elif name=='wrong-actual-argument':
            formal['original_raw']+=4;words[1]+=4
            events=re.sub(r'^argument 1 [0-9a-f]+$','argument 1 %08x'%words[1],events,flags=re.M)
        digest=hashlib.sha256(events.encode()).hexdigest()
        receipt['ownership_entry_projection']['events_sha256']=digest
        try: validate(proof['function'],formal,anchors,native,words,receipt,events,config,digest)
        except (AssertionError,KeyError) as error:
            assert name!='control',str(error)
            checks.append(dict(test=name,result='held',reason=str(error)))
        else:
            assert name=='control',name+' unexpectedly passed'
            checks.append(dict(test=name,result='verified'))
    report=dict(schema=1,scope=__doc__.strip(),proof=str(args.proof),proof_sha256=sha(args.proof),checker_sha256=sha(Path(__file__)),validator_sha256=sha(Path(__file__).with_name('default_argument_owner.py')),checks=checks,elapsed_seconds=time.perf_counter()-started)
    args.receipt.parent.mkdir(parents=True,exist_ok=True)
    args.receipt.write_text(json.dumps(report,indent=2)+'\n')
    print(args.receipt)


if __name__=='__main__': main()
