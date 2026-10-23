#!/usr/bin/env python3
"""Promote exactly one reviewed function from an immutable, current successful trial."""
import argparse
import importlib.util
import json
from pathlib import Path
from common import HERE,read,sha

spec=importlib.util.spec_from_file_location('rc_verify',HERE/'scripts/verify-pair.py')
v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('pair',type=Path);a=p.parse_args()
    review=read(HERE/'cases/help/review.json')
    assert review.get('first_promotion_approved') is True,'First-promotion review is still pending'
    assert review.get('review_id'),'Review identity missing'
    path=a.pair/'candidate.json';trial=read(path);old=read(HERE/'functions.json')
    extra=set(trial['accepted'])-set(old['accepted'])
    assert len(extra)==1 and set(old['accepted'])<set(trial['accepted']),'Trial must add exactly one function'
    expected=dict(old,accepted=trial['accepted'])
    assert expected==trial,'Trial changes more than the one accepted function'
    result=v.validate(a.pair,path)
    record={'accepted':next(iter(extra)),'review_id':review['review_id'],'pair':str(a.pair.resolve()),
            'prefix_sha256':sha(HERE/'functions.json'),'candidate_sha256':sha(path),
            'audit_sha256':sha(a.pair/'audit.json'),'comparison':result}
    (a.pair/'acceptance.json').write_text(json.dumps(record,indent=2)+'\n')
    (HERE/'functions.json').write_bytes(path.read_bytes())
    print(json.dumps(record,indent=2))
