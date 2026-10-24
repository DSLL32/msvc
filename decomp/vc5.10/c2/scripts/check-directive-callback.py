#!/usr/bin/env python3
"""Finite actual directive-call ABI and remaining stores-only boundary controls."""
import argparse
import copy
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time
from compare import compare
from state import BINARY, ROOT, sha
import callback_evidence


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','validation','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();begin=time.monotonic();positive=compare(a.original,a.native,a.validation);rejected=[]
    def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def native(root,field,change):
        path=root/'n/state.json';value=json.loads(path.read_text());change(value[field]);write(path,value)
        path=root/'n/gdb.stdout';raw=path.read_text()
        if field=='directive_callback':raw=re.sub(r'^C2-DIRECTIVE-CALLBACK .*$',lambda m:'C2-DIRECTIVE-CALLBACK '+json.dumps(value[field],sort_keys=True),raw,flags=re.M)
        elif field=='directive_callback_abi':
            rows=iter(value[field]);raw=re.sub(r'^C2-DIRECTIVE-ABI .*$',lambda m:'C2-DIRECTIVE-ABI '+json.dumps(next(rows),sort_keys=True),raw,flags=re.M)
        path.write_text(raw)
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((root/'n/state.json').read_bytes());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def original(root,pattern,replacement):
        path=root/'o/events.txt';before=path.read_text();after=re.sub(pattern,replacement,before,flags=re.M);assert before!=after;path.write_text(after)
        path=root/'o/receipt.json';receipt=json.loads(path.read_text());receipt['events_sha256']=sha((root/'o/events.txt').read_bytes());write(path,receipt)
    controls=[('actual directive formal omitted',lambda r:native(r,'directive_callback',lambda v:v.__setitem__('formals',[]))),
        ('actual directive kind differs',lambda r:native(r,'directive_callback',lambda v:v.__setitem__('kind',6))),
        ('actual directive value differs',lambda r:native(r,'directive_callback',lambda v:v.__setitem__('value','WRONG'))),
        ('next owned directive differs',lambda r:native(r,'directive_callback',lambda v:v.__setitem__('next_offset',0))),
        ('actual typed slot identity differs',lambda r:native(r,'directive_callback',lambda v:v.__setitem__('slot_target_alias',False))),
        ('native incoming ECX differs',lambda r:native(r,'directive_callback_abi',lambda v:v[1].__setitem__('ecx',v[1]['ecx']+4))),
        ('native callback return address differs',lambda r:native(r,'directive_callback_abi',lambda v:v[1]['stack'].__setitem__(0,v[1]['stack'][0]+1))),
        ('native callback ESP differs',lambda r:native(r,'directive_callback_abi',lambda v:v[1].__setitem__('esp',v[1]['esp']-4))),
        ('native callback ESI not preserved',lambda r:native(r,'directive_callback_abi',lambda v:v[1].__setitem__('esi',v[1]['esi']+4))),
        ('native callback slot target differs',lambda r:native(r,'directive_callback_abi',lambda v:v[0].__setitem__('slot_target',v[0]['slot_target']+4))),
        ('native callback exact entry PC differs',lambda r:native(r,'directive_callback_abi',lambda v:v[1].__setitem__('pc',v[1]['pc']+1))),
        ('original incoming ECX differs',lambda r:original(r,r'^(directive-call-abi at=00453e02 ecx=)[0-9a-f]+',r'\g<1>00000000')),
        ('original incoming EBX not preserved',lambda r:original(r,r'^(directive-call-abi at=00453e02 ecx=[0-9a-f]+ ebx=)[0-9a-f]+',r'\g<1>00000000')),
        ('original return address differs',lambda r:original(r,r'^observe-stack 00453dfa ',r'observe-stack 00453df9 '))]
    with tempfile.TemporaryDirectory(prefix='directive-callback-controls-',dir=ROOT.parent/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder)
        for index,(name,change)in enumerate(controls):
            root=folder/str(index);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');shutil.copytree(a.validation.parent,root/'v');change(root)
            path=root/'v/receipt.json';receipt=json.loads(path.read_text());receipt['original_execution_receipt_sha256']=sha((root/'o/receipt.json').read_bytes());write(path,receipt)
            try:compare(root/'o',root/'n',path)
            except (AssertionError,KeyError,ValueError)as error:rejected.append({'control':name,'reason':str(error)})
            else:raise AssertionError('directive ABI mutation accepted: '+name)
        clone=folder/'static';shutil.copytree(BINARY/'src',clone/'src');(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True)
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        for name in ('output-callback-boundary.json','directive-callback-contract.json'):shutil.copyfile(BINARY/'cases/analysis'/name,clone/'cases/analysis'/name)
        config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.validation.parent/'graph-audit.json').read_text());callback_evidence.BINARY=clone
        try:
            assert callback_evidence.verify(config,graph)
            for name,source,before,after in [
                ('unreviewed other member invocation','src/main.c','','\nvoid bad(void) { output_slot_497de4.target(); }\n'),
                ('typed slot unknown escape','src/main.c','','\nvoid bad(void) { void *p = &output_slot_49971c; }\n'),
                ('typed target direct escape','src/main.c','','\nvoid bad(void) { output_callback_453e02(0); }\n'),
                ('cast generalized to other slot','src/output-callbacks.c','output_slot_497de4.target = output_callback_41eb89;','output_slot_497de4.target = (C2OutputDirectiveWriter) output_callback_41eb89;'),
                ('specific noarg cast omitted','src/output-callbacks.c','output_slot_49971c.target = (C2OutputDirectiveWriter) output_callback_41eb89;','output_slot_49971c.target = output_callback_41eb89;'),
                ('consumer ownership changed','src/output-callbacks.c','void write_output_directives(void)','void wrong_consumer(void)'),
                ('typed callback convention changed','src/output-callbacks.h','typedef void (__fastcall *C2OutputDirectiveWriter)','typedef void (__cdecl *C2OutputDirectiveWriter)')]:
                path=clone/source;old=path.read_text();assert not before or before in old;new=old.replace(before,after,1)if before else old+after;assert new!=old;path.write_text(new)
                try:callback_evidence.verify(config,graph)
                except AssertionError as error:rejected.append({'control':name,'reason':str(error)})
                else:raise AssertionError('source boundary mutation accepted: '+name)
                path.write_text(old)
            changed=copy.deepcopy(config);next(r for r in changed['stubs']if r['name']=='_output_callback_41eb89')['pending_signature']=False
            try:callback_evidence.verify(changed,graph)
            except AssertionError:rejected.append({'control':'shared noop global ABI marker erased','reason':'only one typed slot view permitted'})
            else:raise AssertionError('shared noop ABI marker accepted')
            path=clone/'build/c2.exe';old=path.read_bytes();maps=(clone/'build/c2.map').read_text();slot=int(re.search(r'\s+_output_slot_49971c\s+([0-9a-f]{8})',maps)[1],16);pattern=b'\x8b\x4d\xfc\xff\x15'+struct.pack('<I',slot);assert old.count(pattern)==1;path.write_bytes(old.replace(pattern,pattern[:5]+struct.pack('<I',slot+4),1))
            try:callback_evidence.verify(config,graph)
            except AssertionError:rejected.append({'control':'canonical call uses neighboring slot','reason':'exact emitted qualified slot binding'})
            else:raise AssertionError('canonical wrong slot accepted')
        finally:callback_evidence.BINARY=BINARY
    a.output.write_text(json.dumps({'positive':positive,'count':len(rejected),'rejected':rejected,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'validation_sha256':sha(a.validation.read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
