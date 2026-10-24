#!/usr/bin/env python3
"""Bounded cleanup-entry state, opaque close denials and named-import guards."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time
from state import BINARY, ROOT, sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0098-free-type-tables-frontier'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native-complete-provenance/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/folder).glob(pattern)} for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native-complete-provenance','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['main_free_lifetimes']['calls']==7
rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except AssertionError as error:
        assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('old-main-caller-policy',lambda s:s.update(frontier_caller='cvpack_main'),'unmodeled target consumer after main free')
for name in ('type_pdb','type_pdb_types','type_pdb_close_status'):
    reject('changed-'+name,lambda s,n=name:s['globals'].update({n:'01000000'}),name)
marker=events.index('frontier 0040df60\n');tail=events[marker:];match=re.search(r'^(memory [0-9a-f]{8} )46b14000',tail,re.M);assert match
position=marker+match.end();changed=events[:position-8]+'00000000'+events[position:]
reject('wrong-original-child-return',lambda s:None,'unmodeled target consumer after main free',changed)
reject('missing-prior-scratch-return',lambda s:s['main_free_calls'][-1].update(returned=False),'emitted scratch lifetime lacks actual free return')
# Invoke the actual owned native host adapter with the real shared fail125
# implementation. No target body or MSPDB success semantics are substituted.
with tempfile.TemporaryDirectory(prefix='cvpack-pdb-denial-',dir=Path.home()/'.ai-tmp') as temporary:
    work=Path(temporary);probe=work/'probe.c';binary=work/'probe'
    probe.write_text('#define RECON_SHIM_IMPL\n#include <recon.h>\nextern int TypesClose(void *);\nextern int PDBClose(void *);\nint main(int argc,char **argv) { return argc==1 ? TypesClose((void *)1) : PDBClose((void *)1); }\n')
    command=['clang','-m32','-O0','-g','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),str(probe),str(BINARY/'scripts/native-pdb.c'),*[str(p) for p in sorted((ROOT/'lib/recon/src').glob('*.c'))],'-o',str(binary)]
    begin=time.perf_counter();build=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE);assert build.returncode==0,build.stderr.decode();build_seconds=time.perf_counter()-begin
    denials=[]
    for name,args in [('TypesClose',[]),('PDBClose',['pdb'])]:
        begin=time.perf_counter();result=subprocess.run([str(binary),*args],stdout=subprocess.PIPE,stderr=subprocess.PIPE);elapsed=time.perf_counter()-begin
        assert result.returncode==125 and result.stdout==b'' and result.stderr==('recon: unsupported native operation: CVPACK unobserved MSPDB '+name+'\n').encode(),(name,result)
        denials.append(dict(name=name,status=result.returncode,stderr=result.stderr.decode(),seconds=elapsed))
report=dict(schema=1,native_only=True,rejected=rejected,fresh_negative_count=len(rejected)+len(denials),pdb_denials=denials,host_probe_build_seconds=build_seconds,baseline=baseline['main_free_lifetimes'],
 source_sha256=sha(BINARY/'src/types.c'),host_denial_sha256=sha(BINARY/'scripts/native-pdb.c'),pdb_abi_witness_sha256=sha(BINARY/'cases/hello-debug-dll/pdb-close-imports.json'),
 checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),lifetime_validator_sha256=sha(BINARY/'scripts/main-free-state.py'),
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native-complete-provenance/receipt.json'),audit_sha256=sha(pair/'current-audit/audit.json'),
 reused=dict(path='cases/hello-debug-dll/evidence/0097-free-widen-buffers/cleanup-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0097-free-widen-buffers/cleanup-checks.json'),scope='Unchanged7free/generation/capacity checks; fresh controls cover new frontier/caller/globals and unobserved PDB calls.'),seconds=time.perf_counter()-start)
(pair/'type-cleanup-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['fresh_negative_count'],seconds=report['seconds'])))
