#!/usr/bin/env python3
"""Callback-table identities, owned output prefix and genuine ushort ABI."""
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile
import time
from state import BINARY, ROOT, sha
from binary import PE

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0060-rewrite-module-symbols'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['buffer']==dict(owner='rewrite_module_symbols',offset=4,capacity=8705,write_size=12)
allocation=next(row for row in native['image_allocations'] if row.get('owner')=='rewrite_module_symbols')
address=allocation['address'];assert allocation['size']==8705
def write(state,offset,value):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+len(value)]=value
    state['memory'][str(address)]=raw.hex()
rejected=[]
def reject(name,mutation,reason,raw=events):
    state=copy.deepcopy(native);mutation(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('changed-output-signature',lambda s:write(s,0,b'\x01'),'initialized symbol output prefix differs')
reject('truncated-full-output-observation',lambda s:s['memory'].__setitem__(str(address),s['memory'][str(address)][:-2]),'incomplete symbol output capacity observation')
reject('wrong-output-owner',lambda s:next(row for row in s['image_allocations'] if row.get('owner')=='rewrite_module_symbols').update(owner='other'),'symbol output allocation owner differs')
reject('wrong-output-request-size',lambda s:next(row for row in s['image_allocations'] if row.get('owner')=='rewrite_module_symbols').update(size=8704),'actual image allocation request/result differs')
reject('changed-output-cursor',lambda s:s['globals'].update(symbol_output=(address+5).to_bytes(4,'little').hex()),'symbol output cursor allocation/offset differs')
reject('wrong-output-argument-offset',lambda s:s['parameters']['buffer'].update(offset=5),'typed ABI arguments differ')
reject('wrong-output-argument-write-extent',lambda s:s['parameters']['buffer'].update(write_size=13),'typed ABI arguments differ')
reject('wrong-low-segment-word',lambda s:s['parameters'].update(segment=2),'typed ABI arguments differ')
reject('wrong-actual-short-width',lambda s:s['observed_argument_types'][1].update(width=4),'actual frontier formal width differs')
def table(state,offset,value):
    raw=bytearray.fromhex(state['globals']['symbol_rewriters']);raw[offset:offset+len(value)]=value
    state['globals']['symbol_rewriters']=raw.hex()
reject('wrong-dispatch-kind',lambda s:table(s,0,b'\xff\xff'),'symbol callback kind/reserved differs')
reject('wrong-logical-callback',lambda s:table(s,4,s['function_addresses']['enter_symbol_scope'].to_bytes(4,'little')),'symbol callback logical identity differs')
reject('invented-default-callback',lambda s:table(s,38*8+4,s['function_addresses']['enter_symbol_scope'].to_bytes(4,'little')),'symbol callback NULL identity differs')

# Original caller writes AX before pushing EAX. Its high word is deliberately
# irrelevant to the callee's real ushort load at40df34, not a missing state field.
original=decoder.decode(events);word=original['stack'][2];assert word&0xffff==1 and word>>16
old=struct.pack('<I',word).hex();new=struct.pack('<I',(word&0xffff)^0x12340000).hex()
lines=events.splitlines();index=max(i for i,line in enumerate(lines) if line.startswith('memory %08x '%original['esp']))
prefix,payload=lines[index].rsplit(' ',1);assert payload[16:24]==old
lines[index]=prefix+' '+payload[:16]+new+payload[24:]
compare(native,'\n'.join(lines)+'\n')
lines[index]=prefix+' '+payload[:16]+struct.pack('<I',word^1).hex()+payload[24:]
reject('changed-raw-low-segment-word',lambda s:None,'typed ABI arguments differ','\n'.join(lines)+'\n')
state=copy.deepcopy(native);raw=bytearray.fromhex(state['memory'][str(address)])
raw[4:]=bytes(value^0xff for value in raw[4:]);state['memory'][str(address)]=raw.hex();compare(state)

spec=importlib.util.spec_from_file_location('switch_data',BINARY/'scripts/switch-data.py')
tables=importlib.util.module_from_spec(spec);spec.loader.exec_module(tables)
proof=tables.function_tables(config['functions']);assert len(proof)==1 and len(proof[0]['rows'])==39
pe=PE(BINARY/'build/CVPACK.EXE');base=proof[0]['rebuilt'];static=[]
for label,offset,value,reason in [
    ('changed-static-kind',0,b'\xff\xff','symbol rewrite table kind/reserved differs'),
    ('changed-static-reserved',2,b'\x01\x00','symbol rewrite table kind/reserved differs'),
    ('changed-static-callback',4,pe.read(base+8+4,4),'symbol rewrite table callback identity differs'),
    ('changed-static-default',38*8+4,pe.read(base+8+4,4),'symbol rewrite table NULL callback differs')]:
    changed=copy.deepcopy(pe);raw=bytearray(changed.data);position=changed.offset(base)+offset;raw[position:position+len(value)]=value;changed.data=bytes(raw)
    try:tables.function_tables(config['functions'],rebuilt=changed)
    except AssertionError as error:
        assert reason in str(error),str(error)
        static.append(dict(name=label,diagnostic=str(error)))
    else:raise AssertionError('accepted '+label)

# The candidate sidecar's new structured data proof is regenerated by accept;
# replacing its rows and refreshing the receipt hash must still fail.
spec=importlib.util.spec_from_file_location('accept',BINARY/'scripts/accept.py')
accept=importlib.util.module_from_spec(spec);spec.loader.exec_module(accept)
with tempfile.TemporaryDirectory(prefix='cvpack-callback-proof-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary)/'audit';shutil.copytree(pair/'proof-refresh',directory)
    path=directory/'graph-metadata-proof.json';changed=json.loads(path.read_text())
    changed['function_tables'][0]['rows'][0]['function']='enter_symbol_scope';path.write_text(json.dumps(changed,indent=2)+'\n')
    audit=directory/'audit.json';record=json.loads(audit.read_text());record['graph_metadata_proof_sha256']=sha(path);audit.write_text(json.dumps(record,indent=2)+'\n')
    try:accept.validate_pair(pair/'original/configuration.json',pair/'original',pair/'native',audit)
    except AssertionError as error:
        assert 'forged current data proof' in str(error),str(error)
        static.append(dict(name='forged-refreshed-table-proof',diagnostic=str(error)))
    else:raise AssertionError('accepted forged callback data proof')
provider=PE(ROOT/config['target']['path'])
assert provider.read(0x40df34,5)==bytes.fromhex('668b4c2408')
report=dict(schema=1,native_only=True,count=len(rejected)+len(static),rejected=rejected,static_rejections=static,
    owned_capacity=8705,initialized_prefix=4,native_owned_tail_change_allowed=True,dead_ushort_high_word_change_allowed=True,
    actual_original_pushed_segment_word=word,ushort_load=dict(address=0x40df34,bytes='668b4c2408'),callback_rows=39,
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    decoder_sha256=sha(BINARY/'scripts/raw-state.py'),checker_sha256=sha(Path(__file__)),
    table_validator_sha256=sha(BINARY/'scripts/switch-data.py'),seconds=time.perf_counter()-start)
(pair/'symbol-output-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
