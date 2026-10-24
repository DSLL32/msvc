#!/usr/bin/env python3
"""Record-preparation NULL formal and assembly-output setup controls."""
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
        prefix={'record_preparation_input':'C2-RECORD-PREPARATION-INPUT','globals_complete':'C2-GLOBALS-COMPLETE'}[field];path=root/'n/gdb.stdout';text=path.read_text();assert len(re.findall('^'+prefix+' ',text,re.M))==1;path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(v[field],sort_keys=True),text,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,index):
        path=root/'o'/file;raw=bytearray(path.read_bytes());raw[index]^=1;path.write_bytes(raw);path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha(raw)
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=sha(raw)
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';raw=path.read_text();changed=re.sub(pattern,replacement,raw,flags=re.M);assert changed!=raw;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('actual function formal omitted',lambda r:native(r,'record_preparation_input',lambda v:v.__setitem__('formals',[]))),
        ('function formal wrong type',lambda r:native(r,'record_preparation_input',lambda v:v['formals'][0].__setitem__('type','void *'))),
        ('function NULL argument lost',lambda r:native(r,'record_preparation_input',lambda v:v.__setitem__('function_null',False))),
        ('listing enabled unexpectedly',lambda r:native(r,'record_preparation_input',lambda v:v.__setitem__('assembly_output_active',1))),
        ('unreached comment getter pretended executed',lambda r:native(r,'record_preparation_input',lambda v:v.__setitem__('assembly_comment_character',59))),
        ('wrong live controller caller',lambda r:native(r,'record_preparation_input',lambda v:v.__setitem__('caller','initialize_source_output'))),
        ('globals projection kept departed source formal',lambda r:native(r,'globals_complete',lambda v:v.__setitem__('formals',[{'name':'source','type':'char *'}]))),
        ('native complete active global differs',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('assembly_output_active',1))),
        ('native complete comment global differs',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('assembly_comment_character',59))),
        ('original actual function ECX nonNULL',lambda r:event(r,r'^(observe 0041f59c eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000001')),
        ('original active flag differs',lambda r:original(r,'unmodeled-original-data.bin',0x497da4-0x48e000)),
        ('original untouched comment byte differs',lambda r:original(r,'unmodeled-original-data.bin',0x49e69c-0x48e000)),
        ('original assembly-enabled input differs',lambda r:original(r,'unmodeled-original-data.bin',0x497f98-0x48e000)),
        ('original mixed-assembly input differs',lambda r:original(r,'unmodeled-original-data.bin',0x497f94-0x48e000))]
    with tempfile.TemporaryDirectory(prefix='source-output-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('source/frontier mutation accepted: '+name)
    write(a.output,{'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual C2Function*function NULL formal/inventory/currentECX/liveinitialize_backend caller. Fouractual assembly globals retain exactwidths/initialvalues; active iswritten0, commentremainsuntouched0 and coldhelpers masked. Full38B two-library payload and all prior defined state still compared.'})

if __name__=='__main__':main()
