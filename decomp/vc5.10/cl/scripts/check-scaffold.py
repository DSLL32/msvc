#!/usr/bin/env python3
"""Native-only initial trap and fail-closed mask/acceptance checks."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import os
from accept import validate
from binary import PE
from mask import construct

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
config=json.loads((PROJECT/'functions.json').read_text())
# Always verify the zero-acceptance construction independently of progress.
config=copy.deepcopy(config);config['accepted']=[];config['acceptance_reviewed']=False
pe=PE(ROOT/config['target']['path']);masked=construct(config)
for section in pe.sections:
    a=section['off'];b=a+section['size']
    assert masked[a:b]==(b'\xcc'*(b-a) if section['flags']&0x20 else pe.data[a:b])
assert masked[pe.offset(pe.base+pe.entry)]==0xcc
for mutation in ('unknown-accepted','missing-evidence','outside-code','overlap'):
    bad=copy.deepcopy(config)
    if mutation=='unknown-accepted':bad['accepted']=['unknown']
    else:
        bad['driver'][0]['evidence']={'analysis':'test-only'};bad['accepted']=['_main']
        if mutation=='missing-evidence':bad['driver'][0].pop('evidence')
        elif mutation=='outside-code':bad['driver'][0]['end']=0x500000
        else:bad['driver'][0]['text_data']=[[bad['driver'][0]['start'],bad['driver'][0]['start']+1]]
    try:construct(bad)
    except (AssertionError,ValueError):pass
    else:raise AssertionError('mask accepted mutation '+mutation)
try:validate(config,{},'_main')
except AssertionError as error:assert 'review' in str(error)
else:raise AssertionError('acceptance bypassed review')
result=subprocess.run(['gdb','-q','-nx','-batch','-ex','set pagination off','-ex','tbreak *main','-ex','run /?',
    '-ex','info symbol $pc','--args',str(PROJECT/'build/native/CL.EXE')],capture_output=True,text=True,timeout=20,
    env={**os.environ,'CL_ENTRY_PROFILE':'help-canonical-msvcrt-v1'})
names=['main']+[r.get('native_symbol','') for r in config['driver']]
assert 'Temporary breakpoint 1, main' in result.stdout,result.stdout+result.stderr
print('zero acceptance: executable sections entirely INT3; data unchanged; five negative checks and native target main breakpoint pass')
