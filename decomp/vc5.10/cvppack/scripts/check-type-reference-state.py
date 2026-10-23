#!/usr/bin/env python3
"""Actual extended offset/flag objects, cached-import necessity and storage edges."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time
from state import BINARY,ROOT,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
spec=importlib.util.spec_from_file_location('switch_data',BINARY/'scripts/switch-data.py');switches=importlib.util.module_from_spec(spec);spec.loader.exec_module(switches)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['extended_type_references_checked']==1
assert baseline['parameters']==dict(record=dict(root='type_records',index=332),index=4428)
def root(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
table=root(native,'type_records');record=bytes.fromhex(native['memory'][str(table)])[331*32:332*32]
references=int.from_bytes(record[16:20],'little');flags=int.from_bytes(record[24:28],'little')
assert bytes.fromhex(native['memory'][str(references)])==bytes.fromhex('0800240038004c00600078009000a80000000000')
assert bytes.fromhex(native['memory'][str(flags)])==b'\0'*40
original=decoder.decode(events);original_table=int.from_bytes(original['memory'][next(row['original'] for row in data if row['name']=='type_records')],'little')
original_record=original['memory'][original_table][331*32:332*32];original_references=int.from_bytes(original_record[16:20],'little')
def word(state,address,offset,value):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+4]=value.to_bytes(4,'little');state['memory'][str(address)]=raw.hex()
def original_word(address,offset,value):
    matches=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));assert matches
    begin,end=matches[-1].span(1);raw=bytearray.fromhex(matches[-1][1]);raw[offset:offset+4]=value.to_bytes(4,'little')
    return events[:begin]+raw.hex()+events[end:]
rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('wrong-offset-array-owner',lambda s:word(s,table,331*32+16,references+2),'extended type reference allocation/alias differs')
reject('wrong-flag-array-owner',lambda s:word(s,table,331*32+24,references),'extended type reference allocation/alias differs')
reject('wrong-used-offset',lambda s:word(s,references,0,9),'initialized extended type reference bytes differ')
reject('wrong-cleared-offset-tail',lambda s:word(s,references,16,1),'initialized extended type reference bytes differ')
reject('both-dirty-cleared-offset-tail',lambda s:word(s,references,16,1),'unused cleared type reference capacity differs',original_word(original_references,16,1))
reject('wrong-flag-byte',lambda s:word(s,flags,0,1),'initialized extended type reference bytes differ')
for name,address in [('offset',references),('flags',flags)]:
    reject('truncated-'+name+'-capacity',lambda s,a=address:s['memory'].update({str(a):s['memory'][str(a)][:-2]}),'incomplete extended type reference allocation')
reject('wrong-reference-count',lambda s:word(s,table,331*32+12,9),'initialized type record fields differ')
reject('lost-extended-status',lambda s:word(s,table,331*32+28,0x4c0),'initialized type record fields differ')
reject('lost-inline-residual',lambda s:word(s,table,331*32+20,0),'initialized type record fields differ')
reject('wrong-recursive-record',lambda s:s['parameters']['record'].update(index=331),'typed ABI arguments differ')
reject('wrong-recursive-index',lambda s:s['parameters'].update(index=4429),'typed ABI arguments differ')
audit=json.loads((pair/'original/audit.json').read_text());metadata=json.loads((pair/'original/graph-metadata.json').read_text())
assert switches.check_import_sites(audit['graph'])['type_reference_sites']['original_free_sites']==[0x404434,0x4044a7]
ablations=[]
for target in [{'import':{'dll':'msvcrt.dll','name':'free'}},{'function':'_allocate_cleared'}]:
    with tempfile.TemporaryDirectory(prefix='cvpack-reference-import-',dir=Path.home()/'.ai-tmp') as temporary:
        changed=copy.deepcopy(metadata);row=next(row for row in changed['register_imports']['preservation_contracts'] if row['target']==target);row['preserves'].remove('ebx')
        path=Path(temporary)/'metadata.json';path.write_text(json.dumps(changed));command=list(audit['graph_command']);command[command.index('--metadata')+1]=str(path)
        result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
        assert result.returncode and b'@add_type_reference@8' in result.stderr and (b'indirect call topology differs' in result.stderr or b'unresolved register import calls' in result.stderr),result.stderr
        ablations.append(dict(target=target,register='ebx',diagnostic=result.stderr.decode(),metadata_sha256=sha(path)))
changed=copy.deepcopy(audit['graph']);changed['functions']['@add_type_reference@8']['register_imports']['original']['resolved_register_calls'].pop(str(0x4044a7))
try:switches.check_import_sites(changed)
except AssertionError as error:
    assert str(error)=='type reference cached free inventory differs';rejected.append(dict(name='omitted-second-free-site',diagnostic=str(error)))
else:raise AssertionError('accepted missing cached site')
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/type-reference-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-reference-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'type-reference-unit';command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert not result.returncode,(result.returncode,result.stderr)
report=dict(schema=1,native_only=True,count=len(rejected)+len(ablations),rejected=rejected,register_ablations=ablations,parameters=baseline['parameters'],extended_records=1,initialized_offset_capacity=20,initialized_flag_capacity=40,original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),graph_tool_sha256=audit['graph_tool_sha256'],checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),metadata_validator_sha256=sha(BINARY/'scripts/switch-data.py'),unit=dict(scope='Native inline/extended/growth storage boundaries; no new original branch oracle.',inputs=inputs,status=result.returncode,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'type-reference-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],unit_status=result.returncode,seconds=report['seconds'])))
