#!/usr/bin/env python3
"""Main cleanup ownership/dead-state controls and the actual scope callback."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time
from state import BINARY,ROOT,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0096-enter-symbol-scope-lifetimes'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);original=decoder.decode(events);assert baseline['main_free_lifetimes']['calls']==5
assert len(native['widen_release_calls'])==21 and native['widen_release_calls'][-1]['exit_cursor']==21
rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('missing-free-lifetime',lambda s:s['main_free_calls'].pop(),'main free lifetime inventory differs')
for name,field,value,reason in [
    ('wrong-sequence','index',0,'main free actual sequence/source differs'),
    ('wrong-argument','address',0,'main free source-global argument differs'),
    ('missing-return','returned',False,'main free native cdecl return differs'),
    ('wrong-return-stack','exit_esp',0,'main free native cdecl return differs'),
    ('wrong-last-error','exit_last_error',123,'main free observable LastError differs')]:
    reject(name,lambda s,f=field,v=value:s['main_free_calls'][1].update({f:v}),reason)
def byte(state,index,offset):
    call=state['main_free_calls'][index];raw=bytearray.fromhex(call['before']);raw[offset]^=1;call['before']=raw.hex()
reject('changed-initialized-pre-free-byte',lambda s:byte(s,1,0),'main free initialized pre-free bytes differ')
reject('truncated-pre-free-capacity',lambda s:s['main_free_calls'][1].update(before=s['main_free_calls'][1]['before'][:-2]),'main free incomplete pre-free capacity')
reject('cleared-source-pointer',lambda s:s['globals'].update(line_data='00000000'),'main free source pointers changed/cleared')
reject('changed-source-alias-before-free',lambda s:s['main_free_calls'][1].update(symbol_cursor=0),'main free native alias inputs differ')
address=native['main_free_calls'][1]['address']
reject('retired-interior-payload-in-current-state',lambda s:s['memory'].update({str(address+4):'00'*4}),'released payload was captured as current state')
reject('unknown-post-free-caller',lambda s:s.update(frontier_caller='pack_module'),'unmodeled target consumer after main free')
# A live local type table may retain a dangling pointer, but its exact former
# input allocation and offset remain observable identities.
root=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little');records=bytes.fromhex(native['memory'][str(root)])
index=next(i for i in range(int.from_bytes(bytes.fromhex(native['globals']['type_record_count']),'little')) if address<=int.from_bytes(records[i*32:i*32+4],'little')<address+native['main_free_calls'][1]['size'])
def bad_local_alias(state):
    raw=bytearray.fromhex(state['memory'][str(root)]);value=int.from_bytes(raw[index*32:index*32+4],'little');raw[index*32:index*32+4]=(value+1).to_bytes(4,'little');state['memory'][str(root)]=raw.hex()
reject('wrong-retired-local-type-offset',bad_local_alias,'type record segment/payload offset alias differs')
reject('wrong-finished-scope-depth',lambda s:s['globals'].update(public_scope_depth='01000000'),'application global differs')
# Raw provider mutation reaches the qualified public-CRT guard rather than a
# capture hash mismatch; original bytes remain immutable on disk.
match=re.search(r'^main-free-call 00404726 ([0-9a-f]{8}) 0 ([0-9a-f]{8})$',events,re.M);assert match
begin,end=match.span(2);wrong=events[:begin]+'00000000'+events[end:]
reject('original-wrong-free-provider',lambda s:None,'main free actual canonical provider differs',wrong)
marker=re.search(r'^main-free-return 00404726 [0-9a-f]{8}$',events,re.M).start();begin=events.rfind('registers ',0,marker);end=events.index('\n',begin);fields=events[begin:end].split();old=int(fields[-1],16);fields[-1]='%08x'%(old+4)
wrong=events[:begin]+' '.join(fields)+events[end:marker].replace('memory %08x '%old,'memory %08x '%(old+4),1)+events[marker:]
reject('original-wrong-free-return-stack',lambda s:None,'main free original cdecl stack/return differs',wrong)
# An unused capacity byte is each allocator's own uninitialized value; no
# cross-process comparison or original-padding replay is permitted.
changed=copy.deepcopy(native);unused=None
for index,call in enumerate(original['main_free_calls']):
    if not call['address']:continue
    ranges=[(0,4)] if index==1 else []
    ranges.extend((row['buffer']-call['address'],row['buffer']-call['address']+row['returned']) for row in original['file_reads'] if call['address']<=row['buffer']<call['address']+call['size'])
    candidates=[offset for offset in range(call['size']) if not any(first<=offset<last for first,last in ranges)]
    if candidates:unused=(index,candidates[-1]);break
assert unused is not None
byte(changed,*unused);compare(changed)
# Minimal native unit: execute the reconstructed C body on both preserved and
# discarded scopes. Dead sections are removed only from this independent unit,
# never from reconstruction/fixture binaries.
unit='''#include "pack.h"\n#include <recon.h>\n#undef main\nunsigned public_scope_depth, discard_symbol_depth;\nunsigned char *symbol_input;\nint main(void)\n{\n    unsigned char record[12];\n    unsigned i, pass;\n    for (pass = 0; pass < 2; ++pass) {\n        for (i = 0; i < 12; ++i) record[i] = (unsigned char)(0x40 + i);\n        record[2] = 7; record[3] = 2;\n        symbol_input = record; public_scope_depth = 7; discard_symbol_depth = pass ? 2 : 0;\n        enter_symbol_scope();\n        if (public_scope_depth != 8 || discard_symbol_depth != (pass ? 3 : 0)) return 1;\n        if (record[2] != (pass ? 8 : 7) || record[3] != (pass ? 0 : 2)) return 2;\n        for (i = 0; i < 12; ++i) if (i != 2 && i != 3 && record[i] != (unsigned char)(0x40 + i)) return 3;\n    }\n    return 0;\n}\n'''
(pair/'scope-unit.c').write_text(unit)
with tempfile.TemporaryDirectory(prefix='cvpack-scope-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'scope'
    command=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g','-DRECON_NATIVE_TEST','-ffunction-sections','-fdata-sections','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),str(BINARY/'src/symbol-handlers.c'),str(pair/'scope-unit.c'),'-Wl,--gc-sections','-o',str(executable)]
    build=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15);(pair/'scope-unit-build.log').write_bytes(build.stdout);assert build.returncode==0,build.stdout
    execution=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert execution.returncode==0 and not execution.stdout and not execution.stderr
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,scope_unit_cases=2,scope_unit_source_sha256=sha(pair/'scope-unit.c'),scope_source_sha256=sha(BINARY/'src/symbol-handlers.c'),own_uninitialized_pre_free_capacity_change_allowed=list(unused),lifetimes=baseline['main_free_lifetimes'],widen_lifetimes=21,
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),audit_sha256=sha(pair/'final-audit/audit.json'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),lifetime_validator_sha256=sha(BINARY/'scripts/main-free-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),checker_sha256=sha(Path(__file__)),seconds=time.perf_counter()-start)
(pair/'lifetime-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),scope_units=2,seconds=report['seconds'])))
