#!/usr/bin/env python3
"""Actual query thiscall stack/state and next-emit owned descriptor controls."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from binary import COFF,PE
from state import BINARY,ROOT,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0093-query-widen-typed-emit'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);original=decoder.decode(events);query=original['widen_query_calls'][0]
assert len(native['widen_query_calls'])==1 and baseline['parameters']['output']['description']==dict(bytes=60,fixups=2,output='pack_buffer',records='pack_fixups')
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for field,value in [('flags',1),('result',0)]:reject('wrong-query-'+field,lambda s,f=field,v=value:s['widen_query_calls'][0].update({f:v}),'query actual flags/result differ')
reject('wrong-query-object',lambda s:s['widen_query_calls'][0].update(object=0),'query factory/object alias differs')
reject('wrong-query-input-owner',lambda s:s['widen_query_calls'][0].update(input_address=0),'query owned input buffer/offset alias differs')
reject('wrong-query-complete-input',lambda s:s['widen_query_calls'][0].update(input='00'+s['widen_query_calls'][0]['input'][2:]),'query complete initialized input differs')
reject('query-modified-input',lambda s:s['widen_query_calls'][0].update(input_after='00'+s['widen_query_calls'][0]['input_after'][2:]),'query changed initialized input')
reject('query-nonzero-description-input',lambda s:s['widen_query_calls'][0].update(description_before='01'+'00'*15),'query initial descriptor differs')
reject('query-wrong-output-capacity',lambda s:s['widen_query_calls'][0].update(description_after='3d'+s['widen_query_calls'][0]['description_after'][2:]),'query full output descriptor differs')
reject('wrong-emit-buffer-pointer-role',lambda s:s['parameters']['output']['description'].update(records='pack_buffer'),'typed ABI arguments differ')
reject('wrong-emit-flags',lambda s:s['parameters'].update(flags=1),'typed ABI arguments differ')
# Independently alter recorded register state, without altering decoded method fields.
def probe_registers(label,index,value):
    begin=events.index(label);end=events.index('widen method-',begin);match=re.search(r'^registers (.+)$',events[begin:end],re.M);assert match
    words=match[1].split();words[index]='%08x'%value;a,b=begin+match.start(1),begin+match.end(1);return events[:a]+' '.join(words)+events[b:]
reject('original-wrong-this-register',lambda s:None,'query actual ECX/stack ABI differs',probe_registers('widen-query-call 004102f7 ',2,query['object']+4))
reject('original-wrong-callee-stack-cleanup',lambda s:None,'query actual callee cleanup/result ABI differs',probe_registers('widen-query-return 004102fa ',-1,query['exit_esp']+4))
begin=events.index('widen method-return 32 ');match=re.search(r'^memory %08x ([0-9a-f]+)$'%query['input_address'],events[begin:],re.M);assert match
left,right=begin+match.start(1),begin+match.end(1);raw=bytearray.fromhex(match[1]);raw[0]^=1
reject('original-query-modified-input',lambda s:None,'query changed initialized input',events[:left]+raw.hex()+events[right:])
# Query initializes descriptor sizes only; each output allocation is still wholly
# native-owned/unwritten before emit. No original padding values are installed.
changed=copy.deepcopy(native)
for name,pattern in [('pack_buffer','a5'),('pack_fixups','c7')]:
    address=int.from_bytes(bytes.fromhex(changed['globals'][name]),'little')
    count=int.from_bytes(bytes.fromhex(changed['globals']['pack_buffer_size' if name=='pack_buffer' else 'pack_fixup_count']),'little')*(1 if name=='pack_buffer' else 8)
    changed['memory'][str(address)]=pattern*count
compare(changed)
actual=[]
for name in ['descriptor-wrong-width','omitted-flag-formal']:
    with tempfile.TemporaryDirectory(prefix='cvpack-emit-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='emit_widen')
        if name=='descriptor-wrong-width':row['parameters'][1]['width']=4;reason='actual pointer formal pointee extent differs'
        else:row['parameters'].pop();reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
obj=COFF((BINARY/'build/widen.obj').read_bytes());symbols={r['name']:r for r in obj.symbols.values()};row=symbols['_query_widen'];body=obj.sections[row['section']-1]['data'][row['value']:]
probe=COFF((BINARY/'cases/hello-debug-dll/abi/original-caller.obj').read_bytes());symbols={r['name']:r for r in probe.symbols.values()};begin=symbols['_probe_query'];end=symbols['_probe_emit'];independent=probe.sections[begin['section']-1]['data'][begin['value']:end['value']]
assert body==independent and body.hex()=='558bec6a008b4514508b4d10518b550c528b45088b108b4d08ff52205dc3'
original_body=PE(ROOT/config['target']['path']).read(0x4102e0,27);assert original_body.hex()=='8b5424108b4c24046a00528b5424148b01528b54241452ff5020c3'
provider_disassembly=subprocess.check_output(['objdump','-d','-Mintel','--disassemble=query',str(BINARY/'build/native/CVPACK')],text=True)
returns=re.findall(r'\bret\s+(0x[0-9a-f]+)',provider_disassembly);assert returns==['0x10'],provider_disassembly
(pair/'native-query-callback-disassembly.txt').write_text(provider_disassembly)
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,actual_query_calls=1,actual_input_bytes=57,actual_thiscall_stack_bytes=16,original_stack_cleanup=query['exit_esp']-query['entry_esp'],native_callback_cleanup=16,query_output=dict(bytes=60,fixups=2,output=0,records=0),emit_descriptor=baseline['parameters']['output'],own_unwritten_output_allocation_positive=True,original_wrapper_bytes=original_body.hex(),rebuilt_wrapper_bytes=body.hex(),same_as_independent_genuine_cpp_probe=True,independent_abi_receipt_sha256=sha(BINARY/'cases/hello-debug-dll/abi/receipt.json'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),seconds=time.perf_counter()-start)
(pair/'query-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
