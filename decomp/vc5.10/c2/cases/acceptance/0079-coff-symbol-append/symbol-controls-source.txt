#!/usr/bin/env python3
"""Initialized COFF symbol slots, distinct counters/section pointer and new sections."""
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
    def original_entries(root):
        path=root/'o/coff-symbol-entries.bin';data=bytearray(path.read_bytes());data[8]^=4;path.write_bytes(data)
        path=root/'o/receipt.json';value=json.loads(path.read_text());value['globals_complete_hashes']['coff-symbol-entries.bin']=sha(data);write(path,value)
    def original_caller(root):
        path=root/'o/events.txt';before=path.read_text();after=re.sub(r'^(object-writer-input writer=[0-9a-f]+ caller=)[0-9a-f]+',r'\g<1>00000000',before,flags=re.M);assert before!=after;path.write_text(after)
        path=root/'o/receipt.json';value=json.loads(path.read_text());value['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,value)
    controls=[('symbol total differs',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('coff_symbol_total',4))),
        ('next symbol index differs',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('coff_next_symbol',2))),
        ('current section confused with total',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('coff_current_section',3))),
        ('previous source alias differs',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('coff_previous_source',0))),
        ('third initialized symbol alias differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['symbol_buffer'].__setitem__('initialized_entries',[1852,1872,1852]))),
        ('initialized table extent truncated',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['symbol_buffer'].__setitem__('initialized_entries',[1852,1872]))),
        ('malloc remainder falsely claimed zero',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['symbol_buffer'].__setitem__('payload','all4096 bytes zero initialized'))),
        ('auxiliary record kind differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['auxiliary_record'].__setitem__('kind',1))),
        ('auxiliary kind byte differs',lambda r:arena(r,1872)),
        ('auxiliary unwritten byte differs',lambda r:arena(r,1880)),
        ('new section list truncated',lambda r:native(r,'globals_complete',lambda v:v.__setitem__('sections',v['sections'][:-1]))),
        ('new section serial differs',lambda r:native(r,'globals_complete',lambda v:v['objects'][-1].__setitem__('serial',151))),
        ('new section class differs',lambda r:native(r,'globals_complete',lambda v:v['objects'][-1].__setitem__('class_name','WRONG'))),
        ('native writer caller pool alias differs',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_writer_pool_alias',False))),
        ('original initialized third slot differs',original_entries),
        ('original actual caller writer differs',original_caller)]
    with tempfile.TemporaryDirectory(prefix='coff-symbol-state-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for index,(name,change)in enumerate(controls):
            root=Path(folder)/str(index);shutil.copytree(args.original,root/'o');shutil.copytree(args.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('coff-symbol mutation accepted: '+name)
    args.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'original_receipt_sha256':sha((args.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((args.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
