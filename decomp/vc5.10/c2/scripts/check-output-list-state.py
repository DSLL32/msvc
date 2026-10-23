#!/usr/bin/env python3
"""Actual integer-flag constructor formals and its pending24B allocation."""
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
    def byte(root):
        def change(value):
            writer=value['writer'];data=bytearray.fromhex(writer['pool_bytes']);data[1320]=1;writer['pool_bytes']=data.hex()
        native(root,'globals_complete',change)
    controls=[('actual buffered flag differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('buffered',0))),
        ('actual count-only flag differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('count_only',1))),
        ('actual buffered formal omitted',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_formals',[{'name':'count_only','type':'int'}]))),
        ('actual count-only formal omitted',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_formals',[{'name':'buffered','type':'int'}]))),
        ('old pointer flag prototype retained',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_formals',[{'name':'buffered','type':'int'},{'name':'count_only','type':'void *'}]))),
        ('actual caller list allocation alias differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('list_offset',1324))),
        ('pending list allocated extent differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['pending_list'].__setitem__('bytes',20))),
        ('pending constructor fields claimed initialized',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['pending_list'].__setitem__('contents','all constructor fields initialized'))),
        ('pending list known-zero byte differs',byte),
        ('original cached buffered flag differs',lambda r:original(r,'coff-chunk-input buffered=1','coff-chunk-input buffered=0')),
        ('original cached count-only flag differs',lambda r:original(r,'buffered=1 count-only=0','buffered=1 count-only=1'))]
    with tempfile.TemporaryDirectory(prefix='output-list-state-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for index,(name,change)in enumerate(controls):
            root=Path(folder)/str(index);shutil.copytree(args.original,root/'o');shutil.copytree(args.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('output-list mutation accepted: '+name)
    args.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((args.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((args.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
