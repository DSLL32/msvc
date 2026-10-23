#!/usr/bin/env python3
"""Mutate actual Linux response/state evidence sequentially, restoring every byte."""
import argparse
import copy
import json
from pathlib import Path
import time
from link_provider import verify
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
directory=args.directory.resolve();started=time.perf_counter();passed=[]
verify(directory)
def denied(label,name,change):
    path=directory/name;original=path.read_bytes()
    try:
        value=json.loads(original);change(value);path.write_text(json.dumps(value,indent=2)+'\n')
        try:verify(directory)
        except (AssertionError,KeyError,ValueError,IndexError):passed.append(label)
        else:raise AssertionError('accepted altered native evidence: '+label)
    finally:path.write_bytes(original)
def addressed(value,address):
    changed=0
    for row in value['ranges']:
        raw=bytearray.fromhex(row['data']);offset=address-row['address']
        if 0<=offset<len(raw):raw[offset]^=1;row['data']=raw.hex();changed+=1
    assert changed
for phase in ('link-before','link-after','link-postchild','link-exit'):
    filename='native-'+phase+'-gdb.json'
    for symbol in ('errno','doserrno','last_error'):
        denied(phase+' '+symbol,filename,lambda v,symbol=symbol:addressed(v,v['raw_context']['symbols'][symbol]))
for symbol in ('mb_codepage','updates','environment','initenv','os_environment','active_pass','pass_running','mre_calls'):
    denied('state '+symbol,'native-link-after-gdb.json',lambda v,symbol=symbol:addressed(v,v['raw_context']['symbols'][symbol]))
for key in ('fctrl','initial_pctype','binding_identity_address'):
    denied('context '+key,'native-link-after-gdb.json',lambda v,key=key:v['raw_context'].__setitem__(key,v['raw_context'][key]^1))
for key in ('osfile_address','osfhnd_address','standard_handles_address'):
    denied('lowio '+key,'native-link-after-gdb.json',lambda v,key=key:addressed(v,v['raw_context'][key]+(4 if key!='osfile_address'else 1)))
for field in ('mode','file','argv'):
    denied('spawn formal '+field,'native-link-before-gdb.json',lambda v,field=field:addressed(v,v['raw_context']['spawn_formals'][field]))
for field in ('mode','pass','name','environment','args'):
    denied('caller formal '+field,'native-link-before-gdb.json',lambda v,field=field:addressed(v,v['raw_context']['caller_formals'][field]))
for label,change in [
    ('missing request',lambda v:v['requests'].clear()),
    ('extra request',lambda v:v['requests'].append(copy.deepcopy(v['requests'][0]))),
    ('wire profile',lambda v:v['wire'][0].update(state_profile=False)),
    ('wire bytes',lambda v:v['wire'][0].update(payload_hex=('%02x'%(int(v['wire'][0]['payload_hex'][:2],16)^1))+v['wire'][0]['payload_hex'][2:])),
    ('wrong child result',lambda v:v['requests'][0].update(status=2)),
    ('wrong parent effect',lambda v:v['requests'][0]['parent_state_after'].update(last_error=2)),
    ('changed artifact effect',lambda v:v['effects'][0]['output'].update(sha256='0'*64)),
    ('wrong response identity',lambda v:v.update(response_sha256='0'*64)),
    ('cached as fresh',lambda v:v.update(cached_fixture=True))]:denied(label,'link-native.json',change)
denied('different trial mask','config.json',lambda v:v['accepted'].remove('_write_link_message'))
for key in ('present_before','present_after','result','argument_address'):
    denied('unlink '+key,'native-link-unlink-gdb.json',lambda v,key=key:v.__setitem__(key,not v[key]if type(v[key])is bool else v[key]^1))
# Full FILE words are authoritative alongside raw lowio/standard-handle bytes.
path=directory/'streams-link-after-iob.bin';original=path.read_bytes()
try:
    raw=bytearray(original);raw[16]^=1;path.write_bytes(raw)
    try:verify(directory)
    except AssertionError:passed.append('FILE descriptor word')
    else:raise AssertionError('accepted changed FILE descriptor')
finally:path.write_bytes(original)
# Convenience JSON labels are deliberately not a substitute for addressed data.
path=directory/'native-link-after-gdb.json';original=path.read_bytes()
try:
    value=json.loads(original);value['errors']={'errno':999};path.write_text(json.dumps(value)+'\n');verify(directory)
finally:path.write_bytes(original)
verify(directory)
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed,'convenience_fields_ignored':True}))
