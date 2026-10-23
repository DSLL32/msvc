#!/usr/bin/env python3
"""Semantic rejection controls for ECX arguments and caller-owned callback inputs."""
import copy
import importlib.util
import json
import time
from state import BINARY, sha
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();rejected=[]
def fixture(name):
    p=BINARY/'cases/hello-debug-dll/evidence'/name
    c=json.loads((p/'original/configuration.json').read_text());d=json.loads((p/'original/data.json').read_text());e=(p/'original/events').read_text();n=json.loads((p/'native/state.json').read_text())
    a={'hello.dll':(p/'original/artifact-hello.dll').read_bytes()};v={side:{f.name:f.read_bytes() for f in (p/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
    decoder.compare(c,d,e,n,a,v)
    return c,d,e,n,a,v
register=fixture('0042-map-legacy-index');callback=fixture('0046-insert-public-symbol');modern=fixture('0055-map-type-index');type_list=fixture('0064-module-contains-address')
assert type_list[3]['parameters']==dict(index=0x100c,count=4)
def reject(name,case,mutate,reason=None):
    c,d,e,n,a,v=copy.deepcopy(case);e=mutate(n,e)
    try:decoder.compare(c,d,e,n,a,v)
    except (AssertionError,KeyError) as error:
        if reason is not None:assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(name)
    else:raise AssertionError('accepted '+name)
def state_mutation(function):
    def mutate(n,e):function(n);return e
    return mutate
reject('wrong-native-ECX-argument',register,state_mutation(lambda n:n['parameters'].__setitem__('index',1)))
def wrong_ecx(n,e):
    lines=e.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[i].split();fields[3]='00000001';lines[i]=' '.join(fields);return '\n'.join(lines)+'\n'
reject('wrong-raw-original-ECX-argument',register,wrong_ecx)
reject('modern-native-ECX-argument',modern,state_mutation(lambda n:n['parameters'].__setitem__('index',1)))
reject('modern-raw-original-ECX-argument',modern,wrong_ecx)
reject('modern-actual-formal-width',modern,state_mutation(lambda n:n['observed_argument_types'][0].__setitem__('width',2)))
reject('modern-omitted-actual-formals',modern,state_mutation(lambda n:n.__setitem__('observed_argument_names',[])))
reject('two-register-native-ECX',type_list,state_mutation(lambda n:n['parameters'].update(index=0x100d)),'typed ABI arguments differ')
reject('two-register-native-EDX',type_list,state_mutation(lambda n:n['parameters'].update(count=5)),'typed ABI arguments differ')
reject('two-register-raw-ECX',type_list,wrong_ecx,'typed ABI arguments differ')
def wrong_edx(n,e):
    lines=e.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[i].split();fields[4]='00000005';lines[i]=' '.join(fields);return '\n'.join(lines)+'\n'
reject('two-register-raw-EDX',type_list,wrong_edx,'typed ABI arguments differ')
reject('lost-recursion-depth',type_list,state_mutation(lambda n:n['globals'].update(type_recursion_depth='0000')),'application global differs')
reject('lost-recursion-stack',type_list,state_mutation(lambda n:n['globals'].update(type_recursion_stack='00'*1024)),'application global differs')
def wrong_record_flag(state):
    address=int.from_bytes(bytes.fromhex(state['globals']['type_records']),'little');raw=bytearray.fromhex(state['memory'][str(address)])
    raw[17*32+28]^=0x40;state['memory'][str(address)]=raw.hex()
reject('lost-current-type-visiting-flag',type_list,state_mutation(wrong_record_flag),'initialized type record fields differ')
for label,field,value in [('record-content','bytes','00'*28),('record-capacity','capacity',28),('record-owner','owner','other')]:
    reject(label,callback,state_mutation(lambda n,f=field,v=value:n['parameters']['record'].__setitem__(f,v)))
for name in ('hash','name','segment','offset'):
    reject('wrong-output-width-'+name,callback,state_mutation(lambda n,k=name:n['parameters'][k].__setitem__('width',1)))
reject('wrong-defined-segment-input',callback,state_mutation(lambda n:n['parameters']['segment'].__setitem__('bytes','0100')))
reject('wrong-defined-offset-input',callback,state_mutation(lambda n:n['parameters']['offset'].__setitem__('bytes','01000000')))
reject('wrong-callback-divisor',callback,state_mutation(lambda n:n['parameters'].__setitem__('flags',1)))
report=dict(schema=1,native_only=True,positive_pairs=4,rejected=rejected,decoder_sha256=sha(BINARY/'scripts/raw-state.py'),checker_sha256=sha(BINARY/'scripts/check-abi-state.py'),inputs=[dict(pair=name,original_receipt_sha256=sha(BINARY/'cases/hello-debug-dll/evidence'/name/'original/receipt.json'),native_receipt_sha256=sha(BINARY/'cases/hello-debug-dll/evidence'/name/'native/receipt.json')) for name in ('0042-map-legacy-index','0046-insert-public-symbol','0055-map-type-index','0064-module-contains-address')],seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/abi-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
