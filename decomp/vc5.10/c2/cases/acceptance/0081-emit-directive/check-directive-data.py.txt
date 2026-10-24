#!/usr/bin/env python3
"""Mutate the actual compiled switch operands and finite callable namespace."""
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
import directive_data
from state import BINARY, ROOT, sha
from data_evidence import reader


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();begin=time.monotonic();passed=[]
    config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text())
    with tempfile.TemporaryDirectory(prefix='directive-data-controls-',dir=ROOT.parent/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        clone=Path(folder);(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True);shutil.copytree(BINARY/'src',clone/'src')
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        for name in ('output-callback-boundary.json','directive-callback-contract.json'):shutil.copyfile(BINARY/'cases/analysis'/name,clone/'cases/analysis'/name)
        original_binary=(clone/'build/c2.exe').read_bytes();mp=(clone/'build/c2.map').read_text();symbols={r[1]:int(r[2],16)for r in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',mp,re.M)}
        base=symbols['@emit_output_directive@4'];rd=reader(clone/'build/c2.exe');body=rd(base,234);assert original_binary.count(body)==1;file_start=original_binary.index(body)
        assert directive_data.verify(clone,config,graph)
        def replace(offset,data):
            changed=bytearray(original_binary);changed[file_start+offset:file_start+offset+len(data)]=data;return bytes(changed)
        cases=[('table case3 redirected to existing case4',194+2*4,struct.pack('<I',base+84)),
            ('table default hole redirected to existing case1',194+4,struct.pack('<I',base+47)),
            ('case3 quoted flag silently cleared',62+1,b'\0'),
            ('case4 quoted flag silently set',84+1,b'\1'),
            ('case3 payload read shifted to kind',62+9,b'\4'),
            ('case4 prefix changed to valid nodefaultlib string',84+11,rd(base+131+11,4)),
            ('case7 writes neighboring object',119+2,struct.pack('<I',symbols['_coff_directive_option']+4)),
            ('default source-line argument changed',175+1,struct.pack('<I',1515)),
            ('named write child changed to real string child',62+16,struct.pack('<i',symbols['@emit_incremental_string@4']-(base+62+20)))]
        path=clone/'build/c2.exe'
        for name,offset,data in cases:
            mutated=replace(offset,data);assert mutated!=original_binary;path.write_bytes(mutated)
            try:directive_data.verify(clone,config,graph)
            except AssertionError as error:passed.append({'control':name,'reason':str(error)})
            else:raise AssertionError('switch operand mutation accepted: '+name)
            path.write_bytes(original_binary)
        callback_evidence.BINARY=clone
        try:
            assert callback_evidence.verify(config,graph)
            for name,snippet in [('new typed target direct escape','void bad(void) { emit_output_directive(0); }'),('other pending slot member invoked','void bad(void) { output_slot_497de4.target(); }')]:
                path=clone/'src/main.c';before=path.read_bytes();path.write_bytes(before+b'\n'+snippet.encode()+b'\n')
                try:callback_evidence.verify(config,graph)
                except AssertionError as error:passed.append({'control':name,'reason':str(error)})
                else:raise AssertionError('pending source boundary accepted: '+name)
                path.write_bytes(before)
            changed=copy.deepcopy(config);r=next(r for r in changed['stubs']if r['name']=='_output_callback_41eb89');r['pending_signature']=False
            try:callback_evidence.verify(changed,graph)
            except AssertionError:passed.append({'control':'unrelated shared noop globally accepted ABI','reason':'only selected target body qualified'})
            else:raise AssertionError('global noop ABI waiver accepted')
        finally:callback_evidence.BINARY=BINARY
    a.output.write_text(json.dumps({'status':'PASS','count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'data_checker_sha256':sha((BINARY/'scripts/directive_data.py').read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'rebuilt_sha256':sha((BINARY/'build/c2.exe').read_bytes()),'scope':'All modified cases change actual targeted bytes/source/metadata; common Rust graphs deliberately not rewritten or replaced. Original toolchain files untouched.'},indent=2)+'\n')

if __name__=='__main__':main()
