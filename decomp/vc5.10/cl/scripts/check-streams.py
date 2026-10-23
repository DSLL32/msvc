#!/usr/bin/env python3
"""Compare a real stopped stream boundary and exercise semantic mutations."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import tempfile
import streams_state

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',type=Path,required=True)
parser.add_argument('--native',type=Path,required=True)
args=parser.parse_args()
expected={phase:streams_state.original(args.original,phase)for phase in ('entry','frontier')}
assert expected=={phase:streams_state.native(args.native,phase)for phase in expected}
assert expected['frontier']['crt'][2]['role']=='stdout'
assert expected['frontier']['win32'][2]['role']=='stdout'
passed=[]
with tempfile.TemporaryDirectory(prefix='stream-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as name:
    work=Path(name)
    def control(label,change):
        for path in work.iterdir():path.unlink()
        for path in args.native.glob('streams-*'):shutil.copyfile(path,work/path.name)
        shutil.copyfile(args.native/'native-process.json',work/'native-process.json')
        change(work)
        try:actual={phase:streams_state.native(work,phase)for phase in expected}
        except (AssertionError,KeyError,ValueError):passed.append(label);return
        assert actual!=expected,'accepted changed stream state: '+label
        passed.append(label)
    def record(directory,change):
        path=directory/'streams-frontier.json';value=json.loads(path.read_text())
        change(value);path.write_text(json.dumps(value))
    def word(directory,offset,value):
        path=directory/'streams-frontier-iob.bin';raw=bytearray(path.read_bytes())
        struct.pack_into('<I',raw,offset,value);path.write_bytes(raw)
    control('separate Win32 error destination',lambda d:record(d,lambda r:r['win32'][2].update(role='stderr')))
    control('changed CRT output position',lambda d:record(d,lambda r:r['crt'][2].update(position=1)))
    control('CRT/Win32 slot alias lost',lambda d:record(d,lambda r:r['crt'][2].update(handle=r['crt'][2]['handle']+1)))
    control('changed lowio flags',lambda d:record(d,lambda r:r['osfile'].__setitem__(2,0)))
    control('changed LastError',lambda d:record(d,lambda r:r['errors'].update(last_error=0)))
    control('different process',lambda d:record(d,lambda r:r.update(pid=r['pid']+1)))
    control('stdin ownership flags changed',lambda d:word(d,12,1))
    control('stdin capacity changed',lambda d:word(d,24,8192))
    control('stdin pointer/base alias lost',lambda d:word(d,0,1))
    control('unobserved live stdin bytes',lambda d:word(d,4,1))
    control('FILE error descriptor changed',lambda d:word(d,64+16,1))
print('Actual original/native stream boundary matches; %d semantic mutation controls pass'%len(passed))
