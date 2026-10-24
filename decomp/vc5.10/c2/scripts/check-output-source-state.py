#!/usr/bin/env python3
"""Actual source-symbol record/payload/caller pointer frontier."""
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
    controls=[('record pointer formal omitted',lambda r:native(r,'object_writer',lambda v:v.__setitem__('formals',[]))),
        ('caller source formal omitted',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_formals',[]))),
        ('actual record pointer offset differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('record_offset',1864))),
        ('caller source name differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_name','wrong.c'))),
        ('caller source global alias differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_name_global_alias',False))),
        ('source kind differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['source_record'].__setitem__('kind',5))),
        ('source payload alias differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['source_record'].__setitem__('source_offset',1860))),
        ('source info name alias differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['source_info'].__setitem__('name_offset',44))),
        ('source next index differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['source_info'].__setitem__('next_source',1))),
        ('source kind byte differs',lambda r:arena(r,1852)),
        ('unwritten field allocator byte differs',lambda r:arena(r,1856)),
        ('source info pointer byte differs',lambda r:arena(r,1860)),
        ('source borrowed name byte differs',lambda r:arena(r,1864)),
        ('source next index byte differs',lambda r:arena(r,1868)),
        ('original actual record pointer differs',lambda r:original(r,'coff-symbol-input record=', 'coff-symbol-input record=0'))]
    with tempfile.TemporaryDirectory(prefix='output-source-state-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for index,(name,change)in enumerate(controls):
            root=Path(folder)/str(index);shutil.copytree(args.original,root/'o');shutil.copytree(args.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('output-source mutation accepted: '+name)
    args.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((args.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((args.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
