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
        prefix={'coff_section_initialize':'C2-COFF-SECTION-INITIALIZE','coff_section_symbols':'C2-COFF-SECTION-SYMBOLS','globals_complete':'C2-GLOBALS-COMPLETE','coff_section_output':'C2-COFF-SECTION-OUTPUT'}[field];path=root/'n/gdb.stdout';raw=path.read_text();assert len(re.findall('^'+prefix+' ',raw,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(s[field],sort_keys=True),raw,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,at,value):
        path=root/'o'/file;data=bytearray(path.read_bytes());assert data[at:at+len(value)]!=value;data[at:at+len(value)]=value;path.write_bytes(data)
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha((root/'o'/file).read_bytes())
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=r['globals_complete_hashes'][file]
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';old=path.read_text();changed=re.sub(pattern,replacement,old,flags=re.M);assert changed!=old;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('data formal omitted',lambda r:native(r,'coff_section_symbols',lambda v:v.__setitem__('formals',[]))),
        ('actual pointee type differs',lambda r:native(r,'coff_section_symbols',lambda v:v['formals'][0].__setitem__('type','C2OutputSection *'))),
        ('data owner offset differs',lambda r:native(r,'coff_section_symbols',lambda v:v.__setitem__('offset',1884))),
        ('data loses initializer local alias',lambda r:native(r,'coff_section_symbols',lambda v:v.__setitem__('caller_data_alias',False))),
        ('section backpointer loses table alias',lambda r:native(r,'coff_section_symbols',lambda v:v.__setitem__('section_table_alias',False))),
        ('section loses data backlink',lambda r:native(r,'coff_section_symbols',lambda v:v.__setitem__('section_data_alias',False))),
        ('actual data ECX NULL',lambda r:event(r,r'^(observe 0041f0c4 eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000000')),
        ('data list offset differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_data'].__setitem__('list_offset',1320))),
        ('new chunk owner differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_list'].__setitem__('last_offset',1344))),
        ('allocated data payload size differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_data'].__setitem__('allocation_bytes',52))),
        ('new data selflink cleared',lambda r:original(r,'writer-pool.bin',1908,bytes(4))),
        ('new data section backpointer cleared',lambda r:original(r,'writer-pool.bin',1912,bytes(4))),
        ('new data output list cleared',lambda r:original(r,'writer-pool.bin',1920,bytes(4))),
        ('new data relocation block cleared',lambda r:original(r,'writer-pool.bin',1924,bytes(4))),
        ('new data line block cleared',lambda r:original(r,'writer-pool.bin',1928,bytes(4))),
        ('new data flags overwritten',lambda r:original(r,'writer-pool.bin',1958,b'\1\0')),
        ('uninterpreted gap loses allocatorzero',lambda r:original(r,'writer-pool.bin',1936,b'\1')),
        ('unmodeled recordtail loses allocatorzero',lambda r:original(r,'writer-pool.bin',1967,b'\1')),
        ('new output chunk selflink cleared',lambda r:original(r,'writer-pool.bin',1992,bytes(4))),
        ('relocation last selflink cleared',lambda r:original(r,'writer-pool.bin',2504,bytes(4))),
        ('line count overwritten',lambda r:original(r,'writer-pool.bin',3008,b'\1\0\0\0'))]
    with tempfile.TemporaryDirectory(prefix='coff-table-append-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('table/descriptor state mutation accepted: '+name)
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual typeddata formal/caller/table/backlink aliases; exact60B record,24Blist,508Bchunk and508/308B block initializedprefixes. Gap/tails compared onlyasexistingallocatorzero writer state, no newlayout/payloadcredit.'},indent=2)+'\n')

if __name__=='__main__':main()
