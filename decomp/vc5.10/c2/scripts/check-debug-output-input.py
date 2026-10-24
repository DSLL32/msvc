#!/usr/bin/env python3
"""No-argument debug-output frontier and actual parent/list controls."""
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
        prefix={'debug_output_input':'C2-DEBUG-OUTPUT-INPUT','globals_complete':'C2-GLOBALS-COMPLETE'}[field];path=root/'n/gdb.stdout';text=path.read_text();assert len(re.findall('^'+prefix+' ',text,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(v[field],sort_keys=True),text,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,index):
        path=root/'o'/file;raw=bytearray(path.read_bytes());raw[index]^=1;path.write_bytes(raw);path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha(raw)
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=sha(raw)
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';raw=path.read_text();changed=re.sub(pattern,replacement,raw,flags=re.M);assert changed!=raw;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('noarg output helper invents formal',lambda r:native(r,'debug_output_input',lambda v:v.__setitem__('formals',[{'name':'symbol','type':'C2Symbol *'}]))),
        ('caller records formal omitted',lambda r:native(r,'debug_output_input',lambda v:v.__setitem__('caller_formals',[]))),
        ('caller records formal wrong type',lambda r:native(r,'debug_output_input',lambda v:v['caller_formals'][0].__setitem__('type','void *'))),
        ('caller records NULL lost',lambda r:native(r,'debug_output_input',lambda v:v.__setitem__('caller_records_null',False))),
        ('wrong actual parent',lambda r:native(r,'debug_output_input',lambda v:v.__setitem__('caller','initialize_backend'))),
        ('queued symbol root NULL lost',lambda r:native(r,'debug_output_input',lambda v:v.__setitem__('pending_null',False))),
        ('full model adds fake formal',lambda r:native(r,'globals_complete',lambda v:v.__setitem__('formals',[{'name':'symbols','type':'C2DebugLink *'}]))),
        ('native queued root changed',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('pending_debug_symbols',1))),
        ('original queued root changed',lambda r:original(r,'unmodeled-original-data.bin',0x497458-0x48e000)),
        ('original parent ESI records nonNULL',lambda r:event(r,r'^(debug-output-input owner=)[0-9a-f]+',r'\g<1>00000001')),
        ('original actual queued root nonNULL',lambda r:event(r,r'^(debug-output-input owner=[0-9a-f]+ pending=)[0-9a-f]+',r'\g<1>00000001'))]
    with tempfile.TemporaryDirectory(prefix='source-output-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('source/frontier mutation accepted: '+name)
    write(a.output,{'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual noarg frontier, process_records C2DebugLink*NULL caller, unchanged shared queued root and all typed globals/output. Naturalalignment headNULL earlyreturn only; nonNULL body execution notclaimed. Common101 strict graph verifies correct distinct is_known_symbol child and whole126B hot/cold ownership.'})

if __name__=='__main__':main()
