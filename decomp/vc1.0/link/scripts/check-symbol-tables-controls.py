#!/usr/bin/env python3
"""Focused host-only corruption tests of frozen symbol-table observations."""
import copy,json,time
from pathlib import Path
from symbol_tables_abi import compare,fp
CASE=Path(__file__).resolve().parents[1]/'cases/direct-symbol-tables'
def main():
    start=time.perf_counter();d=CASE/'acceptance/copy';left=[json.loads(x) for x in (d/'original.jsonl').read_text().splitlines()];right=json.loads((d/'native.json').read_text());compare(left,right);results=[]
    # Approved C0 is the sole relaxed bit; all other architectural status stays strict.
    a=copy.deepcopy(left[0]);b=copy.deepcopy(right['events'][0]);b['fp_status']^=0x100;fp(a,b);results.append(dict(control='only-C0-change',status='accepted-qualified-dead-bit'))
    for bit in [1,2,4,8,16,32,64,128,512,1024,2048,4096,8192,16384,32768]:
        a=copy.deepcopy(left[0]);b=copy.deepcopy(right['events'][0]);b['fp_status']^=bit
        try:fp(a,b)
        except AssertionError:results.append(dict(control='other-FP-bit-'+hex(bit),status='rejected'))
        else:raise AssertionError('FP bit unexpectedly qualified')
    def entry(r):return r['events'][0]
    mutations=[('missing-formal',lambda l,r:entry(r)['arguments'].clear()),('wrong-table-formal',lambda l,r:entry(r)['arguments'].__setitem__(0,0)),('bad-stack',lambda l,r:r['events'][1].__setitem__('esp',0)),('nonvolatile-clobber',lambda l,r:r['events'][1].__setitem__('ebx',0)),('wrong-errors',lambda l,r:entry(r)['errors'].__setitem__(0,1)),('wrong-shortname',lambda l,r:entry(r).__setitem__('short_name','00'*9)),('input-mutated',lambda l,r:entry(r).__setitem__('inputs_unchanged',False)),('wrong-key-owner',lambda l,r:entry(r)['tables'][0].__setitem__('key_owner',False)),('wrong-CW',lambda l,r:entry(r).__setitem__('fp_control',0x37f)),('wrong-tag',lambda l,r:entry(r).__setitem__('fp_tag',0)),('missing-observation',lambda l,r:r['events'].pop()),('both-empty',lambda l,r:(l.clear(),r['events'].clear()))]
    for name,mutate in mutations:
        l=copy.deepcopy(left);r=copy.deepcopy(right);mutate(l,r)
        try:compare(l,r)
        except (AssertionError,KeyError,IndexError):results.append(dict(control=name,status='rejected'))
        else:raise AssertionError('bad observation accepted: '+name)
    print(json.dumps(dict(status='pass',checks=results,seconds=time.perf_counter()-start,scope='Frozen evidence only, no runtime/debugger/original. Qualified exactly deadC0;15otherbits/actual formal/ABI/live-state denials.'),indent=2))
if __name__=='__main__':main()
