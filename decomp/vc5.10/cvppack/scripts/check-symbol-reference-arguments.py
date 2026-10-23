#!/usr/bin/env python3
"""Owned rewritten record slices and actual bucket/scalar reference ABI."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import struct
import tempfile
import time
from binary import PE
from state import BINARY,ROOT,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0089-record-symbol-references'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);parameters=baseline['parameters'];assert parameters['record']['offset']==108 and len(bytes.fromhex(parameters['record']['bytes']))==56
assert parameters['buckets']==dict(null=False,global_name='type_table_local') and parameters['module']==2 and parameters['kind']==0x400
original=decoder.decode(events);rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for field,value in [('offset',109),('capacity',8704),('owner','scan_module_symbols'),('bytes','ff'+parameters['record']['bytes'][2:])]:
    reject('wrong-record-'+field,lambda s,f=field,v=value:s['parameters']['record'].update({f:v}),'typed ABI arguments differ')
reject('wrong-bucket-table',lambda s:s['parameters']['buckets'].update(global_name='type_table_forward'),'typed ABI arguments differ')
reject('wrong-module',lambda s:s['parameters'].update(module=3),'typed ABI arguments differ')
reject('wrong-kind',lambda s:s['parameters'].update(kind=0x401),'typed ABI arguments differ')
reject('wrong-pointer-formal-pointee',lambda s:s['observed_argument_types'][1].update(pointee_width=1),'actual frontier pointee extent differs')
def stack(changes):
    address=original['esp'];match=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M))[-1];raw=bytearray.fromhex(match[1])
    for slot,value in changes.items():raw[slot*4:slot*4+4]=value.to_bytes(4,'little')
    return events[:match.start(1)]+raw.hex()+events[match.end(1):]
reject('original-interior-non-record',lambda s:None,'original reference record offset/owner differs',stack({1:original['stack'][1]+1}))
reject('original-record-in-unwritten-tail',lambda s:None,'original reference record offset/owner differs',stack({1:original['stack'][1]+1000,4:1108}))
compare(native,stack({3:original['stack'][3]^0x12340000,5:original['stack'][5]^0xabcd0000}))
actual=[]
for name in ['bucket-address-versus-value','record-offset-alias','omitted-formals']:
    with tempfile.TemporaryDirectory(prefix='cvpack-symbol-reference-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='record_symbol_reference')
        if name=='bucket-address-versus-value':row['parameters'][1]['global_value']=False;reason='argument is not the specified global address'
        elif name=='record-offset-alias':row['parameters'][0]['offset_name']='module';reason='reference record offset/owner differs'
        else:row['parameters']=[];reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
provider=PE(ROOT/config['target']['path']);witness={0x40f473:'668b54241c',0x40f47f:'6689501c',0x40f45d:'668b542424',0x40f46b:'66895012'}
for address,hexbytes in witness.items():assert provider.read(address,len(bytes.fromhex(hexbytes))).hex()==hexbytes
dispatch=struct.unpack('<8I',provider.read(0x40f5c4,32));assert dispatch==(0x40f555,0x40f51d,0x40f5a5,0x40f5a5,0x40f574,0x40f574,0x40f555,0x40f51d)
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,parameters=parameters,module_kind_high_word_dead_positive=True,original_word_witnesses={hex(k):v for k,v in witness.items()},original_dispatch=dict(start=0x40f5c4,end=0x40f5e4,cases=list(range(0x100a,0x1012)),targets=dispatch),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),seconds=time.perf_counter()-start)
(pair/'symbol-reference-argument-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
