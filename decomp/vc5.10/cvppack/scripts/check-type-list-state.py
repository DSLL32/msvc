#!/usr/bin/env python3
"""Owned type-record pointer ABI, list recursion and fastcall mapper data."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha
from binary import PE

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0065-compact-type-list'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']==dict(record=dict(root='type_records',index=12))
table=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little')
records=bytes.fromhex(native['memory'][str(table)])
assert int.from_bytes(records[12*32+28:12*32+30],'little')==0x444
assert int.from_bytes(records[17*32+28:17*32+30],'little')==0x440
assert native['globals']['type_recursion_depth']=='0100'
assert bytes.fromhex(native['globals']['type_recursion_stack'])[:8]==struct.pack('<II',0x1011,0x100c)
rejected=[]
def reject(name,mutation,reason,raw=events):
    changed=copy.deepcopy(native);mutation(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('wrong-record-index',lambda s:s['parameters']['record'].update(index=13),'typed ABI arguments differ')
reject('wrong-record-owner',lambda s:s['parameters']['record'].update(root='other'),'typed ABI arguments differ')
reject('wrong-real-record-pointee',lambda s:s['observed_argument_types'][0].update(pointee_width=28),'actual frontier pointee extent differs')
reject('lost-list-depth-pop',lambda s:s['globals'].update(type_recursion_depth='0200'),'application global differs')
reject('lost-retained-stack-entry',lambda s:s['globals'].update(type_recursion_stack=(struct.pack('<II',0x1011,0)+bytes(1016)).hex()),'application global differs')
def flag(state,offset):
    raw=bytearray.fromhex(state['memory'][str(table)]);raw[offset]^=1;state['memory'][str(table)]=raw.hex()
reject('wrong-list-reference-count',lambda s:flag(s,12*32+12),'initialized type record fields differ')
reject('wrong-list-state-flags',lambda s:flag(s,12*32+28),'initialized type record fields differ')
def wrong_mapper(state):
    raw=bytearray.fromhex(state['globals']['type_index_mappers'])
    raw[8:12]=state['function_addresses']['compact_legacy_index'].to_bytes(4,'little')
    state['globals']['type_index_mappers']=raw.hex()
reject('wrong-native-mapper-identity',wrong_mapper,'symbol callback logical identity differs')
original=decoder.decode(events);lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '))
words=lines[i].split();address=int(words[3],16)
for label,value,reason in [('wrong-original-record-slot',address+32,'typed ABI arguments differ'),('misaligned-original-record',address+1,'argument is not an owned original local type record')]:
    changed=list(lines);fields=list(words);fields[3]='%08x'%value;changed[i]=' '.join(fields)
    reject(label,lambda s:None,reason,'\n'.join(changed)+'\n')

spec=importlib.util.spec_from_file_location('switch_data',BINARY/'scripts/switch-data.py')
tables=importlib.util.module_from_spec(spec);spec.loader.exec_module(tables)
proof=tables.function_tables(config['functions']);mapper=next(row for row in proof if row['name']=='type_index_mappers')
assert [row['function'] for row in mapper['rows']]==['compact_legacy_index','compact_type_index','compact_type_index']
pe=PE(BINARY/'build/CVPACK.EXE');changed=copy.deepcopy(pe);raw=bytearray(pe.data)
offset=pe.offset(mapper['rebuilt']);raw[offset+8:offset+12]=raw[offset:offset+4];changed.data=bytes(raw)
try:tables.function_tables(config['functions'],rebuilt=changed)
except AssertionError as error:
    assert 'symbol rewrite table callback identity differs' in str(error),str(error)
    rejected.append(dict(name='wrong-linked-PE-mapper-identity',diagnostic=str(error)))
else:raise AssertionError('accepted bad linked mapper')
with tempfile.TemporaryDirectory(prefix='cvpack-type-record-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config)
    row=next(row for row in changed['functions'] if row['name']=='intern_type');row['parameters'][0]['native_pointee_width']=28
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
        '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],
        cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
    assert result.returncode and 'actual pointer formal pointee extent differs' in diagnostic,diagnostic
    actual=dict(returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py'))
report=dict(schema=1,native_only=True,count=len(rejected)+1,rejected=rejected,actual_gdb_type_record_rejection=actual,
    actual_formals=native['observed_argument_types'],list_record=12,parent_record=17,complete_type_records=336,mapper_rows=mapper,
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),table_validator_sha256=sha(BINARY/'scripts/switch-data.py'),seconds=time.perf_counter()-start)
(pair/'type-list-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
