#!/usr/bin/env python3
"""A native rebuild cannot change any original execution input or mask."""
import copy,json
from pathlib import Path
from system_info_reference import native_only_trial_refresh


def main():
    packet=json.loads(Path('cases/minimal/pool-bind-member-trial.json').read_text());original=(Path(packet['original'])/'trial.json').read_bytes();changed=json.loads(original);old=changed['bound_inputs']['build/native/c1xx-host'];changed['bound_inputs']['build/native/c1xx-host']='a'*64 if old!='a'*64 else 'b'*64
    assert native_only_trial_refresh(original,json.dumps(changed).encode())['only_changed_field']=='bound_inputs.build/native/c1xx-host'
    mutants=['mask','case-input','rebuilt','candidate','unchanged-native']
    for case in mutants:
        trial=copy.deepcopy(changed)
        if case=='mask':trial['ranges'][-1]['end_rva']+=1
        elif case=='case-input':trial['case_inputs']['cases/minimal/contract.json']='b'*64
        elif case=='rebuilt':trial['bound_inputs']['build/c1n32_l.dll']='b'*64
        elif case=='candidate':trial['candidate_sha256']='b'*64
        else:trial['bound_inputs']['build/native/c1xx-host']=old
        try:native_only_trial_refresh(original,json.dumps(trial).encode())
        except ValueError:pass
        else:raise RuntimeError('accepted refresh with '+case+' drift')
    print(json.dumps({'result':'PASS','positive':1,'negative':len(mutants),'rejected':mutants,'scope':'exact native ELF-only immutable trial identity guard; no binary execution'}))


if __name__=='__main__':main()
