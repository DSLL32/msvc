#!/usr/bin/env python3
"""Actual two-register list-constructor inputs and reached reset globals."""
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
    def native(root,section,key,value):
        path=root/'n/state.json';state=json.loads(path.read_text())
        if section=='object_writer':state[section][key]=value
        else:state[section]['coff'][key]=value
        write(path,state);path=root/'n/gdb.stdout';prefix='C2-OBJECT-WRITER'if section=='object_writer'else'C2-GLOBALS-COMPLETE'
        path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(state[section],sort_keys=True),path.read_text(),flags=re.M))
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((root/'n/state.json').read_bytes());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def original(root,old,new):
        path=root/'o/events.txt';before=path.read_text();assert old in before;path.write_text(before.replace(old,new))
        path=root/'o/receipt.json';receipt=json.loads(path.read_text());receipt['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,receipt)
    controls=[('actual list kind differs',lambda r:native(r,'object_writer','kind',2)),
        ('actual list context differs',lambda r:native(r,'object_writer','context_null',False)),
        ('actual kind formal omitted',lambda r:native(r,'object_writer','formals',[{'name':'context','type':'void *'}])),
        ('actual context formal omitted',lambda r:native(r,'object_writer','formals',[{'name':'kind','type':'int'}])),
        ('actual context formal type differs',lambda r:native(r,'object_writer','formals',[{'name':'kind','type':'int'},{'name':'context','type':'char *'}])),
        ('original kind ECX differs',lambda r:original(r,'coff-list-input kind=1','coff-list-input kind=2')),
        ('original context EDX differs',lambda r:original(r,'kind=1 context=00000000','kind=1 context=00000001'))]
    for name in ('coff_current_list','coff_free_chunks','coff_free_blocks_508','coff_free_blocks_308','coff_pending_499690'):
        controls.append(('reset global '+name+' differs',lambda r,name=name:native(r,'globals_complete',name,1)))
    with tempfile.TemporaryDirectory(prefix='coff-list-input-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for index,(name,change)in enumerate(controls):
            root=Path(folder)/str(index);shutil.copytree(args.original,root/'o');shutil.copytree(args.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('list-input mutation accepted: '+name)
    args.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((args.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((args.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
