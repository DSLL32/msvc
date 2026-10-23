#!/usr/bin/env python3
"""Exercise the actual LINK packet decoder on independent mutated raw copies."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile
import time
from link_response import decode

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);args=parser.parse_args()
started=time.perf_counter();expected=decode(args.original);passed=[]
with tempfile.TemporaryDirectory(prefix='link-response-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary)/'raw';shutil.copytree(args.original,directory)
    base_capture=(directory/'capture.json').read_bytes()
    def mutation(label,edits,capture_edit=None):
        capture=json.loads(base_capture);saved={}
        for name,change in edits.items():
            path=directory/name;saved[name]=path.read_bytes();raw=change(saved[name]);path.write_bytes(raw)
            capture['files'][name]={'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
        if capture_edit:capture_edit(capture)
        (directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
        try:
            try:decode(directory)
            except (AssertionError,KeyError,ValueError,struct.error):passed.append(label)
            else:raise AssertionError('accepted altered LINK evidence: '+label)
        finally:
            for name,raw in saved.items():(directory/name).write_bytes(raw)
            (directory/'capture.json').write_bytes(base_capture)
    def xor(offset):
        def change(raw):
            result=bytearray(raw);result[offset]^=1;return bytes(result)
        return change
    def replace(old,new):
        def change(raw):assert old in raw;return raw.replace(old,new,1)
        return change
    for name,key,value in [('unmasked','baseline',True),('wrong family','case','minimal-c'),('missing mode','link_response',False),('failed controller','controller_error','failure')]:
        mutation(name,{},lambda c,key=key,value=value:c.update({key:value}))
    mutation('wrong marker',{'dependency-observer.txt':replace(b'LINK',b'C1XX')})
    for label,offset in [('spawn mode',0),('spawn path pointer',4),('spawn argv pointer',8)]:mutation(label,{'observer/spawn-before-stack.bin':xor(offset)})
    mutation('changed caller argv',{'observer/spawn-after-caller-argv.bin':xor(16)})
    mutation('child status',{'observer/spawn-after-context.bin':xor(176)})
    mutation('x87 control word',{'observer/spawn-after-context.bin':xor(28)})
    mutation('CRT errno',{'mktemp-spawn-before-ptd.bin':xor(8)})
    mutation('CRT DOS error',{'mktemp-spawn-after-ptd.bin':xor(12)})
    mutation('Win32 LastError',{'mktemp-spawn-before-teb.bin':xor(0x34)})
    mutation('code page',{'observer/spawn-after-crt-words.bin':xor(24)})
    mutation('ctype table',{'observer/spawn-after-ctype.bin':xor(128)})
    mutation('CRT environment',{'observer/spawn-after-crt-environment.bin':xor(16)})
    mutation('OS environment',{'observer/spawn-after-os-environment.bin':xor(0)})
    mutation('FILE state',{'observer/spawn-after-static-iob.bin':xor(16)})
    mutation('Win32 standard handle',{'observer/spawn-after-parameters.bin':xor(0x1c)})
    mutation('live argv pointer identity',{'observer/spawn-after-crt-words.bin':xor(80)})
    mutation('pass environment',{'observer/spawn-before-caller-pass.bin':xor(8)})
    mutation('response bytes',{'observer/link-input/'+Path(expected['child']['argv'][2][3:]).name:xor(0)})
    mutation('object bytes',{'observer/link-input/test.obj':xor(0)})
    mutation('output bytes',{'observer/link-output/test.exe':xor(64)})
    mutation('executable identity',{'observer/spawn-executable.bin':xor(64)})
    mutation('adjacent CRT provider input',{'MSVCRT.DLL':xor(64)})
    mutation('adjacent PDB provider input',{'MSPDB50.DLL':xor(64)})
    mutation('unlink filename',{'observer/unlink-1-before-filename.bin':xor(2)})
    mutation('unlink result',{'observer/events.txt':replace(b'unlink-return sequence=1 result=00000000',b'unlink-return sequence=1 result=00000001')})
    mutation('child family marker',{'observer/events.txt':replace(b'observed-child family=link',b'observed-child family=c2xx')})
    mutation('full exit status',{'events.txt':replace(b'status=0\n',b'status=2\n')})
    mutation('unexpected stderr',{'stderr.bin':lambda raw:raw+b'X'})
    mutation('unknown extra invocation',{'observer/events.txt':lambda raw:raw+b'observed-child family=link result=00000000 putenv_calls=0\n'})
assert decode(args.original)==expected
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed}))
