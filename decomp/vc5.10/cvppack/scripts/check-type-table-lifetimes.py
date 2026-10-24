#!/usr/bin/env python3
"""Focused historical-table and finite12free lifecycle controls; native only."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0099-finalize-derived-types';source=pair/'native'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((source/'state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);life=baseline['main_free_lifetimes'];assert life['calls']==12 and life['historical_typed_table_bytes_checked']==416768
rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError,IndexError) as error:
        assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
def byte(state,index,offset):
    row=state['main_free_calls'][index];raw=bytearray.fromhex(row['before']);raw[offset]^=1;row['before']=raw.hex()
reject('missing-final-table-free',lambda s:s['main_free_calls'].pop(),'main free lifetime inventory differs')
reject('wrong-table-free-owner',lambda s:s['main_free_calls'][7].update(owner='free_widen_buffers'),'main free native cdecl return differs')
reject('wrong-table-free-argument',lambda s:s['main_free_calls'][7].update(address=0),'main free source-global argument differs')
reject('wrong-table-return-stack',lambda s:s['main_free_calls'][11].update(exit_esp=0),'main free native cdecl return differs')
reject('truncated-prefree-record-capacity',lambda s:s['main_free_calls'][11].update(before=s['main_free_calls'][11]['before'][:-2]),'main free incomplete pre-free capacity')
for index in (7,10):reject('changed-cleared-byte-table-'+str(index),lambda s,i=index:byte(s,i,0),'unmodeled nonzero cleanup table')
reject('changed-initialized-record-scalar',lambda s:byte(s,11,4),'initialized type record fields differ')
reject('changed-unused-cleared-record',lambda s:byte(s,11,383999),'unused type record capacity is not cleared')
for index,reason in [(8,'recursive-type node NULL/identity/chain differs'),(9,'interned-type node NULL/identity/chain differs')]:
    raw=bytes.fromhex(native['main_free_calls'][index]['before']);offset=next(i for i in range(0,len(raw),4) if any(raw[i:i+4]))
    def head(state,i=index,o=offset):
        call=state['main_free_calls'][i];b=bytearray.fromhex(call['before']);b[o:o+4]=bytes(4);call['before']=b.hex()
    reject('changed-historical-head-'+str(index),head,reason)
reject('wrong-historical-free-identity',lambda s:s['historical_type_tables'][0].update(free_index=7),'native historical table traversal identity differs')
for index in range(7,12):
    address=native['main_free_calls'][index]['address']
    reject('crossing-current-table-'+str(index),lambda s,a=address:s['memory'].update({str(a-4):'00'*8}),'released payload was captured as current state')
reject('crossing-current-table-string',lambda s:s['strings'].update({str(s['main_free_calls'][11]['address']-4):'00'*8}),'released payload was captured as current string state')
reject('unknown-postcleanup-caller',lambda s:s.update(frontier_caller='free_type_tables'),'unmodeled target consumer after main free')
original=decoder.decode(events);address=original['main_free_calls'][11]['address']
reject('crossing-original-current-table',lambda s:None,'released payload was captured as current state',events+'memory %08x %s\n'%(address-4,'00'*8))
# Force the actual GDB observer to attempt a crossing released-table read.
# An isolated scratch copy keeps every owned raw capture unchanged.
with tempfile.TemporaryDirectory(prefix='cvpack-table-guard-',dir=Path.home()/'.ai-tmp') as temporary:
    scratch=Path(temporary)/'native';shutil.copytree(source,scratch)
    from caseio import prepare
    prepare(json.loads((pair/'original/case-definition.json').read_text()),scratch)
    helper=scratch/'gdb-state.py';text=helper.read_text();needle="return bytes.fromhex(calls[0]['before'])";assert text.count(needle)==1
    helper.write_text(text.replace(needle,'return bytes(inferior.read_memory(address-4,8))'))
    commands=(scratch/'commands.gdb');commands.write_text(commands.read_text().replace(str(source),str(scratch)))
    (scratch/'state.json').unlink()
    receipt=json.loads((pair/'original/receipt.json').read_text());env=os.environ.copy()
    for key in ('RECON_TRACE_FD','RECON_TRACE_FILTER','RECON_TRACE_SEEN'):env.pop(key,None)
    env.update(VC510_CVPACK_ARGV0=receipt['canonical_argv0'],RECON_PROGRAM_PATH=receipt['actual_target_windows'],VC510_CVPACK_RESOURCES=str(scratch/'resources.rmr'),VC510_CVPACK_WIDEN=str(scratch/'widen.rwp'))
    begin=time.perf_counter();result=subprocess.run(['gdb','-q','-nx','-batch','-x',str(commands),str(BINARY/'build/native/CVPACK')],cwd=scratch,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15);seconds=time.perf_counter()-begin
    reason='current-state observer tried to dereference released payload'
    assert result.returncode!=0 and reason.encode() in result.stderr and not (scratch/'state.json').exists(),(result.returncode,result.stderr)
    rejected.append(dict(name='actual-GDB-crossing-released-table-read',diagnostic=reason,seconds=seconds))
# Exercise the actual original-observer range selector as pure C. Its source
# is extracted, never reimplemented; mock child reads expose unsafe fallback.
with tempfile.TemporaryDirectory(prefix='cvpack-table-range-',dir=Path.home()/'.ai-tmp') as temporary:
    work=Path(temporary);text=(BINARY/'scripts/debug-run.c').read_text()
    first=text.index('static int historical_type_table(');last=text.index('\nstatic int write_bytes(',first);body=text[first:last]
    unit=work/'ranges.c';binary=work/'ranges'
    unit.write_text('#include <stdint.h>\n#include <string.h>\ntypedef uint32_t DWORD; typedef void *HANDLE;\nstatic DWORD main_free_addresses[12],main_free_sizes[12],main_free_count;\nstatic unsigned char *main_free_before[12]; static int child_reads;\nstatic int read_bytes(HANDLE h,DWORD a,void *p,DWORD n) { ++child_reads; return 1; }\n'+body+'\nint main(void) { unsigned char data[16],out[32]; int i; for(i=0;i<16;++i)data[i]=(unsigned char)i; main_free_count=8;main_free_addresses[7]=0x1000;main_free_sizes[7]=16;main_free_before[7]=data; if(!historical_type_table(0,0x1004,out,4)||memcmp(out,data+4,4)||child_reads)return 1; if(historical_type_table(0,0xffc,out,8)||child_reads)return 2; if(historical_type_table(0,0x100c,out,8)||child_reads)return 3; if(!historical_type_table(0,0xff8,out,4)||child_reads!=1)return 4; return 0; }\n')
    build=subprocess.run(['clang','-m32','-O0',str(unit),'-o',str(binary)],stdout=subprocess.PIPE,stderr=subprocess.PIPE);assert build.returncode==0,build.stderr.decode()
    result=subprocess.run([str(binary)],stdout=subprocess.PIPE,stderr=subprocess.PIPE);assert result.returncode==0,(result.returncode,result.stderr)
    rejected.extend([dict(name='original-observer-crossing-before-base',diagnostic='return0,nochildread'),dict(name='original-observer-crossing-after-end',diagnostic='return0,nochildread')])
    range_positive=dict(saved_contained_bytes=True,wholly_disjoint_child_read=True,extracted_body_sha256=__import__('hashlib').sha256(body.encode()).hexdigest())
report=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,observer_range_positive=range_positive,baseline=life,typed_records_checked=baseline['typed_record_entries_checked'],recursive_nodes_checked=baseline['recursive_type_nodes_checked'],interned_nodes_checked=baseline['interned_type_nodes_checked'],
 source_sha256=sha(BINARY/'src/derived-types.c'),lifetime_validator_sha256=sha(BINARY/'scripts/main-free-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(source/'gdb-state.py'),checker_sha256=sha(Path(__file__)),
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(source/'receipt.json'),audit_sha256=sha(pair/'original/audit.json'),
 reused=dict(path='cases/hello-debug-dll/evidence/0098-free-type-tables-frontier/type-cleanup-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0098-free-type-tables-frontier/type-cleanup-checks.json'),scope='Earlier7free paths, bound provider, PDB125 and protocol controls unchanged; fresh table/pointer-field/overlap/guard controls here.'),seconds=time.perf_counter()-start)
(pair/'type-table-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=report['seconds'])))
