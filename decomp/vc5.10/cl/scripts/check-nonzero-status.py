#!/usr/bin/env python3
"""Focused explicit nonzero completion/status protocol controls; no acceptance."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import struct
import tempfile
import case_inputs
from state import basic_project

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    assert basic_project(platform,directory)['fields']['status']==2
    with tempfile.TemporaryDirectory(prefix='nonzero-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
        work=Path(temporary)
        names=(['events.txt','capture.json']if platform=='original'else
            ['debugger.txt','entry-profile.json','terminal-status.bin','status_reader.py','native-process.json','native-build.json',
             'native_status.py','gdb_capture.py','gdb-capture.py'])
        for name in names:shutil.copy2(directory/name,work/name)
        def negative(label,change,restore):
            try:
                change()
                try:basic_project(platform,work)
                except (AssertionError,KeyError,ValueError):passed.append(platform+': '+label)
                else:raise AssertionError('nonzero mutation ignored: '+label)
            finally:restore()
        original_load=case_inputs.load
        def expectation(value):
            def load(name):
                result=copy.deepcopy(original_load(name));result['expected_status']=value;return result
            case_inputs.load=load
        negative('wrong declared expectation',lambda:expectation(0),lambda:setattr(case_inputs,'load',original_load))
        terminal=work/('events.txt'if platform=='original'else'debugger.txt');old=terminal.read_bytes()
        marker=(b'exit event=999 status=2\n'if platform=='original'else b'NATIVE_COMPLETION_STATUS=2\n')
        negative('duplicate completion marker',lambda:terminal.write_bytes(old+marker),lambda:terminal.write_bytes(old))
        if platform=='native':
            path=work/'terminal-status.bin';raw=path.read_bytes();changed=bytearray(raw);struct.pack_into('<I',changed,16,258)
            assert 258&255==2
            negative('fullDWORD258 versus2 with identicalPOSIXlow8',lambda:path.write_bytes(changed),lambda:path.write_bytes(raw))
        assert basic_project(platform,work)['fields']['status']==2
for value in (True,False,-1,0x100000000,0x7001,0x7003,'2',None):
    try:case_inputs.target_status(value)
    except AssertionError:passed.append('invalid/reserved expectation '+repr(value))
    else:raise AssertionError('invalid expectation accepted')
assert case_inputs.target_status(2)==2 and case_inputs.target_status(258)==258
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
