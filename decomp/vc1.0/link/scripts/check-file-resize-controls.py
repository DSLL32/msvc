#!/usr/bin/env python3
"""Host-only denials of corrupted actual file-resize evidence; no execution."""
import copy,json,time
from pathlib import Path
from file_resize_abi import compare,load
CASE=Path(__file__).resolve().parents[1]/'cases/direct-file-resize'
def main():
    start=time.perf_counter();results=[]
    for phase in ['mapped','buffered']:
        base=CASE/'acceptance'/phase;left,right=load(base/'original/events.jsonl',base/'native/state.json');compare(copy.deepcopy(left),copy.deepcopy(right),phase)
        def app(r):return next(e for e in r['events'] if e['event']=='entry')
        def api(r):return next(e for e in r['events'] if e['event']=='api_entry')
        mutations=[('missing-formal',lambda l,r:app(r)['formal_words'].pop()),('wrong-size',lambda l,r:app(r)['formal_words'].__setitem__(1,1)),('over-wide-slot',lambda l,r:app(r)['slot'].append(0)),('wrong-live-error',lambda l,r:api(r)['errors'].__setitem__(2,1)),('wrong-public-owner',lambda l,r:api(r).__setitem__('actual_caller','unrelated')),('wrong-precision',lambda l,r:api(r).__setitem__('fp_control',0x37f)),('missing-public-boundary',lambda l,r:r['events'].remove(api(r))),('wrong-return',lambda l,r:next(e for e in r['events'] if e['event']=='return').__setitem__('eax',17)),('bad-stack-cleanup',lambda l,r:next(e for e in r['events'] if e['event']=='return').__setitem__('esp',0)),('clobbered-nonvolatile',lambda l,r:next(e for e in r['events'] if e['event']=='return').__setitem__('ebx',0)),('both-empty',lambda l,r:(l.clear(),r['events'].clear()))]
        if phase=='buffered':mutations.append(('wrong-page-owner',lambda l,r:app(r)['pages'][0]['head'].__setitem__(0,999)))
        for name,mutate in mutations:
            l=copy.deepcopy(left);r=copy.deepcopy(right);mutate(l,r)
            try:compare(l,r,phase)
            except (AssertionError,KeyError,IndexError):results.append(dict(phase=phase,control=name,status='rejected'))
            else:raise AssertionError('invalid evidence accepted: '+phase+'/'+name)
    print(json.dumps(dict(status='pass',controls=results,seconds=time.perf_counter()-start,scope='Copied frozen observations only. No native/original/debugger/Wine execution.'),indent=2))
if __name__=='__main__':main()
