#!/usr/bin/env python3
"""Completed allocator/table state and undefined-malloc boundary controls."""
import json
from pathlib import Path
import re
import shutil
import tempfile
import time
from compare import compare
from state import sha


def main():
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True);parser.add_argument('--native',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();begin=time.monotonic();positive=compare(args.original,args.native);passed=[]
    def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def native(root,change):
        path=root/'n/state.json';state=json.loads(path.read_text());change(state['globals_complete']);write(path,state)
        path=root/'n/gdb.stdout';path.write_text(re.sub(r'^C2-GLOBALS-COMPLETE .*$',lambda m:'C2-GLOBALS-COMPLETE '+json.dumps(state['globals_complete'],sort_keys=True),path.read_text(),flags=re.M))
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((root/'n/state.json').read_bytes());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def byte(root,index):
        def change(state):
            value=state['writer'];raw=bytearray.fromhex(value['pool_bytes']);raw[index]^=1;value['pool_bytes']=raw.hex()
        native(root,change)
    def original_result(root):
        path=root/'o/events.txt';before=path.read_text();after=re.sub(r'^(coff-initialized malloc-result=)[0-9a-f]+$',lambda m:m[1]+'00000000',before,flags=re.M);assert before!=after;path.write_text(after)
        path=root/'o/receipt.json';receipt=json.loads(path.read_text());receipt['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,receipt)
    controls=[('table allocation offset differs',lambda r:native(r,lambda v:v['coff_allocations'].__setitem__('table_offset',68))),
        ('table allocation extent differs',lambda r:native(r,lambda v:v['coff_allocations'].__setitem__('table_bytes',28))),
        ('page allocation extent differs',lambda r:native(r,lambda v:v['coff_allocations'].__setitem__('page_bytes',1204))),
        ('opaque storage allocation extent differs',lambda r:native(r,lambda v:v['coff_allocations'].__setitem__('storage_bytes',20))),
        ('malloc payload wrongly claims zero defaults',lambda r:native(r,lambda v:v['coff_allocations']['symbol_buffer'].__setitem__('payload','zero initialized'))),
        ('malloc capacity differs',lambda r:native(r,lambda v:v['coff'].__setitem__('coff_symbol_capacity',1024))),
        ('table global loses arena alias',lambda r:native(r,lambda v:v['coff'].__setitem__('coff_table',0))),
        ('storage global loses arena alias',lambda r:native(r,lambda v:v['coff'].__setitem__('coff_storage',0))),
        ('original malloc result lineage differs',original_result)]
    for name,index in [('capacity',68),('untouched padding',70),('first page pointer',88),('explicit first entry',100),('opaque storage zero provenance',1304),('unallocated arena tail',4095)]:
        controls.append(('table/page '+name+' differs',lambda r,index=index:byte(r,index)))
    with tempfile.TemporaryDirectory(prefix='coff-allocation-state-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for index,(name,change)in enumerate(controls):
            root=Path(folder)/str(index);shutil.copytree(args.original,root/'o');shutil.copytree(args.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('allocation mutation accepted: '+name)
    args.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((args.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((args.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
