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
        prefix={'coff_section_initialize':'C2-COFF-SECTION-INITIALIZE','coff_section_record':'C2-COFF-SECTION-RECORD','object_writer':'C2-OBJECT-WRITER','globals_complete':'C2-GLOBALS-COMPLETE','coff_section_output':'C2-COFF-SECTION-OUTPUT'}[field];path=root/'n/gdb.stdout';raw=path.read_text();assert len(re.findall('^'+prefix+' ',raw,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(s[field],sort_keys=True),raw,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,at,value):
        path=root/'o'/file;data=bytearray(path.read_bytes());assert data[at:at+len(value)]!=value;data[at:at+len(value)]=value;path.write_bytes(data)
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha((root/'o'/file).read_bytes())
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=r['globals_complete_hashes'][file]
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';old=path.read_text();changed=re.sub(pattern,replacement,old,flags=re.M);assert changed!=old;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('current section formal omitted',lambda r:native(r,'coff_section_record',lambda v:v.__setitem__('formals',[]))),
        ('obsolete prior data formal accepted',lambda r:native(r,'coff_section_record',lambda v:v.__setitem__('formals',[{'name':'data','type':'C2CoffSectionData *'}]))),
        ('global projection retains obsolete data formal',lambda r:native(r,'globals_complete',lambda v:v.__setitem__('formals',[{'name':'data','type':'C2CoffSectionData *'}]))),
        ('writer projection retains obsolete data formal',lambda r:native(r,'object_writer',lambda v:v.__setitem__('formals',[{'name':'data','type':'C2CoffSectionData *'}]))),
        ('current section owner offset differs',lambda r:native(r,'coff_section_record',lambda v:v.__setitem__('offset',1980))),
        ('current section loses creator alias',lambda r:native(r,'coff_section_record',lambda v:v.__setitem__('caller_section_alias',False))),
        ('current actual ECX NULL',lambda r:event(r,r'^(observe 0041f173 eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000000')),
        ('storage count not advanced',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['storage_state'].__setitem__('count',0))),
        ('storage index not advanced',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['storage_state'].__setitem__('next_index',0))),
        ('storage entry data owner differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['storage_entry'].__setitem__('data_offset',1884))),
        ('returned index not stored',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_data'].__setitem__('storage_index',0))),
        ('caller flags not completed',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['inserted_section'].__setitem__('flags',8))),
        ('original storage first pointer cleared',lambda r:original(r,'writer-pool.bin',1304,bytes(4))),
        ('original storage count cleared',lambda r:original(r,'writer-pool.bin',1312,bytes(4))),
        ('original storage nextindex cleared',lambda r:original(r,'writer-pool.bin',1316,bytes(4))),
        ('original entry data pointer cleared',lambda r:original(r,'writer-pool.bin',3344,bytes(4))),
        ('original entry NULL next overwritten',lambda r:original(r,'writer-pool.bin',3340,b'\1\0\0\0')),
        ('original returned storage index cleared',lambda r:original(r,'writer-pool.bin',1956,bytes(2))),
        ('original section flags not completed',lambda r:original(r,'writer-pool.bin',1902,b'\10\0')),
        ('original section count not completed',lambda r:original(r,'unmodeled-original-data.bin',0x497d78-0x48e000,bytes(2)))]
    with tempfile.TemporaryDirectory(prefix='coff-table-append-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('table/descriptor state mutation accepted: '+name)
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual currentC2Section* formal and creator alias, explicit rejection of obsolete prior-data formal in allthree projections;16Bstorage header/8Bentry/count/index and callerdata-index/flags/count writes. No target/ABI/decoder change for validator correction.'},indent=2)+'\n')

if __name__=='__main__':main()
