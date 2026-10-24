#!/usr/bin/env python3
"""Check the actual typed section formal and finite storage-only callback view."""
import argparse
import copy
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time
import callback_evidence
from compare import compare
from state import BINARY, sha


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();begin=time.monotonic();positive=compare(a.original,a.native);passed=[]
    def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def native(root,field,change):
        path=root/'n/state.json';state=json.loads(path.read_text());old=json.dumps(state[field],sort_keys=True);change(state[field]);assert old!=json.dumps(state[field],sort_keys=True);write(path,state)
        prefix={'section_callback':'C2-SECTION-CALLBACK','coff_directive_output':'C2-COFF-DIRECTIVE-OUTPUT','object_writer':'C2-OBJECT-WRITER'}[field]
        path=root/'n/gdb.stdout';raw=path.read_text();assert len(re.findall('^'+prefix+' ',raw,re.M))==1
        path.write_text(re.sub('^'+prefix+r' .*$',lambda m:prefix+' '+json.dumps(state[field],sort_keys=True),raw,flags=re.M))
        path=root/'n/receipt.json';r=json.loads(path.read_text());r['state_sha256']=sha((root/'n/state.json').read_bytes());r['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,r)
    def original(root,pattern,replacement):
        path=root/'o/events.txt';raw=path.read_text();changed=re.sub(pattern,replacement,raw,flags=re.M);assert changed!=raw;path.write_text(changed)
        path=root/'o/receipt.json';r=json.loads(path.read_text());r['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,r)
    controls=[
        ('section formal omitted',lambda r:native(r,'section_callback',lambda v:v.__setitem__('formals',[]))),
        ('section formal wrong type',lambda r:native(r,'section_callback',lambda v:v['formals'][0].__setitem__('type','void *'))),
        ('section record wrong offset',lambda r:native(r,'section_callback',lambda v:v.__setitem__('section_offset',1980))),
        ('section global alias lost',lambda r:native(r,'section_callback',lambda v:v.__setitem__('directive_global_alias',False))),
        ('stored target identity lost',lambda r:native(r,'section_callback',lambda v:v.__setitem__('stored_target_alias',False))),
        ('actual direct caller changed',lambda r:native(r,'section_callback',lambda v:v.__setitem__('caller','initialize_coff_output'))),
        ('actual parent value alias lost',lambda r:native(r,'coff_directive_output',lambda v:v.__setitem__('caller_value_alias',False))),
        ('writer pool alias lost',lambda r:native(r,'object_writer',lambda v:v.__setitem__('caller_writer_pool_alias',False))),
        ('original actual section ECX NULL',lambda r:original(r,r'^(observe 0041ec20 eax=[0-9a-f]+ ecx=)[0-9a-f]+',r'\g<1>00000000')),
        ('original actual section return address changed',lambda r:original(r,r'^observe-stack 00453ebd ',r'observe-stack 00453ebc '))]
    with tempfile.TemporaryDirectory(prefix='section-callback-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        root=Path(folder)
        for i,(name,change)in enumerate(controls):
            trial=root/str(i);shutil.copytree(a.original,trial/'o');shutil.copytree(a.native,trial/'n');change(trial)
            try:compare(trial/'o',trial/'n')
            except (AssertionError,KeyError,ValueError)as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('actual section mutation accepted: '+name)
        clone=root/'static';shutil.copytree(BINARY/'src',clone/'src');(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True)
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        for name in ('output-callback-boundary.json','directive-callback-contract.json','section-callback-storage-contract.json'):shutil.copyfile(BINARY/'cases/analysis'/name,clone/'cases/analysis'/name)
        config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text());callback_evidence.BINARY=clone
        try:
            assert callback_evidence.verify(config,graph)
            mutations=[
                ('erased stored section slot invoked','src/main.c','','\nvoid bad(void) { output_slot_497ce4.target(); }\n'),
                ('stored section slot escaped','src/main.c','','\nvoid bad(void) { void *p = &output_slot_497ce4; }\n'),
                ('typed section target escaped elsewhere','src/main.c','','\nvoid bad(void) { select_output_section(0); }\n'),
                ('storage cast omitted','src/output-callbacks.c','(void (__cdecl *)()) select_output_section','select_output_section'),
                ('storage cast generalized to other slot','src/output-callbacks.c','output_slot_497de4.target = output_callback_41eb89;','output_slot_497de4.target = (void (__cdecl *)()) output_callback_41eb89;'),
                ('typed direct caller wrong section','src/coff-output.c','select_output_section(directive_section);','select_output_section(code_section);'),
                ('direct section convention changed','src/output-callbacks.h','void __fastcall select_output_section(C2Section *section);','void __cdecl select_output_section(C2Section *section);')]
            for name,file,before,after in mutations:
                path=clone/file;old=path.read_text();assert not before or before in old;changed=old.replace(before,after,1)if before else old+after;assert changed!=old;path.write_text(changed)
                try:callback_evidence.verify(config,graph)
                except AssertionError as error:passed.append({'control':name,'reason':str(error)})
                else:raise AssertionError('section source boundary accepted: '+name)
                path.write_text(old)
            changed=copy.deepcopy(config);next(r for r in changed['stubs']if r['name']=='_output_callback_41eb89')['pending_signature']=False
            try:callback_evidence.verify(changed,graph)
            except AssertionError:passed.append({'control':'unrelated shared noop signature erased','reason':'specific storage view gives no other ABI credit'})
            else:raise AssertionError('global pending signature waiver accepted')
            symbols={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(clone/'build/c2.map').read_text(),re.M)}
            path=clone/'build/c2.exe';old=path.read_bytes();slot=symbols['_output_slot_497ce4'];target=symbols['@select_output_section@4'];pattern=b'\xc7\x05'+struct.pack('<II',slot,target);assert old.count(pattern)==1
            for name,replacement in [('stored canonical target changed',b'\xc7\x05'+struct.pack('<II',slot,target+1)),('stored canonical slot changed',b'\xc7\x05'+struct.pack('<II',slot+4,target))]:
                changed=old.replace(pattern,replacement,1);assert changed!=old;path.write_bytes(changed)
                try:callback_evidence.verify(config,graph)
                except AssertionError as error:passed.append({'control':name,'reason':str(error)})
                else:raise AssertionError('canonical section storage mutation accepted: '+name)
                path.write_bytes(old)
        finally:callback_evidence.BINARY=BINARY
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'scope':'Ten fresh actual raw/formal mutations and ten finite source/metadata/emitted-storage mutations. Source guards supplement common Rust, not CFG discovery. No calls through erased497ce4 authorized.'},indent=2)+'\n')

if __name__=='__main__':main()
