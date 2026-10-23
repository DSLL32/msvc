#!/usr/bin/env python3
"""Focused forbidden-delta controls for the reviewed pending-only derivation."""
import argparse
import copy
import json
import tempfile
import shutil
import time
from pathlib import Path
from pending_metadata import delta,certificate
from state import BINARY, load, sha


def main():
    start=time.monotonic()
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path)
    p.add_argument('--correction',required=True,type=Path);a=p.parse_args()
    _,image,inventory=load();current=(BINARY/'functions.json').read_bytes();old=(a.original/'base_configuration.json').read_bytes()
    claim=json.loads(a.correction.read_text());config=json.loads(current)
    control=certificate(a.original,a.native,current,a.correction.resolve(),image,inventory)
    rejected=[]
    def rejected_delta(name,change):
        c=copy.deepcopy(config);proof=copy.deepcopy(claim);change(c)
        data=(json.dumps(c,indent=2)+'\n').encode()
        # Rebind the base digest and textual delta so the structural allowlist,
        # exact interval proof and ownership checks themselves are exercised.
        proof['current_base_sha256']=sha(data)
        orig=json.loads(old)
        proof['changes']=[]
        for x,y in zip(orig['stubs'],c['stubs']):
            changed=sorted(k for k in x.keys()|y.keys() if x.get(k)!=y.get(k))
            if changed:proof['changes'].append({'name':x['name'],'old':{k:x.get(k)for k in changed},'new':{k:y.get(k)for k in changed}})
        try:delta(old,data,proof,image,inventory)
        except (AssertionError,KeyError,ValueError):rejected.append(name)
        else:raise AssertionError('forbidden delta accepted: '+name)
    def stub(c):return next(r for r in c['stubs']if r['name']=='@read_initializer_records@4')
    rejected_delta('accepted set changed',lambda c:c['accepted'].append('@read_initializer_records@4'))
    rejected_delta('trial set changed',lambda c:c['trial'].clear())
    rejected_delta('primary entry changed',lambda c:stub(c).__setitem__('start',0x4523d3))
    rejected_delta('primary extent changed',lambda c:stub(c).__setitem__('end',0x452476))
    rejected_delta('fastcall formal ABI changed',lambda c:stub(c).__setitem__('abi','cdecl, FILE* input'))
    rejected_delta('native target changed',lambda c:stub(c).__setitem__('native','unrelated'))
    rejected_delta('frontier state/formals removed',lambda c:stub(c).__setitem__('frontier_contract',''))
    rejected_delta('pending stub reclassified',lambda c:stub(c).__setitem__('kind','runtime'))
    rejected_delta('unknown extent made known',lambda c:stub(c).__setitem__('extent_known_for_tail_abi',False))
    rejected_delta('accepted caller extent changed',lambda c:c['driver'][0].__setitem__('end',c['driver'][0]['end']+1))
    rejected_delta('accepted caller source changed',lambda c:c['driver'][0].__setitem__('source','unrelated.c'))
    rejected_delta('case environment changed',lambda c:c['case']['environment'].__setitem__('MSC_CMD_FLAGS',''))
    rejected_delta('cold prefix shortened',lambda c:stub(c).__setitem__('code_chunks',[[0x4741c7,0x474244]]))
    rejected_delta('cold suffix widened',lambda c:stub(c).__setitem__('code_chunks',[[0x4741c6,0x474245]]))
    rejected_delta('cold envelope covers unrelated gap',lambda c:stub(c).__setitem__('code_chunks',[[0x452475,0x474244]]))
    rejected_delta('chunk overlaps own primary',lambda c:stub(c).__setitem__('code_chunks',[[0x4523d2,0x4523d3]]))
    rejected_delta('chunk overlaps accepted caller',lambda c:stub(c).__setitem__('code_chunks',[[0x4532fa,0x453305]]))
    rejected_delta('pending table extent widened',lambda c:stub(c).__setitem__('shared',[[0x452478,0x4524aa]]))
    rejected_delta('pending table covers RET',lambda c:stub(c).__setitem__('shared',[[0x452474,0x4524a9]]))
    rejected_delta('unrelated stub changed',lambda c:c['stubs'][-1].__setitem__('abi','changed'))
    with tempfile.TemporaryDirectory(prefix='pending-delta-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as scratch:
        def physical(name,side,path):
            root=Path(scratch)/str(len(rejected));shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n')
            dest=root/side/path;dest.write_bytes(dest.read_bytes()+b'changed')
            try:certificate(root/'o',root/'n',current,a.correction.resolve(),image,inventory)
            except (AssertionError,KeyError,ValueError):rejected.append(name)
            else:raise AssertionError('physical change accepted: '+name)
        physical('historical masked bytes changed','o','C2.EXE')
        physical('historical breakpoint plan changed','o','plan.txt')
        physical('historical helper changed','o','capture-helper.exe')
        physical('historical ELF changed','n','native.elf')
        physical('historical rebuilt executable changed','o','rebuilt.exe')
        physical('historical old base relabeled','o','base_configuration.json')
    print(json.dumps({'control':control,'fresh_rejections':len(rejected),'rejected':rejected,'seconds':time.monotonic()-start,'accepted':False},indent=2))


if __name__=='__main__':main()
