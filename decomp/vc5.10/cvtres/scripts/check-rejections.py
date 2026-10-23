#!/usr/bin/env python3
"""Initial real-pair and fail-closed adversarial evidence checks; native-only."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import tempfile
from mask import BINARY,load,sha
from evidence import bind,compare_frontier,native_raw,original_raw
from audit import check_source
from accept import bind_audit

p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--audit',required=True,type=Path);a=p.parse_args()
config,_=load();old=a.original.resolve();new=a.native.resolve();passed=[]
assert not config['accepted'] and all(f['status']=='stub' for f in config['functions'])
bind(config,old,new);match=compare_frontier(config,old,new,initial=True)

def reject(name,fn):
    try:fn()
    except (AssertionError,KeyError,ValueError,TypeError):passed.append(name)
    else:raise AssertionError('negative accepted: '+name)

with tempfile.TemporaryDirectory(prefix='negative-',dir=str(Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres')) as directory:
    root=Path(directory)
    def pair(name):
        one=root/name/'original';two=root/name/'native'
        shutil.copytree(old,one);shutil.copytree(new,two);return one,two
    o,n=pair('raw-missing');(o/'events.txt').write_text('')
    reject('missing original raw frontier',lambda:compare_frontier(config,o,n,True))
    o,n=pair('raw-wrong');s=(o/'events.txt').read_text().replace('trap 01003180 eip 01003181','trap 010014a5 eip 010014a6');(o/'events.txt').write_text(s)
    reject('wrong raw original frontier',lambda:compare_frontier(config,o,n,True))
    o,n=pair('raw-none');state=json.loads((n/'state.json').read_text());state['function']=None;(n/'state.json').write_text(json.dumps(state))
    reject('equal None is not a frontier',lambda:compare_frontier(config,o,n,True))
    o,n=pair('false-exit');(n/'state.json').write_text(json.dumps({'events':[],'exit_status':None}));(n/'gdb.stdout').write_text('copied receipt says complete')
    reject('missing native exit is not completion',lambda:native_raw(n))
    o,n=pair('stale');snapshot=json.loads((o/'configuration.json').read_text());snapshot['accepted']=['_main'];data=json.dumps(snapshot).encode()
    for d in [o,n]:
        (d/'configuration.json').write_bytes(data);receipt=json.loads((d/'receipt.json').read_text());receipt['configuration_sha256']=sha(data);(d/'receipt.json').write_text(json.dumps(receipt))
    reject('mutually matching stale snapshots',lambda:bind(config,o,n))
    o,n=pair('changed-raw');(o/'events.txt').write_text((o/'events.txt').read_text()+'changed\n')
    reject('copied receipt cannot hide changed raw capture',lambda:bind(config,o,n))
    o,n=pair('wrong-provider');(o/'loaded-msvcrt.dll').write_bytes(b'wrong provider')
    reject('actual loaded provider file identity',lambda:bind(config,o,n))
    o,n=pair('wrong-mask');data=bytearray((o/'CVTRES.EXE').read_bytes());data[0x14a5-0x1000+0x600]=0x90
    (o/'CVTRES.EXE').write_bytes(data);(o/'loaded-image.exe').write_bytes(data)
    receipt=json.loads((o/'receipt.json').read_text());receipt['masked_sha256']=receipt['loaded_image_hfile_sha256']=sha(data);(o/'receipt.json').write_text(json.dumps(receipt))
    reject('raw mask must equal exact current trial',lambda:bind(config,o,n))
    wrong=copy.deepcopy(config);wrong['functions'][1]['start']=wrong['functions'][0]['start']
    reject('overlapping application ownership',lambda:check_source(wrong))
    wrong=copy.deepcopy(config);wrong['functions'][1]['cleanup']=8
    reject('wrong declared ABI cleanup',lambda:check_source(wrong))
    wrong=copy.deepcopy(config);wrong['functions'][0]['status']='candidate'
    reject('trapping source cannot be candidate implementation',lambda:check_source(wrong))
    report=json.loads((old/'receipt.json').read_text())
    wrong=copy.deepcopy(config);wrong['functions'][0]['status']='candidate'
    reject('prefix audit is not current trial',lambda:bind_audit(wrong,report,a.audit))
    audit_copy=root/'copied-prefix-audit';shutil.copytree(a.audit,audit_copy)
    receipt=json.loads((audit_copy/'receipt.json').read_text());receipt['trial_sha256']=sha(json.dumps(wrong,sort_keys=True).encode());(audit_copy/'receipt.json').write_text(json.dumps(receipt))
    reject('copied trial hash cannot hide omitted candidate graph',lambda:bind_audit(wrong,report,audit_copy))

receipt=dict(schema=1,initial_real_pair=match,rejected=passed,count=len(passed),wine_invoked=False)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
