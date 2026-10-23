#!/usr/bin/env python3
"""Separate masked response capture from original-only analysis, without Wine."""
import copy
import hashlib
from original import dependency_mode
from accept import reconstruction_capture

config={'accepted':['_execute_pass']}
assert dependency_mode(config,False,'minimal-c',False,False,True)=='CL_DEPENDENCY_RESPONSE_V1\n'
assert dependency_mode(config,True,'minimal-c',True,False,False)=='CL_DEPENDENCY_ANALYSIS_V1\n'
assert dependency_mode(config,True,'minimal-c',True,True,False)=='CL_DEPENDENCY_ANALYSIS_V2_COMPLETE\n'
assert dependency_mode(config,False,'minimal-c',False,False,False)is None
passed=[]
def denied(name,fn):
    try:fn()
    except (AssertionError,KeyError):passed.append(name)
    else:raise AssertionError('accepted invalid response capture mode: '+name)
denied('response from unmasked baseline',lambda:dependency_mode(config,True,'minimal-c',False,False,True))
denied('mixed response and analysis',lambda:dependency_mode(config,False,'minimal-c',True,False,True))
denied('mixed response and complete analysis',lambda:dependency_mode(config,False,'minimal-c',False,True,True))
denied('unapproved response case',lambda:dependency_mode(config,False,'help',False,False,True))
denied('patching unaccepted dispatcher',lambda:dependency_mode({'accepted':[]},False,'minimal-c',False,False,True))
denied('analysis without baseline',lambda:dependency_mode(config,False,'minimal-c',True,False,False))
marker=b'CL_DEPENDENCY_RESPONSE_V1\n'
receipt={'baseline':False,'dependency_analysis':False,'dependency_complete':False,'dependency_response':True,
         'case':'minimal-c','controller_error':None,'helper_status':0,'command':['helper','target','args','cwd','1','1'],
         'files':{'dependency-observer.txt':{'sha256':hashlib.sha256(marker).hexdigest(),'bytes':len(marker)}}}
reconstruction_capture(receipt)
for name,change in (
    ('relabelled analysis',lambda c:c.update(dependency_analysis=True)),
    ('missing actual response request',lambda c:c['files'].clear()),
    ('different actual request',lambda c:c['files']['dependency-observer.txt'].update(sha256='0'*64)),
    ('unmasked helper launch',lambda c:c['command'].__setitem__(-2,'0'))):
    value=copy.deepcopy(receipt);change(value);denied(name,lambda:reconstruction_capture(value))
print('response capture mode positive controls and %d negatives pass; no original run or acceptance'%len(passed))
