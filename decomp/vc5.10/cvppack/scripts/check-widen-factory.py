#!/usr/bin/env python3
"""Fresh factory ABI/opaque identity, raw writes and complete next-query inputs."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
from binary import COFF,PE
from state import BINARY,ROOT,sha

begin=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0092-create-widen-named-import'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);original=decoder.decode(events);factory=original['widen_factory_calls'][0]
assert len(original['widen_factory_calls'])==len(native['widen_factory_calls'])==1
assert baseline['parameters']['input']['size']==baseline['parameters']['size']==57 and baseline['parameters']['output']['width']==16
contract=json.loads((BINARY/'cases/hello-debug-dll/widen-contract.json').read_text());assert baseline['parameters']['input']['bytes']==contract['instances'][0]['query']['input']
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for field,value in [('capacity',257),('flags',1),('result',0)]:
    reject('wrong-actual-factory-'+field,lambda s,f=field,v=value:s['widen_factory_calls'][0].update({f:v}),'factory actual arguments/result differ')
for field in ['entry_last_error','exit_last_error']:
    reject('wrong-factory-'+field,lambda s,f=field:s['widen_factory_calls'][0].update({f:7}),'factory actual LastError differs')
reject('wrong-native-vtable-callback',lambda s:s['widen_factory_calls'][0]['vtable_slots'].__setitem__(0,0),'factory native typed method identities differ')
reject('wrong-native-object-output-alias',lambda s:s['widen_object_arguments']['object'].update(address=s['widen_object_arguments']['object']['address']+4),'opaque native factory output/argument alias differs')
reject('wrong-native-caller-output-slot',lambda s:s['widen_object_arguments']['object'].update(output_slot=s['widen_object_arguments']['object']['output_slot']+4),'opaque native factory output/argument alias differs')
reject('wrong-query-input-byte',lambda s:s['parameters']['input'].update(bytes='00'+s['parameters']['input']['bytes'][2:]),'typed ABI arguments differ')
reject('wrong-query-descriptor-initialization',lambda s:s['parameters']['output'].update(bytes='01'+s['parameters']['output']['bytes'][2:]),'typed ABI arguments differ')
reject('wrong-query-input-size',lambda s:s['parameters'].update(size=56),'typed ABI arguments differ')
# Mutate the raw return observation itself, preserving all other raw observations.
index=events.index('widen create-return ');match=re.search(r'^memory %08x ([0-9a-f]+)$'%factory['table'],events[index:],re.M);assert match
start,end=index+match.start(1),index+match.end(1);raw=bytearray.fromhex(match[1]);raw[0]^=4
reject('wrong-original-vtable-target',lambda s:None,'factory original method identities differ',events[:start]+raw.hex()+events[end:])
match=re.search(r'^memory %08x ([0-9a-f]+)$'%factory['output_argument'],events[index:],re.M);assert match
start,end=index+match.start(1),index+match.end(1);raw=bytearray.fromhex(match[1]);raw[0]^=4
reject('wrong-original-pointer-write',lambda s:None,'factory actual output pointer write differs',events[:start]+raw.hex()+events[end:])
reject('wrong-original-factory-scalar',lambda s:None,'factory actual arguments/result differ',events.replace('widen create-call %08x 256 0'%factory['output_argument'],'widen create-call %08x 257 0'%factory['output_argument']))
actual=[]
for name in ['descriptor-four-byte-schema','input-without-header-offset','omitted-formals']:
    with tempfile.TemporaryDirectory(prefix='cvpack-query-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='query_widen')
        if name=='descriptor-four-byte-schema':row['parameters'][1]['width']=4;reason='actual pointer formal pointee extent differs'
        elif name=='input-without-header-offset':row['parameters'][2]['offset']=0;reason='buffer argument does not alias specified global'
        else:row['parameters']=[];reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
# Refresh the receipt bindings as an adversary would; reject actual module bytes,
# rather than merely an outdated listed hash. Never alter the owned original.
spec=importlib.util.spec_from_file_location('acceptance',BINARY/'scripts/accept.py');acceptance=importlib.util.module_from_spec(spec);spec.loader.exec_module(acceptance)
with tempfile.TemporaryDirectory(prefix='cvpack-provider-bytes-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary)
    for side in ['original','native']:
        (directory/side).mkdir()
        for source in (pair/side).iterdir():
            if source.is_file():(directory/side/source.name).symlink_to(source.resolve())
    fake=directory/'wrong-provider.bin';raw=bytearray((BINARY/'cases/hello-debug-dll/providers/loaded-mspdb50.bin').read_bytes());raw[200]^=1;fake.write_bytes(raw)
    receipt=json.loads((pair/'original/receipt.json').read_text());receipt['actual_loaded_mspdb']['path']=str(fake)
    (directory/'original/receipt.json').unlink();(directory/'original/receipt.json').write_text(json.dumps(receipt))
    receipt=json.loads((pair/'native/receipt.json').read_text());receipt['original_receipt_sha256']=sha(directory/'original/receipt.json')
    (directory/'native/receipt.json').unlink();(directory/'native/receipt.json').write_text(json.dumps(receipt))
    try:acceptance.validate_pair(BINARY/'functions.json',directory/'original',directory/'native')
    except AssertionError as error:
        assert 'actual loaded MSPDB provider bytes differ' in str(error),str(error);rejected.append(dict(name='wrong-actual-provider-bytes-with-refreshed-receipts',diagnostic=str(error)))
    else:raise AssertionError('accepted wrong actual loaded-provider bytes')
original_code=PE(ROOT/config['target']['path']).read(0x410290,22);assert original_code.hex()=='8b4424046a00680001000050ff154c62410083c40cc3'
obj=COFF((BINARY/'build/widen.obj').read_bytes());assert [r['name'] for r in obj.symbols.values() if r['section']>0 and r.get('type')==0x20]==['_create_widen']
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,actual_factory_calls=1,query_not_yet_called=True,original_wrapper_bytes=original_code.hex(),original_ret_cleanup=0,qualified_import=json.loads((pair/'original/audit.json').read_text())['graph']['functions']['_create_widen'],new_cpp_target_vertices=['_create_widen'],source_sha256=sha(BINARY/'src/widen.cpp'),host_binding_sha256=sha(BINARY/'scripts/native-widen-factory.cpp'),provider_source_sha256=sha(BINARY/'scripts/native-widen.c'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),seconds=time.perf_counter()-begin)
(pair/'factory-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
