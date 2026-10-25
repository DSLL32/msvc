#!/usr/bin/env python3
"""Reached CF environment verification; no original or debugger process."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'watcom'))
from frontend import verify_overlay,validate_launch_command
from ipc import ProtocolError

def denied(block,overlay):
    try:verify_overlay(block,overlay)
    except (ProtocolError,UnicodeError):return
    raise AssertionError('invalid reached target overlay accepted')
block=b'=R:=R:\\repo\0Path=\0Keep=one\0Case=two\0\0'
assert verify_overlay(block,{'PATH':'','gone':None,'case':'two'})['verified']
assert verify_overlay(b'\0\0',{'removed':None})['actual_entries']==0
for overlay in ({'PATH':None},{'keep':'wrong'},{'missing':''},{'case':'Two'}):denied(block,overlay)
for bad in (b'PATH=x\0',b'PATH=x\0\0extra',b'PATH=x\0path=y\0\0',b'bad\0\0',b'X=\xff\0\0'):
    denied(bad,{})
print('CF reached target environment: set/empty/removal/case/hidden/malformed PASS')

valid=['/usr/bin/wine']+['argument']*14
assert validate_launch_command(valid)==valid
for bad in ([{'Unix':[47,117,115,114]}]+valid[1:],valid[:-1],['wine']+valid[1:],valid[:-1]+['bad\0path']):
    try:validate_launch_command(bad)
    except ProtocolError:continue
    raise AssertionError('invalid subprocess shape accepted')
print('CF Rust/Python subprocess shape: path strings/absolute/NUL/width PASS')
