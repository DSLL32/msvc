#!/usr/bin/env python3
"""Actual four-formal COFF section creator output boundary."""
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
    a=p.parse_args();begin=time.monotonic();positive=compare(a.original,a.native);passed=[]
    def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def native(root,change):
        path=root/'n/state.json';s=json.loads(path.read_text());before=json.dumps(s['coff_section_output'],sort_keys=True);change(s['coff_section_output']);assert before!=json.dumps(s['coff_section_output'],sort_keys=True);write(path,s)
        path=root/'n/gdb.stdout';raw=path.read_text();assert raw.count('C2-COFF-SECTION-OUTPUT ')==1;path.write_text(re.sub(r'^C2-COFF-SECTION-OUTPUT .*$',lambda m:'C2-COFF-SECTION-OUTPUT '+json.dumps(s['coff_section_output'],sort_keys=True),raw,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,pattern,replacement):
        path=root/'o/events.txt';before=path.read_text();after=re.sub(pattern,replacement,before,flags=re.M);assert before!=after;path.write_text(after)
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    cases=[('name formal omitted',lambda r:native(r,lambda v:v.__setitem__('formals',v['formals'][1:]))),
        ('reuse stackformal omitted',lambda r:native(r,lambda v:v.__setitem__('formals',v['formals'][:-1]))),
        ('flags formal wrong width',lambda r:native(r,lambda v:v['formals'][1].__setitem__('type','unsigned short'))),
        ('section name differs',lambda r:native(r,lambda v:v.__setitem__('name','.data'))),
        ('section flags differ',lambda r:native(r,lambda v:v.__setitem__('flags',0x60000020))),
        ('first stack flag differs',lambda r:native(r,lambda v:v.__setitem__('flag8',0))),
        ('second stack reuse flag differs',lambda r:native(r,lambda v:v.__setitem__('reuse',1))),
        ('section name loses owner field alias',lambda r:native(r,lambda v:v.__setitem__('name_section_alias',False))),
        ('actual creator caller differs',lambda r:native(r,lambda v:v.__setitem__('caller','select_output_section'))),
        ('original actual flags EDX differs',lambda r:original(r,r'^(observe 004522ec eax=[0-9a-f]+ ecx=[0-9a-f]+ edx=)[0-9a-f]+',r'\g<1>00000000')),
        ('original actual first stackarg differs',lambda r:original(r,r'^observe-stack 0041eedd 00000001 00000000',r'observe-stack 0041eedd 00000000 00000000')),
        ('original actual second stackarg differs',lambda r:original(r,r'^observe-stack 0041eedd 00000001 00000000',r'observe-stack 0041eedd 00000001 00000001'))]
    with tempfile.TemporaryDirectory(prefix='coff-section-input-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        for i,(name,change)in enumerate(cases):
            root=Path(folder)/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('section input mutation accepted: '+name)
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
