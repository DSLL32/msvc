#!/usr/bin/env python3
"""Compile-only observers remain mandatory when that actual contract is selected."""
import argparse
import json
from pathlib import Path
from dependency_provider import verify

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('native',type=Path)
args=parser.parse_args();directory=args.native
assert json.loads((directory/'dependency-response.json').read_text())['completed']
assert verify(directory)['completed']
count=0
for name in ('native-exit-gdb.json','native-unlink-gdb.json','streams-postchild-iob.bin','streams-exit.json'):
    path=directory/name;before=path.read_bytes()
    try:
        path.unlink()
        try:verify(directory)
        except FileNotFoundError:count+=1
        else:raise AssertionError('selected compile contract accepted missing observer: '+name)
    finally:path.write_bytes(before)
assert verify(directory)['completed']
print(json.dumps({'result':'PASS','selected_compile_observer_rejections':count}))
