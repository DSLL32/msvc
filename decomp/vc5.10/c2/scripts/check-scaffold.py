#!/usr/bin/env python3
"""Fail-closed masking and evidence mutation checks; no original launches."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import tempfile
from compare import compare
from state import load, masked, offset, sha, validate


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--original',type=Path,required=True)
    p.add_argument('--native',type=Path,required=True)
    args=p.parse_args()
    config,original,inventory=load()
    assert not config['accepted'] and not config['driver'] and not config['support'],'initial scaffold check only'
    image=masked(config,original,inventory)
    for section in inventory['sections']:
        start=section['raw_offset'];end=start+section['raw_size']
        assert image[start:end]==(b'\xcc'*(end-start)if int(section['characteristics'],16)&0x20 else original[start:end])
    assert compare(args.original,args.native)['matched']=='_c2_crt_startup'
    passed=[]
    def reject(name,callback):
        try:callback()
        except (AssertionError,KeyError,ValueError):passed.append(name)
        else:raise AssertionError('negative accepted: '+name)
    broken=copy.deepcopy(config);broken['accepted']=['unknown']
    reject('unknown acceptance',lambda:masked(broken,original,inventory))
    reject('unknown candidate',lambda:masked(config,original,inventory,['unknown']))
    broken=copy.deepcopy(config);broken['case']['environment']['MSC_CMD_FLAGS']=''
    reject('absent environment changed to empty',lambda:validate(broken,original,inventory))
    broken=copy.deepcopy(config);broken['driver']=[dict(name='bad',start=0x452797,end=0x452798)]
    reject('unproved ownership',lambda:validate(broken,original,inventory))
    broken=copy.deepcopy(config);broken['driver']=[dict(name='a',start=0x452797,end=0x452799,evidence='test'),dict(name='b',start=0x452798,end=0x45279a,evidence='test')]
    reject('overlapping ownership',lambda:validate(broken,original,inventory))
    broken=copy.deepcopy(config);broken['driver']=[dict(name='bad',start=0x499728,end=0x49972c,evidence='test')]
    reject('data section claimed as code',lambda:validate(broken,original,inventory))
    changes=[
        ('stale original configuration','o','configuration.json',lambda b:b+b' '),
        ('stale native configuration','n','configuration.json',lambda b:b+b' '),
        ('provider file changed','o','loaded-msvcrt.bin',lambda b:bytes([b[0]^1])+b[1:]),
        ('masked instruction changed','o','C2.EXE',lambda b:b[:offset(inventory,0x452797)]+b'\x90'+b[offset(inventory,0x452797)+1:]),
        ('original event changed','o','events.txt',lambda b:b+b'exit 00000000\n'),
        ('native state changed','n','state.json',lambda b:b+b' '),
        ('partial stream changed','n','stderr.bin',lambda b:b+b'x'),
    ]
    with tempfile.TemporaryDirectory(prefix='scaffold-negatives-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as scratch:
        root=Path(scratch)
        for index,(name,side,filename,change)in enumerate(changes):
            work=root/str(index);shutil.copytree(args.original,work/'o');shutil.copytree(args.native,work/'n')
            target=work/side/filename;target.write_bytes(change(target.read_bytes()))
            reject(name,lambda:compare(work/'o',work/'n'))
        work=root/'logical';shutil.copytree(args.original,work/'o');shutil.copytree(args.native,work/'n')
        path=work/'n/state.json';state=json.loads(path.read_text());state['function']='unrelated';path.write_text(json.dumps(state))
        path=work/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((work/'n/state.json').read_bytes());path.write_text(json.dumps(receipt))
        reject('different logical frontier with internally matching hash',lambda:compare(work/'o',work/'n'))
    print(json.dumps({'initial_mask_bytes':sum(s['raw_size']for s in inventory['sections']if int(s['characteristics'],16)&0x20),'matched_frontier':'_c2_crt_startup','negative_checks':passed,'count':len(passed),'accepted_functions':0},indent=2))

if __name__=='__main__':main()
