#!/usr/bin/env python3
"""Affected callback identity/escape controls against real immutable evidence."""
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
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();begin=time.monotonic();positive=compare(a.original,a.native);rejected=[]
    def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n')
    def native(root,change):
        path=root/'n/state.json';state=json.loads(path.read_text());change(state['output_setup']);write(path,state)
        path=root/'n/gdb.stdout';raw=re.sub(r'^C2-OUTPUT-SETUP .*$',lambda m:'C2-OUTPUT-SETUP '+json.dumps(state['output_setup'],sort_keys=True),path.read_text(),flags=re.M);path.write_text(raw)
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['state_sha256']=sha((root/'n/state.json').read_bytes());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def original(root,address,value):
        path=root/'o/unmodeled-original-data.bin';data=bytearray(path.read_bytes());struct.pack_into('<I',data,address-0x48e000,value);path.write_bytes(data)
        path=root/'o/receipt.json';receipt=json.loads(path.read_text());receipt['globals_complete_hashes']['unmodeled-original-data.bin']=sha(data);receipt['unmodeled_original_data']['sha256']=sha(data);write(path,receipt)
    def missing_observation(root):
        path=root/'n/gdb.stdout';path.write_text(re.sub(r'^C2-OUTPUT-SETUP .*\n','',path.read_text(),flags=re.M))
        path=root/'n/receipt.json';receipt=json.loads(path.read_text());receipt['gdb_outputs']['gdb.stdout']=sha((root/'n/gdb.stdout').read_bytes());write(path,receipt)
    def static_receipt(root):
        path=root/'o/callback-evidence.json';value=json.loads(path.read_text());value['slots'][0]['rebuilt']+=4;write(path,value)
        path=root/'o/audit-receipt.json';value=json.loads(path.read_text());value['callback_evidence_sha256']=sha((root/'o/callback-evidence.json').read_bytes());write(path,value)
        path=root/'o/receipt.json';value=json.loads(path.read_text());value['audit_receipt_sha256']=sha((root/'o/audit-receipt.json').read_bytes());write(path,value)
    controls=[('missing callback slot',lambda root:native(root,lambda state:state['slots'].pop())),
        ('wrong native slot identity',lambda root:native(root,lambda state:state['slots'][3].__setitem__('address',state['slots'][3]['address']+4))),
        ('native callback aliases wrong valid target',lambda root:native(root,lambda state:state['slots'][3].__setitem__('target',state['slots'][0]['target']))),
        ('native callback group changes',lambda root:native(root,lambda state:state.__setitem__('group_name','DGROUP'))),
        ('native callback selection option changes',lambda root:native(root,lambda state:state.__setitem__('enabled',0))),
        ('original callback changed to allowed but wrong alternative',lambda root:original(root,0x497dbc,0x41eb89)),
        ('original callback group identity changes',lambda root:original(root,0x49973c,0x49d4ec)),
        ('missing actual callback GDB observation',missing_observation),
        ('changed slot ownership evidence with rebound hashes',static_receipt)]
    with tempfile.TemporaryDirectory(prefix='callback-controls-',dir=ROOT.parent/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        folder=Path(folder)
        for i,(name,change)in enumerate(controls):
            root=folder/str(i);shutil.copytree(a.original,root/'o');shutil.copytree(a.native,root/'n');change(root)
            try:compare(root/'o',root/'n')
            except (AssertionError,KeyError,ValueError)as error:rejected.append({'control':name,'reason':str(error)})
            else:raise AssertionError('callback mutation accepted: '+name)
        clone=folder/'model';shutil.copytree(BINARY/'src',clone/'src');(clone/'build').mkdir();(clone/'cases/analysis').mkdir(parents=True)
        for name in ('c2.exe','c2.map'):shutil.copyfile(BINARY/'build'/name,clone/'build'/name)
        boundary=(BINARY/'cases/analysis/output-callback-boundary.json').read_bytes();(clone/'cases/analysis/output-callback-boundary.json').write_bytes(boundary)
        config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text())
        callback_evidence.BINARY=clone
        assert callback_evidence.verify(config,graph)
        for name,snippet in [('pending member invocation','void bad(void) { output_slot_497dbc.target(); }'),('pending slot escape','void bad(void) { void *p = &output_slot_497dbc; }'),('pending target direct invocation','void bad(void) { output_callback_4346cf(); }')]:
            path=clone/'src/main.c';before=path.read_bytes();path.write_bytes(before+b'\n'+snippet.encode()+b'\n')
            try:callback_evidence.verify(config,graph)
            except AssertionError as error:rejected.append({'control':name,'reason':str(error)})
            else:raise AssertionError('source escape accepted')
            path.write_bytes(before)
        changed=copy.deepcopy(config);changed['stubs'][next(i for i,r in enumerate(changed['stubs'])if r.get('pending_signature'))]['pending_signature']=False
        try:callback_evidence.verify(changed,graph)
        except AssertionError:rejected.append({'control':'erased unresolved signature marker','reason':'pending signature guard'})
        else:raise AssertionError('pending signature removal accepted')
        callback_evidence.BINARY=BINARY
    value={'positive':positive,'count':len(rejected),'rejected':rejected,'seconds':time.monotonic()-begin,
           'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'native_receipt_sha256':sha((a.native/'receipt.json').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())}
    a.output.write_text(json.dumps(value,indent=2)+'\n');print(json.dumps(value,indent=2))

if __name__=='__main__':main()
