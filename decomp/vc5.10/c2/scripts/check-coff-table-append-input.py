#!/usr/bin/env python3
"""Current generic table/item formals and actual caller-owned descriptor state."""
import argparse
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time
from compare import compare
from state import sha


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();begin=time.monotonic();positive=compare(a.original,a.native);passed=[]
    def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def native(root,field,change):
        path=root/'n/state.json';s=json.loads(path.read_text());old=json.dumps(s[field],sort_keys=True);change(s[field]);assert old!=json.dumps(s[field],sort_keys=True);write(path,s)
        prefix={'coff_table_append':'C2-COFF-TABLE-APPEND','globals_complete':'C2-GLOBALS-COMPLETE','coff_section_output':'C2-COFF-SECTION-OUTPUT'}[field];path=root/'n/gdb.stdout';raw=path.read_text();assert len(re.findall('^'+prefix+' ',raw,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(s[field],sort_keys=True),raw,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def record_native(root,at,value):
        def change(s):
            data=bytearray.fromhex(s['record_bytes']);assert data[at:at+len(value)]!=value;data[at:at+len(value)]=value;s['record_bytes']=data.hex()
        native(root,'coff_table_append',change)
    def original(root,file,at,value):
        path=root/'o'/file;data=bytearray(path.read_bytes());assert data[at:at+len(value)]!=value;data[at:at+len(value)]=value;path.write_bytes(data)
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha((root/'o'/file).read_bytes())
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=r['globals_complete_hashes'][file]
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';old=path.read_text();changed=re.sub(pattern,replacement,old,flags=re.M);assert changed!=old;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('table formal omitted',lambda r:native(r,'coff_table_append',lambda v:v.__setitem__('formals',v['formals'][1:]))),
        ('generic item prototype incorrectly specialized',lambda r:native(r,'coff_table_append',lambda v:v['formals'][1].__setitem__('type','C2OutputSection *'))),
        ('table pointer wrong owned offset',lambda r:native(r,'coff_table_append',lambda v:v.__setitem__('table_offset',96))),
        ('item pointer wrong allocation offset',lambda r:native(r,'coff_table_append',lambda v:v.__setitem__('item_offset',1852))),
        ('table loses actual global alias',lambda r:native(r,'coff_table_append',lambda v:v.__setitem__('table_global_alias',False))),
        ('item loses actual caller allocation alias',lambda r:native(r,'coff_table_append',lambda v:v.__setitem__('item_caller_alias',False))),
        ('caller record size differs',lambda r:native(r,'coff_table_append',lambda v:v.__setitem__('record_size',20))),
        ('record characteristics differ',lambda r:record_native(r,8,struct.pack('<I',0x60000020))),
        ('name written before table child returned',lambda r:record_native(r,4,struct.pack('<I',1))),
        ('index written before table child returned',lambda r:record_native(r,16,struct.pack('<H',1))),
        ('pre-insertion flag8 absent',lambda r:record_native(r,18,bytes(2))),
        ('caller-owned untouched byte21 differs',lambda r:record_native(r,21,b'\1')),
        ('new writer arena cursor differs',lambda r:native(r,'globals_complete',lambda v:v['writer'].__setitem__('cursor_offset',1884))),
        ('new writer arena remaining differs',lambda r:native(r,'globals_complete',lambda v:v['writer'].__setitem__('remaining',2212))),
        ('parent borrowed name alias lost',lambda r:native(r,'coff_section_output',lambda v:v.__setitem__('name_section_alias',False))),
        ('original caller-owned characteristics differ',lambda r:original(r,'writer-pool.bin',1884+8,bytes(4))),
        ('original name prematurely assigned',lambda r:original(r,'writer-pool.bin',1884+4,struct.pack('<I',0x497d7c))),
        ('original reuse index advanced on reuse0',lambda r:original(r,'unmodeled-original-data.bin',0x49d5ec-0x48e000,bytes(4))),
        ('original eight-slot initialization loses NULL',lambda r:original(r,'unmodeled-original-data.bin',0x49e6e0-0x48e000,struct.pack('<I',0x49e300))),
        ('actual table ECX NULL',lambda r:event(r,r'^(observe 0041ef35 eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000000')),
        ('actual generic item EDX NULL',lambda r:event(r,r'^(observe 0041ef35 eax=[0-9a-f]+ ecx=[0-9a-f]+ edx=)[0-9a-f]+',r'\g<1>00000000'))]
    with tempfile.TemporaryDirectory(prefix='coff-table-append-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('table/descriptor state mutation accepted: '+name)
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'The item remains void*. Only this actual caller-owned24B allocation is interpreted; no generic table-pointee layout or unknown reuse content is invented.'},indent=2)+'\n')

if __name__=='__main__':main()
