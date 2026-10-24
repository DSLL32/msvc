#!/usr/bin/env python3
"""Affected V2 case/marker/dispatch guards; reuse identical reviewed primitives."""
import hashlib,json,time
from pathlib import Path
from original import mre_analysis_mode
from accept import reconstruction_capture
p=Path(__file__).resolve().parents[1];old=Path('/root/.ai-tmp/msvc2/vc5.10/cl/mre-prefix-analysis-1');passed=[];started=time.perf_counter()
source=(p/'scripts/mre-observer.inc').read_text();historical=(old/'mre-observer.inc').read_text()
extract=lambda s:s[s.index('static int mre_observation_mode('):s.index('static void mre_observation_install(')]
assert extract(source)==extract(historical)
assert mre_analysis_mode(True,'mre-prefix',False,False,False,False,False,False,True)=='CL_MRE_PREFIX_ANALYSIS_V2\n'
for index,value,label in [(0,False,'masked'),(1,'mre-dry-run','old case'),(2,True,'other dependency analysis'),(5,True,'LINK response'),(6,True,'clock analysis'),(7,True,'clock response')]:
 args=[True,'mre-prefix',False,False,False,False,False,False,True];args[index]=value
 try:mre_analysis_mode(*args)
 except AssertionError:passed.append(label)
 else:raise AssertionError('accepted '+label)
for baseline in(False,True):
 try:reconstruction_capture({'mre_analysis':True,'baseline':baseline})
 except AssertionError:passed.append('never acceptance '+str(baseline))
 else:raise AssertionError('accepted analysis')
assert 'CL_MRE_PREFIX_ANALYSIS_V2\\n'in source and 'if (i==entry_count) ExitProcess(175);'in source
helper=(p/'scripts/reference-debugger.c').read_text()
assert 'mre_analysis_observer && guard->rva==0x550d'in helper
assert 'analysis-stop kind=unexpected-dispatch'in helper
prior=Path('/root/.ai-tmp/msvc2/vc5.10/cl/mre-dry-run-plan/mode-controls-v3.json')
print(json.dumps({'passed':True,'fresh_negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed,
 'reused':{'path':str(prior),'sha256':hashlib.sha256(prior.read_bytes()).hexdigest(),'count':154,'premise':'Identical C mode/call primitives, unchanged byte/range/cleanup/order guards; V2 case/marker and mandatory dispatch stop reviewed separately.'},'primitive_sha256':hashlib.sha256(extract(source).encode()).hexdigest()}))
