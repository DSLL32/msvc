#!/usr/bin/env python3
"""Full emit capacities, actual initialized prefixes, own tails and live consumers."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0094-emit-widen'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);original=decoder.decode(events);emit=original['widen_emit_calls'][0];actual=native['widen_emit_calls'][0]
assert len(native['widen_emit_calls'])==1 and actual['output_capacity']==60 and actual['fixup_capacity']==2
assert int.from_bytes(bytes.fromhex(actual['description_after'])[:4],'little')==57
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
def byte(state,field,offset):
    call=state['widen_emit_calls'][0];raw=bytearray.fromhex(call[field]);raw[offset]^=1;call[field]=raw.hex()
for name,value in [('flags',1),('result',0)]:reject('wrong-emit-'+name,lambda s,n=name,v=value:s['widen_emit_calls'][0].update({n:v}),'emit actual flags/result differ')
reject('missing-actual-emit',lambda s:s.update(widen_emit_calls=[]),'emit lifetime inventory differs')
reject('changed-initialized-output',lambda s:byte(s,'output_after',0),'emit initialized output prefix differs')
reject('changed-initialized-fixup',lambda s:byte(s,'fixups_after',0),'emit initialized output prefix differs')
reject('changed-own-output-tail',lambda s:byte(s,'output_after',59),'emit changed own unwritten capacity tail')
reject('changed-input-after-emit',lambda s:byte(s,'input_after',0),'emit changed initialized input')
reject('truncated-output-capacity-before',lambda s:s['widen_emit_calls'][0].update(output_before=s['widen_emit_calls'][0]['output_before'][:-2]),'emit full capacity observation differs')
reject('truncated-output-capacity-after',lambda s:s['widen_emit_calls'][0].update(output_after=s['widen_emit_calls'][0]['output_after'][:-2]),'emit full capacity observation differs')
reject('changed-output-allocation-pointer',lambda s:byte(s,'description_after',8),'emit changed output allocation pointers')
reject('wrong-written-byte-count',lambda s:byte(s,'description_after',0),'emit returned initialized counts differ')
reject('wrong-input-object-alias',lambda s:s['widen_emit_calls'][0].update(object=0),'emit queried object alias differs')
def live_byte(state):
    address=state['widen_emit_calls'][0]['output_address'];raw=bytearray.fromhex(state['memory'][str(address)]);raw[0]^=1;state['memory'][str(address)]=raw.hex()
reject('changed-live-scratch-consumer',live_byte,'live emitted scratch buffer state differs')
reject('wrong-release-lifetime-token',lambda s:s['parameters']['object'].update(state='queried'),'typed ABI arguments differ')
# Each side preserves ITS own initial tail; it need not match original bytes.
changed=copy.deepcopy(native);call=changed['widen_emit_calls'][0]
for field in ['output_before','output_after']:call[field]=call[field][:57*2]+'c7'*3
changed['memory'][str(call['output_address'])]=call['output_after'];compare(changed)
# Original raw capacity mutation must fail the own-tail semantic guard even
# when aggregate file hashes are deliberately outside this decoder-level test.
index=events.index('widen method-return 36 ');pointer=int.from_bytes(emit['before'][emit['description']][8:12],'little')
match=re.search(r'^memory %08x ([0-9a-f]+)$'%pointer,events[index:],re.M);assert match
begin,end=index+match.start(1),index+match.end(1);raw=bytearray.fromhex(match[1]);assert len(raw)==60;raw[59]^=1
reject('original-changed-capacity-tail',lambda s:None,'emit changed own unwritten capacity tail',events[:begin]+raw.hex()+events[end:])
# A newly encountered one-formal release boundary still uses the actual caller
# local object, even though the callee body is unreconstructed.
with tempfile.TemporaryDirectory(prefix='cvpack-release-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);bad=copy.deepcopy(config);next(r for r in bad['functions'] if r['name']=='release_widen')['parameters']=[]
    path=directory/'configuration.json';path.write_text(json.dumps(bad));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(path),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and 'incomplete frontier ABI contract' in diagnostic,diagnostic
    rejected.append(dict(name='actual-gdb-missing-release-formal',diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(path)))
obj=COFF((BINARY/'build/widen.obj').read_bytes());symbols={r['name']:r for r in obj.symbols.values()};row=symbols['_emit_widen'];body=obj.sections[row['section']-1]['data'][row['value']:]
assert body.hex()=='558bec8b4518508b4d14518b5510528b450c508b4d088b118b4d08ff52245dc3'
original_body=PE(ROOT/config['target']['path']).read(0x410300,30);assert original_body.hex()=='8b5424148b4c2404528b5424148b01528b542414528b54241452ff5024c3'
provider_disassembly=subprocess.check_output(['objdump','-d','-Mintel','--disassemble=emit',str(BINARY/'build/native/CVPACK')],text=True)
assert re.findall(r'\bret\s+(0x[0-9a-f]+)',provider_disassembly)==['0x10'];(pair/'native-emit-callback-disassembly.txt').write_text(provider_disassembly)
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,actual_emit_calls=1,actual_input_bytes=57,output_capacity=60,written_output_bytes=57,fixup_capacity=2,written_fixups=2,full_capacity_bytes_checked=76,output_own_tail_bytes=3,fixup_own_tail_bytes=0,own_tail_arbitrary_native_positive=True,original_thiscall_cleanup=emit['exit_esp']-emit['entry_esp'],native_thiscall_cleanup=16,original_wrapper_bytes=original_body.hex(),rebuilt_wrapper_bytes=body.hex(),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),seconds=time.perf_counter()-start)
(pair/'emit-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
