#!/usr/bin/env python3
"""Completed directive bytes and actual source-name frontier controls."""
import argparse
import json
from pathlib import Path
import re
import shutil
import tempfile
import time
from compare import compare
from state import sha


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();start=time.monotonic();positive=compare(a.original,a.native);passed=[]
    def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
    def native(root,field,change):
        path=root/'n/state.json';v=json.loads(path.read_text());before=json.dumps(v[field],sort_keys=True);change(v[field]);assert before!=json.dumps(v[field],sort_keys=True);write(path,v)
        prefix={'source_output_input':'C2-SOURCE-OUTPUT-INPUT','globals_complete':'C2-GLOBALS-COMPLETE'}[field];path=root/'n/gdb.stdout';text=path.read_text();assert len(re.findall('^'+prefix+' ',text,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(v[field],sort_keys=True),text,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,index):
        path=root/'o'/file;raw=bytearray(path.read_bytes());raw[index]^=1;path.write_bytes(raw);path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha(raw)
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=sha(raw)
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';raw=path.read_text();changed=re.sub(pattern,replacement,raw,flags=re.M);assert changed!=raw;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('actual source formal omitted',lambda r:native(r,'source_output_input',lambda v:v.__setitem__('formals',[]))),
        ('source formal wrong type',lambda r:native(r,'source_output_input',lambda v:v['formals'][0].__setitem__('type','void *'))),
        ('source name differs',lambda r:native(r,'source_output_input',lambda v:v.__setitem__('source','other.c'))),
        ('borrowed source global alias lost',lambda r:native(r,'source_output_input',lambda v:v.__setitem__('source_global_alias',False))),
        ('returned directive frame substituted',lambda r:native(r,'source_output_input',lambda v:v.__setitem__('caller','write_coff_directive'))),
        ('source formal omitted from globals projection',lambda r:native(r,'globals_complete',lambda v:v.__setitem__('formals',[]))),
        ('complete directive write count differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_list'].__setitem__('write_count',5))),
        ('complete directive byte count differs',lambda r:native(r,'globals_complete',lambda v:v['coff_allocations']['section_list'].__setitem__('used_bytes',37))),
        ('original actual source ECX lost',lambda r:event(r,r'^(observe 00453e64 eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000000')),
        ('original source global alias lost',lambda r:original(r,'unmodeled-original-data.bin',0x497dc4-0x48e000)),
        ('original captured source contents differ',lambda r:original(r,'source-output-input.bin',0)),
        ('original first directive space differs',lambda r:original(r,'writer-pool.bin',1996+16)),
        ('original second prefix differs',lambda r:original(r,'writer-pool.bin',1996+17)),
        ('original second library differs',lambda r:original(r,'writer-pool.bin',1996+29)),
        ('original final space differs',lambda r:original(r,'writer-pool.bin',1996+37)),
        ('original section size differs',lambda r:original(r,'pool-block.bin',1904)),
        ('original used bytes differs',lambda r:original(r,'writer-pool.bin',1980)),
        ('original write count differs',lambda r:original(r,'writer-pool.bin',1984))]
    with tempfile.TemporaryDirectory(prefix='source-output-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('source/frontier mutation accepted: '+name)
    write(a.output,{'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual char*source formal/inventory/currentECX+global497dc4 alias/full22B NULstring/liveinitialize_backend caller. Returned directiveframes notclaimed. Full38B two-library payload/section_size38/used38/write_count6 and all prior defined data compared.'})

if __name__=='__main__':main()
