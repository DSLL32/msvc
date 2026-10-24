#!/usr/bin/env python3
"""Focused offline qualification/actual-input corruption controls. No execution."""
import copy,json,time
from pathlib import Path
from module_diagnostics_abi import compare
CASE=Path(__file__).resolve().parents[1]/'cases/direct-module-diagnostics'
def main():
    start=time.perf_counter();root=CASE/'analysis/state-qualification';left=[json.loads(x) for x in (root/'original/events.jsonl').read_text().splitlines()];right=json.load(open(root/'native/state.json'));compare(left,right,qualified=True);checks=[]
    def entry(r):return next(e for e in r['events'] if e.get('phase')==2 and e['event']=='entry')
    mutations=[('native-Last-other-value',lambda l,r:entry(r)['errors'].__setitem__(2,7)),('original-Last-other-value',lambda l,r:next(e for e in l if e.get('phase')==2)['errors'].__setitem__(2,7)),('errno',lambda l,r:entry(r)['errors'].__setitem__(0,1)),('DOS',lambda l,r:entry(r)['errors'].__setitem__(1,1)),('wrong-control',lambda l,r:entry(r).__setitem__('fp_control',0x37f)),('wrong-tag',lambda l,r:entry(r).__setitem__('fp_tag',0)),('missing-formal',lambda l,r:entry(r)['arguments'].clear()),('wrong-text-formal',lambda l,r:entry(r)['arguments'].__setitem__(0,0)),('missing-global-owner',lambda l,r:entry(r)['modules']['rows'][0].pop('owner_edges')),('missing-watch',lambda l,r:r['events'].__setitem__(slice(None),[e for e in r['events'] if e['event']!='last_error_watch_armed'])),('writeonly-watch',lambda l,r:next(e for e in l if e['event']=='last_error_watch_armed').__setitem__('access','write')),('narrow-watch',lambda l,r:next(e for e in l if e['event']=='last_error_watch_armed').__setitem__('width',1)),('native-read-event',lambda l,r:r['events'].append(dict(event='last_error_access'))),('original-read-event',lambda l,r:l.insert(-1,dict(event='last_error_access'))),('new-thread',lambda l,r:l.insert(-1,dict(event='new_thread'))),('failed-controller',lambda l,r:l[-1].__setitem__('failed',8)),('both-empty',lambda l,r:(l.clear(),r['events'].clear()))]
    def late_watch(l,r):
        e=next(e for e in l if e['event']=='last_error_watch_armed');l.remove(e);l.insert(-2,e)
    mutations.append(('late-watch',late_watch))
    for bit in [1,2,4,8,16,32,64,128,512,1024,2048,4096,8192,16384,32768]:
        mutations.append(('other-FP-bit-'+hex(bit),lambda l,r,b=bit:entry(r).__setitem__('fp_status',entry(r)['fp_status']^b)))
    for name,mutate in mutations:
        l=copy.deepcopy(left);r=copy.deepcopy(right);mutate(l,r)
        try:compare(l,r,qualified=True)
        except (AssertionError,KeyError,IndexError,StopIteration):checks.append(dict(control=name,status='rejected'))
        else:raise AssertionError('bad evidence accepted: '+name)
    r=copy.deepcopy(right)
    for e in r['events']:
        if 'fp_status' in e and not (e.get('phase')==0 and e['event']=='entry'):e['fp_status']^=0x100
    compare(copy.deepcopy(left),r,qualified=True);checks.append(dict(control='only-deadC0-change',status='accepted-bounded-qualification'))
    try:compare(copy.deepcopy(left),copy.deepcopy(right))
    except AssertionError:checks.append(dict(control='unqualified-raw-mismatch',status='held'))
    else:raise AssertionError('unqualified raw mismatch hidden')
    print(json.dumps(dict(status='pass',checks=checks,seconds=time.perf_counter()-start,scope='Frozenobservations only; no program/debugger/original/Wine run. Onlypost-mainambientLast1224/0 anddeadC0 permitted; strict consumedstate and guardinterval.'),indent=2))
if __name__=='__main__':main()
