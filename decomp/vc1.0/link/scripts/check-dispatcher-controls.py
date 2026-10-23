#!/usr/bin/env python3
"""Host-only negative controls for frozen actual dispatcher ABI evidence."""
import copy,json
from pathlib import Path
from dispatcher_abi import compare
CASE=Path(__file__).resolve().parents[1]/'cases/dispatcher-assistance'
def main():
    results=[]
    for phase in ['helper','direct']:
        root=CASE/'acceptance'/('helper-acceptance' if phase=='helper' else 'direct-acceptance')
        left=[json.loads(x) for x in (root/'original/events.jsonl').read_text().splitlines()]
        right=json.loads((root/'native/state.json').read_text());compare(left,right,phase,True)
        mutations=[('missing-actual-entry',lambda l,r:r['events'].pop(0)),('wrong-callback-owner',lambda l,r:r['events'][0]['run_owners'].__setitem__(0,False)),('wrong-tool-length',lambda l,r:r['events'][0]['tools'][0].__setitem__('length',5)),('changed-full-name-tail',lambda l,r:r['events'][1]['tools'][1].__setitem__('bytes','00'*16)),('missing-pending-byte',lambda l,r:r['events'][1].__setitem__('pending','')),('live-error-change',lambda l,r:r['events'][1]['errors'].__setitem__(0,1)),('changed-ctype',lambda l,r:r['events'][0].__setitem__('ctype','00'*514)),('wrong-return-value',lambda l,r:r['events'][1].__setitem__('eax',2)),('bad-cdecl-cleanup',lambda l,r:r['events'][1].__setitem__('esp',r['events'][1]['esp']+4)),('clobbered-nonvolatile',lambda l,r:r['events'][1].__setitem__('ebx',r['events'][1]['ebx']^1)),('wrong-actual-caller',lambda l,r:r['events'][0].__setitem__('actual_caller','invented_caller'))]
        if phase=='helper':mutations.append(('missing-formal',lambda l,r:r['events'][0]['formal_words'].pop()))
        else:mutations.append(('invented-formal',lambda l,r:r['events'][0]['formal_words'].append(1)))
        mutations.append(('both-sides-empty',lambda l,r:(l.clear(),r['events'].clear())))
        for name,mutate in mutations:
            l=copy.deepcopy(left);r=copy.deepcopy(right);mutate(l,r)
            try:compare(l,r,phase,True)
            except (AssertionError,KeyError,IndexError):results.append(dict(phase=phase,control=name,status='rejected'))
            else:raise AssertionError('Invalid evidence accepted: '+phase+'/'+name)
    print(json.dumps(dict(status='pass',controls=results),indent=2))
if __name__=='__main__':main()
