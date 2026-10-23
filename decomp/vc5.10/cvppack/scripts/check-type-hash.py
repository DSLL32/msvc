#!/usr/bin/env python3
"""Prove three byte maps and the observed hash/caller result independently."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import time
from binary import PE
from state import BINARY, ROOT, sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0067-hash-type-record'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']==dict(record=dict(root='type_records',index=12))
source=BINARY/'src/type-hash.c';text=source.read_text();pe=PE(ROOT/config['target']['path'])
assert sha(ROOT/config['target']['path'])==config['target']['sha256']
maps=[]
def verify_columns(columns,table):
    for value in range(256):
        calculated=0
        for bit,column in enumerate(columns):
            if value & (1 << bit):calculated^=column
        assert calculated==table[value],('GF(2) column map differs',value,calculated,table[value])
for name,address in [('previous',0x411178),('current',0x411278),('output',0x411378)]:
    match=re.search(r'static const unsigned char '+name+r'_columns\[8\] = \{([^}]+)\}',text)
    assert match,('missing actual unsigned-byte coefficients',name)
    columns=[int(value.strip(),0) for value in match.group(1).split(',') if value.strip()]
    assert len(columns)==8 and all(0<=value<=255 for value in columns)
    table=pe.read(address,256);verify_columns(columns,table)
    maps.append(dict(name=name,start=address,end=address+256,columns=columns,table=table.hex()))
# Recover complete input through the actual initialized segment and record.
root=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little')
record=bytes.fromhex(native['memory'][str(root)])[12*32:13*32]
pointer=int.from_bytes(record[:4],'little')
segment=int.from_bytes(bytes.fromhex(native['globals']['line_data']),'little')
segment_bytes=bytes.fromhex(native['memory'][str(segment)])
size=int.from_bytes(segment_bytes[pointer-segment:pointer-segment+2],'little')+2
payload=segment_bytes[pointer-segment:pointer-segment+size]
assert payload.hex()=='160001120400000003040000750000007500000012000000'
# This reference uses original lookup bytes, not reconstructed coefficients.
tables=[bytes.fromhex(row['table']) for row in maps];current=previous=0;mix=None;cursor=0;order=[]
while cursor<len(payload):
    group=((len(payload)-cursor-1)&7)+1
    for position in range(group-1,-1,-1):
        order.append(cursor+position);mix=tables[0][previous]^tables[1][current]
        previous=current;current=tables[2][mix^payload[cursor+position]]
    cursor+=group
bucket=((current<<8)|previous)&0xfff
original=decoder.decode(events)
for name,value in [('type_hash_current',current),('type_hash_previous',previous),('type_hash_mix',mix)]:
    row=next(row for row in data if row['name']==name)
    assert original['memory'][row['original']]==bytes([value]),name
    assert native['globals'][name]==bytes([value]).hex(),name
assert original['registers']['ebp']&0xffff==bucket==native['caller_state']['hash_bucket']['value']==2321
assert order==[7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16]
assert list(struct.unpack('<4I',pe.read(0x40e4e4,16)))==[0x40e1c3,0x40e1c3,0x40e1c8,0x40e1f2]
assert list(struct.unpack('<8I',pe.read(0x40e4f4,32)))==[0x40e468,0x40e419,0x40e3ca,0x40e37b,0x40e32c,0x40e2dd,0x40e28e,0x40e23f]
rejected=[]
def reject(name,mutation,reason,raw=events):
    changed=copy.deepcopy(native);mutation(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for row in maps:
    columns=list(row['columns']);columns[3]^=1
    try:verify_columns(columns,bytes.fromhex(row['table']))
    except AssertionError as error:rejected.append(dict(name='altered-'+row['name']+'-coefficient',diagnostic=str(error)))
    else:raise AssertionError('accepted altered coefficient')
for name in ('type_hash_current','type_hash_previous','type_hash_mix'):
    reject('wrong-'+name,lambda s,n=name:s['globals'].update({n:'00'}),'application global differs')
for field,value in [('value',2320),('width',4),('native_type','unsigned int'),('owner','other'),('local','other')]:
    reject('wrong-caller-'+field,lambda s,f=field,v=value:s['caller_state']['hash_bucket'].update({f:v}),'typed caller state differs')
reject('missing-caller-observation',lambda s:s.pop('caller_state'),'typed caller state differs')
lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[i].split();fields[7]='00000910';lines[i]=' '.join(fields)
reject('wrong-original-return-bucket',lambda s:None,'typed caller state differs','\n'.join(lines)+'\n')
actual=[]
for field,value in [('width',4),('native_type','unsigned int')]:
    with tempfile.TemporaryDirectory(prefix='cvpack-hash-caller-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config)
        row=next(row for row in changed['functions'] if row['name']=='allocate_global_type');row['caller_state'][0][field]=value
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
            '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],
            cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
        assert result.returncode and 'actual caller-local type/extent differs' in diagnostic,diagnostic
        actual.append(dict(name='actual-DWARF-caller-'+field,returncode=result.returncode,diagnostic=diagnostic,
            bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py')))
report=dict(schema=1,native_only=True,lookup_values_proved=768,maps=maps,payload=payload.hex(),byte_visit_order=order,
    hash_globals=dict(current=current,previous=previous,mix=mix),returned_bucket=bucket,
    executed_scope='24-byte generic arglist, three eight-byte groups; other kind/partial-group branches are source/static proof, not executed credit.',
    count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    source_sha256=sha(source),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'type-hash-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(values=768,rejected=report['count'],bucket=bucket,seconds=report['seconds']),indent=2))
