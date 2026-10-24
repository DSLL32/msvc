#!/usr/bin/env python3
"""New second-timer/terminal stream, cleanup and architectural-state controls."""
import argparse,json,time
from pathlib import Path
from clock_provider import verify

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
directory=args.directory;passed=[];started=time.perf_counter();verify(directory)
def denied(label,name,change,binary=False):
    path=directory/name;raw=path.read_bytes()
    try:
        value=bytearray(raw)if binary else json.loads(raw);change(value)
        path.write_bytes(bytes(value)if binary else(json.dumps(value,indent=2)+'\n').encode())
        try:verify(directory)
        except(AssertionError,KeyError,ValueError,IndexError):passed.append(label)
        else:raise AssertionError('accepted '+label)
    finally:path.write_bytes(raw)
for phase in ('link-postchild','link-exit'):
    for family in ('crt','win32'):
        for field in ('position','handle'):
            denied(phase+' '+family+' '+field,'streams-'+phase+'.json',
                lambda v,family=family,field=field:v[family][1].__setitem__(field,v[family][1][field]+1))
    denied(phase+' FILE word','streams-'+phase+'-iob.bin',lambda v:v.__setitem__(16,v[16]^1),True)
for field,value in [('result',1),('present_before',False),('present_after',True),('argument_address',0),('filename_bytes','00')]:
    denied('unlink '+field,'native-link-unlink-gdb.json',lambda v,field=field,value=value:v.__setitem__(field,value))
verify(directory)
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed}))
