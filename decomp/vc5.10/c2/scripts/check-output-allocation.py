#!/usr/bin/env python3
"""First table allocation: actual stub/caller formals and original callsite."""
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
    def native(root,key,value):
        path=root/'n/state.json';state=json.loads(path.read_text());state['object_writer'][key]=value;write(path,state)
        path=root/'n/gdb.stdout';path.write_text(re.sub(r'^C2-OBJECT-WRITER .*$',lambda m:'C2-OBJECT-WRITER '+json.dumps(state['object_writer'],sort_keys=True),path.read_text(),flags=re.M))
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((root/'n/state.json').read_bytes());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def original(root,old,new):
        path=root/'o/events.txt';before=path.read_text();assert old in before;path.write_text(before.replace(old,new))
        path=root/'o/receipt.json';receipt=json.loads(path.read_text());receipt['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,receipt)
    controls=[('actual allocator unsigned bytes differs',lambda r:native(r,'allocation_bytes',33)),
        ('actual allocator formal missing',lambda r:native(r,'formals',[])),
        ('actual allocator formal type differs',lambda r:native(r,'formals',[{'name':'bytes','type':'unsigned short'}])),
        ('actual constructor capacity differs',lambda r:native(r,'capacity',301)),
        ('actual constructor caller differs',lambda r:native(r,'caller_function','initialize_coff_output')),
        ('actual constructor formal omitted',lambda r:native(r,'caller_formals',[])),
        ('actual constructor formal type differs',lambda r:native(r,'caller_formals',[{'name':'capacity','type':'int'}])),
        ('original allocator ECX bytes differs',lambda r:original(r,'coff-allocate-input bytes=32','coff-allocate-input bytes=33')),
        ('original constructor DI capacity differs',lambda r:original(r,'bytes=32 capacity=300','bytes=32 capacity=301')),
        ('original actual first allocation callsite differs',lambda r:original(r,'observe-stack 00453d8b','observe-stack 00453dae'))]
    with tempfile.TemporaryDirectory(prefix='table-allocation-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for index,(name,change)in enumerate(controls):
            root=Path(folder)/str(index);shutil.copytree(args.original,root/'o');shutil.copytree(args.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('allocation mutation accepted: '+name)
    args.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((args.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((args.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
