#!/usr/bin/env python3
"""Typed record frontier and approved physical empty-function ABI controls."""
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
        prefixes={'process_records_input':'C2-PROCESS-RECORDS-INPUT','globals_complete':'C2-GLOBALS-COMPLETE','global_noop_abi':'C2-GLOBAL-NOOP-ABI'}
        prefix=prefixes[field];path=root/'n/gdb.stdout';text=path.read_text();rows=re.findall('^'+prefix+r' .*$',text,re.M)
        values=v[field]if field=='global_noop_abi'else[v[field]]
        assert len(rows)==len(values)
        for before,item in zip(rows,values):text=text.replace(before,prefix+' '+json.dumps(item,sort_keys=True),1)
        path.write_text(text)
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,file,index):
        path=root/'o'/file;raw=bytearray(path.read_bytes());raw[index]^=1;path.write_bytes(raw);path=root/'o/receipt.json';r=json.loads(path.read_text());r['globals_complete_hashes'][file]=sha(raw)
        if file=='unmodeled-original-data.bin':r['unmodeled_original_data']['sha256']=sha(raw)
        write(path,r)
    def event(root,pattern,replacement):
        path=root/'o/events.txt';raw=path.read_text();changed=re.sub(pattern,replacement,raw,flags=re.M);assert changed!=raw;path.write_text(changed);path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('records formal omitted',lambda r:native(r,'process_records_input',lambda v:v.__setitem__('formals',[]))),
        ('records formal wrong type',lambda r:native(r,'process_records_input',lambda v:v['formals'][0].__setitem__('type','void *'))),
        ('records formal NULL lost',lambda r:native(r,'process_records_input',lambda v:v.__setitem__('records_null',False))),
        ('wrong actual parent',lambda r:native(r,'process_records_input',lambda v:v.__setitem__('caller','prepare_global_records'))),
        ('full model missing actual formal',lambda r:native(r,'globals_complete',lambda v:v.__setitem__('formals',[]))),
        ('native processing flag not reset',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('global_records_processing',1))),
        ('native initializer head not NULL',lambda r:native(r,'globals_complete',lambda v:v['coff'].__setitem__('global_initializers',1))),
        ('original processing flag not reset',lambda r:original(r,'unmodeled-original-data.bin',0x497df0-0x48e000)),
        ('original initializer head not NULL',lambda r:original(r,'unmodeled-original-data.bin',0x497d98-0x48e000)),
        ('original records formal ECX nonNULL',lambda r:event(r,r'^(process-records-input records=)[0-9a-f]+',r'\g<1>00000001')),
        ('original returned head EAX nonNULL',lambda r:event(r,r'^(process-records-input records=[0-9a-f]+ result=)[0-9a-f]+',r'\g<1>00000001'))]
    def npoint(index,key,value):return lambda r:native(r,'global_noop_abi',lambda v:v[index].__setitem__(key,value(v[index][key])if callable(value)else value))
    for label,index,key,value in [('native call PC wrong owner',0,'pc',lambda v:v+1),('native entry wrong physical target',1,'pc',lambda v:v+1),('native return PC wrong',2,'pc',lambda v:v+1),('native call ECX not head',0,'ecx',1),('native entry ECX not head',1,'ecx',1),('native call processing flag absent',0,'flag',0),('native return processing flag prematurely cleared',2,'flag',0),('native entry stack imbalance',1,'esp',lambda v:v+4),('native return hidden cleanup',2,'esp',lambda v:v+4),('native return EBX changed',2,'ebx',lambda v:v^1),('native return EBP changed',2,'ebp',lambda v:v^1),('native return ESI changed',2,'esi',lambda v:v^1),('native return EDI changed',2,'edi',lambda v:v^1)]:cases.append((label,npoint(index,key,value)))
    cases += [('native entry wrong return address',lambda r:native(r,'global_noop_abi',lambda v:v[1]['stack'].__setitem__(0,v[1]['stack'][0]+1))),
        ('original call wrong physical target',lambda r:event(r,r'^(global-noop-abi role=entry pc=)[0-9a-f]+',r'\g<1>0041eb8a')),
        ('original call ECX not head',lambda r:event(r,r'^(global-noop-abi role=call pc=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000001')),
        ('original entry ECX not head',lambda r:event(r,r'^(global-noop-abi role=entry pc=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000001')),
        ('original head ESI not NULL',lambda r:event(r,r'^(global-noop-abi role=call .* esi=)[0-9a-f]+',r'\g<1>00000001')),
        ('original return EBX clobbered',lambda r:event(r,r'^(global-noop-abi role=return .* ebx=)[0-9a-f]+',r'\g<1>00000001')),
        ('original return processing flag cleared early',lambda r:event(r,r'^(global-noop-abi role=return .* flag=)[0-9a-f]+',r'\g<1>00000000'))]
    with tempfile.TemporaryDirectory(prefix='source-output-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except(AssertionError,KeyError,ValueError)as e:passed.append({'control':name,'reason':str(e)})
            else:raise AssertionError('source/frontier mutation accepted: '+name)
    write(a.output,{'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-start,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Actual C2DebugLink*NULL frontier, initialized head/processing flag, raw call/entry/return physical41eb89 ABI and liveELF instruction/target binding. Approved direct use only;17 stores-only alias slots unchanged; no new body/ABI credit for pendingchildren. Full99 strict common graph audit independent.'})

if __name__=='__main__':main()
