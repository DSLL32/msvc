#!/usr/bin/env python3
"""Mutation controls for a recorded temporary-name/status boundary; no executions."""
import argparse
import json
import re
from pathlib import Path
import shutil
import struct
import tempfile
from mktemp_state import original,native
from native_status import read as terminal_status

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',type=Path,required=True);parser.add_argument('--native',type=Path,required=True)
args=parser.parse_args()
expected=original(args.original);assert native(args.native)==expected
assert terminal_status(args.native,0)==0
count=0


def control(platform,mutate,different=False):
    global count
    source=args.original if platform=='original' else args.native
    with tempfile.TemporaryDirectory(prefix='boundary-control-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cl') as work:
        path=Path(work)/'raw';shutil.copytree(source,path);mutate(path)
        try:value=(original if platform=='original' else native)(path)
        except (AssertionError,ValueError,KeyError):pass
        else:assert different and value!=expected,'mutated boundary passed unchanged'
    count+=1


def text(path,name,transform):
    p=path/name;p.write_text(transform(p.read_text()))


def word(path,name,offset,value):
    p=path/name;data=bytearray(p.read_bytes());struct.pack_into('<I',data,offset,value);p.write_bytes(data)


def json_field(path,name,key,value):
    p=path/name;data=json.loads(p.read_text());data[key]=value;p.write_text(json.dumps(data))


control('original',lambda p:text(p,'mktemp-events.txt',lambda s:s+s.splitlines()[1]+'\n'))
control('original',lambda p:text(p,'mktemp-events.txt',lambda s:re.sub(r'result=[0-9a-f]{8}','result=00000001',s,count=1)))
control('original',lambda p:(p/'mktemp-before.bin').write_bytes(b'otherXXXXXX\0'))
control('original',lambda p:word(p,'mktemp-entry-teb.bin',0x34,123),different=True)
control('original',lambda p:word(p,'mktemp-before-ptd.bin',8,111),different=True)
control('native',lambda p:word(p,'mktemp-receipt.bin',8,1))
control('native',lambda p:word(p,'mktemp-receipt.bin',12,2))
control('native',lambda p:(p/'mktemp-receipt.bin').write_bytes((p/'mktemp-receipt.bin').read_bytes()[:-1]))
control('native',lambda p:json_field(p,'native-mktemp-gdb.json','calls',[]))
def altered_gdb(p):
    path=p/'native-mktemp-gdb.json';data=json.loads(path.read_text());data['calls'][0]['before']['last_error']=0;path.write_text(json.dumps(data))
control('native',altered_gdb)
control('native',lambda p:json_field(p,'native-entry-errors.json','last_error',0),different=True)
with tempfile.TemporaryDirectory(prefix='terminal-control-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cl') as work:
    path=Path(work)/'raw';shutil.copytree(args.native,path)
    word(path,'terminal-status.bin',16,256)
    assert terminal_status(path,0)==256,'high DWORD status bits were discarded'
    word(path,'terminal-status.bin',8,1)
    try:terminal_status(path,0)
    except ValueError:pass
    else:raise AssertionError('wrong terminal owner passed')
    count+=2
print('Original/native boundary positive control and',count,'semantic mutation controls pass; no child execution')
