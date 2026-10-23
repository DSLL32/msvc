#!/usr/bin/env python3
"""Affected bounded LoadStringA controls for a newly observed contextual ID."""
import argparse
from pathlib import Path
from resources import verify_api_capture

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('capture',type=Path);parser.add_argument('identifier',type=int)
args=parser.parse_args();positive=verify_api_capture(args.capture)
record=next(x for x in positive['calls']if x['call']['id']==args.identifier);sequence=record['call']['sequence'];passed=[]
def negative(name,path,change):
    old=path.read_bytes()
    try:
        path.write_bytes(change(old))
        try:verify_api_capture(args.capture)
        except (AssertionError,KeyError):passed.append(name)
        else:raise AssertionError('resource mutation accepted: '+name)
    finally:path.write_bytes(old)
path=args.capture/('resource-%04d-after.bin'%sequence)
negative('observed payload byte',path,lambda b:bytes([b[0]^1])+b[1:])
negative('tail after terminatingNUL',path,lambda b:b[:-1]+bytes([b[-1]^1]))
events=args.capture/'resource-events.txt'
negative('wrong bounded output capacity',events,lambda b:b.replace(('sequence=%d id=%d module=00400000 output=00409698 capacity=256'%(sequence,args.identifier)).encode(),('sequence=%d id=%d module=00400000 output=00409698 capacity=255'%(sequence,args.identifier)).encode()))
negative('wrong return length',events,lambda b:b.replace(('return sequence=%d id=%d value='%(sequence,args.identifier)).encode(),('return sequence=%d id=%d value=1'%(sequence,args.identifier)).encode()))
assert verify_api_capture(args.capture)==positive
print('Observed contextual resource and %d payload/bounds/tail/return controls pass'%len(passed))
