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
        prefix={'coff_section_initialize':'C2-COFF-SECTION-INITIALIZE','coff_storage_input':'C2-COFF-STORAGE-INPUT','globals_complete':'C2-GLOBALS-COMPLETE','coff_section_output':'C2-COFF-SECTION-OUTPUT'}[field];path=root/'n/gdb.stdout';raw=path.read_text();assert len(re.findall('^'+prefix+' ',raw,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(s[field],sort_keys=True),raw,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,at,value):
        path=root/'o'/file;data=bytearray(path.read_bytes());assert data[at:at+len(value)]!=value;data[at:at+len(value)]=value;path.write_bytes(data)
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha((root/'o'/file).read_bytes())
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=r['globals_complete_hashes'][file]
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';old=path.read_text();changed=re.sub(pattern,replacement,old,flags=re.M);assert changed!=old;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('storage data formal omitted',lambda r:native(r,'coff_storage_input',lambda v:v.__setitem__('formals',[]))),
        ('storage data pointee type differs',lambda r:native(r,'coff_storage_input',lambda v:v['formals'][0].__setitem__('type','void *'))),
        ('storage data owner differs',lambda r:native(r,'coff_storage_input',lambda v:v.__setitem__('offset',3316))),
        ('storage data loses caller alias',lambda r:native(r,'coff_storage_input',lambda v:v.__setitem__('caller_data_alias',False))),
        ('actual data ECX NULL',lambda r:event(r,r'^(observe 0041f126 eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000000')),
        ('section symbol index differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_data'].__setitem__('symbol_index',0))),
        ('initialized symbol prefix incomplete',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['symbol_buffer']['initialized_entries'].pop())),
        ('new section symbol kind differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_symbol_record'].__setitem__('kind',1))),
        ('new auxiliary kind differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_auxiliary_record'].__setitem__('kind',2))),
        ('original section symbol slot omitted',lambda r:original(r,'coff-symbol-entries.bin',12,bytes(4))),
        ('original auxiliary slot omitted',lambda r:original(r,'coff-symbol-entries.bin',16,bytes(4))),
        ('original section symbol kind differs',lambda r:original(r,'writer-pool.bin',3316,b'\1\0\0\0')),
        ('original section record data alias lost',lambda r:original(r,'writer-pool.bin',3324,bytes(4))),
        ('original auxiliary kind omitted',lambda r:original(r,'writer-pool.bin',3328,bytes(4))),
        ('original section data symbol index omitted',lambda r:original(r,'writer-pool.bin',1916,bytes(4))),
        ('original symbol total regresses',lambda r:original(r,'unmodeled-original-data.bin',0x497d4c-0x48e000,b'\3\0\0\0')),
        ('original section flags set before storage returns',lambda r:original(r,'writer-pool.bin',1902,b'\13\0'))]
    with tempfile.TemporaryDirectory(prefix='coff-table-append-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('table/descriptor state mutation accepted: '+name)
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actualstorageformal/caller/table/backlink aliases; two12B section/aux records and allfive initializedmallocpointers only20B. Remaining4076B UNMODELED. Actualsymbolindex3 and flags8 before storage-return arechecked.'},indent=2)+'\n')

if __name__=='__main__':main()
