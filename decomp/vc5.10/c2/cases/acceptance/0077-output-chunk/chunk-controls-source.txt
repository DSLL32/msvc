#!/usr/bin/env python3
"""Completed list/chunk fields and actual source-pointer frontier."""
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
    def native(root,section,change):
        path=root/'n/state.json';state=json.loads(path.read_text());change(state[section]);write(path,state)
        path=root/'n/gdb.stdout';prefix='C2-OBJECT-WRITER'if section=='object_writer'else'C2-GLOBALS-COMPLETE'
        path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(state[section],sort_keys=True),path.read_text(),flags=re.M))
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((root/'n/state.json').read_bytes());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def original(root,old,new):
        path=root/'o/events.txt';before=path.read_text();assert old in before;path.write_text(before.replace(old,new))
        path=root/'o/receipt.json';receipt=json.loads(path.read_text());receipt['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,receipt)
    def arena(root,offset):
        def change(value):
            writer=value['writer'];data=bytearray.fromhex(writer['pool_bytes']);data[offset]^=1;writer['pool_bytes']=data.hex()
        native(root,'globals_complete',change)
    controls=[('source char pointer formal omitted',lambda r:native(r,'object_writer',lambda v:v.__setitem__('formals',[]))),
        ('source name differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('name','wrong.c'))),
        ('source global alias lost',lambda r:native(r,'object_writer',lambda v:v.__setitem__('name_global_alias',False))),
        ('source pool offset differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('name_pool_offset',44))),
        ('completed list last chunk differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['list'].__setitem__('last_offset',1348))),
        ('completed list count-only differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['list'].__setitem__('count_only',1))),
        ('chunk allocation shortened',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['chunk'].__setitem__('allocation_bytes',504))),
        ('chunk self alias lost',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['chunk'].__setitem__('next_self_alias',False))),
        ('chunk prefix mistaken for full layout',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['chunk'].__setitem__('modeled_prefix_bytes',508))),
        ('chunk tail claimed as object data',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['chunk'].__setitem__('tail','fully reconstructed payload'))),
        ('list last pointer byte differs',lambda r:arena(r,1340)),
        ('chunk link byte differs',lambda r:arena(r,1344)),
        ('chunk allocator-zeroed tail differs',lambda r:arena(r,1851)),
        ('original source alias differs',lambda r:original(r,'coff-source-input name=', 'coff-source-input name=0'))]
    with tempfile.TemporaryDirectory(prefix='output-chunk-state-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for index,(name,change)in enumerate(controls):
            root=Path(folder)/str(index);shutil.copytree(args.original,root/'o');shutil.copytree(args.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('output-chunk mutation accepted: '+name)
    args.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((args.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((args.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
