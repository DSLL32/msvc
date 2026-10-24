#!/usr/bin/env python3
"""Source callback ABI, live owner, new generation and cached-free controls."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0100-source-file-index';source=pair/'native';audit=pair/'hardened-audit/audit.json'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((source/'state.json').read_text())
def module(name):
    spec=importlib.util.spec_from_file_location(name.replace('-','_'),BINARY/'scripts'/(name+'.py'));result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
decoder=module('raw-state');switches=module('switch-data')
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError,IndexError) as error:
        assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for field,value,reason in [('after_frees',11,'source-index allocator generation ledger differs'),('owner','allocate_bytes','source-index actual new generation inventory differs')]:
    reject('wrong-generation-'+field,lambda s,f=field,v=value:(s['source_index_allocations'][0].update({f:v}),s['checked_allocations'][-1].update({f:v})),reason)
reject('wrong-new-block-alias',lambda s:s['source_index_context'].update(block_address=0),'source-index temporary block alias differs')
reject('wrong-live-module-owner',lambda s:s['source_index_context'].update(module_address=0),'source-index actual live source owner differs')
reject('wrong-live-source-owner',lambda s:s['source_index_context'].update(source_address=0),'source-index actual live source owner differs')
def header(state,offset):
    raw=bytearray.fromhex(state['source_index_context']['block_bytes']);raw[offset]^=1;state['source_index_context']['block_bytes']=raw.hex()
reject('changed-initialized-module-header',lambda s:header(s,0),'source-index initialized temporary header differs')
reject('truncated-new-block',lambda s:s['source_index_context'].update(block_bytes=s['source_index_context']['block_bytes'][:-2]),'incomplete new source module allocation capacity')
reject('wrong-unwritten-offset-cursor',lambda s:s['source_index_context'].update(offset_cursor=0),'source-index unwritten offset cursor differs')
reject('wrong-file-counter',lambda s:s['source_index_context'].update(file_index=1),'source-index first-file counter differs')
reject('wrong-name-offset',lambda s:s['source_index_context'].update(name_bytes=1),'source-index first filename offset differs')
reject('changed-counted-name',lambda s:s['parameters']['name'].update(bytes='00'),'typed ABI arguments differ')
reject('wrong-counted-name-offset',lambda s:s['parameters']['name'].update(source_offset=37),'typed ABI arguments differ')
reject('wrong-name-tail-owner',lambda s:s['parameters']['tail'].update(owner='cvpack_main'),'typed ABI arguments differ')
reject('unknown-source-caller',lambda s:s.update(frontier_caller='cvpack_main'),'unmodeled target consumer after main free')
# Generic current-memory remains closed over the retired region. Only the
# independently captured new-generation context can observe this block.
block=native['source_index_context']['block_address'];assert baseline['source_index_context']['new_generation_reuses_retired_regions']
reject('new-block-in-stale-current-namespace',lambda s:s['memory'].update({str(block):s['source_index_context']['block_bytes']}),'released payload was captured as current state')
original=decoder.decode(events);address=original['source_index_module']['address'];raw=events+'memory %08x %s\n'%(address,original['source_index_module']['bytes'].hex())
reject('disjoint-new-block-in-old-current-namespace',lambda s:None,'new generation must remain separate from stale-root current memory',raw)
# Each unwritten slot remains the native run's own value.
changed=copy.deepcopy(native);header(changed,8);sentinel=bytearray.fromhex(changed['source_index_context']['module_sentinel_bytes']);sentinel[0]^=1;changed['source_index_context']['module_sentinel_bytes']=sentinel.hex();compare(changed)
# Force an interior filename pointer in actual original stack bytes.
marker=events.index('frontier 0040f0d0\n');tail=events[marker:];stack=original['esp'];old=original['stack'][1].to_bytes(4,'little').hex();new=(original['stack'][1]+1).to_bytes(4,'little').hex()
match=re.search(r'^memory %08x ([0-9a-f]+)$'%stack,tail,re.M);assert match and match[1][8:16]==old
where=marker+match.start(1)+8;raw=events[:where]+new+events[where+8:]
reject('original-interior-counted-name-pointer',lambda s:None,'original source-name argument lacks exact initialized owner',raw)
# Report-level exact site inventory must reject an aggregate-equivalent forgery.
report=json.loads(audit.read_text());graph=copy.deepcopy(report['graph']);calls=graph['functions']['_build_source_file_index']['register_imports']['original']['resolved_register_calls'];del calls[str(0x40f3e7)]
try:switches.check_import_sites(graph)
except AssertionError as error:
    assert 'source-index cached free sites differ' in str(error);rejected.append(dict(name='omitted-cached-free-site',diagnostic=str(error)))
else:raise AssertionError('accepted missing cachedfree site')
with tempfile.TemporaryDirectory(prefix='cvpack-source-entry-',dir=Path.home()/'.ai-tmp') as temporary:
    work=Path(temporary)
    metadata=json.loads((audit.parent/'graph-metadata.json').read_text());claims=[r for r in metadata['register_imports']['preservation_contracts'] if r['target'].get('import')==dict(dll='msvcrt.dll',name='free')];assert len(claims)==1
    claims[0]['preserves'].remove('edi');path=work/'without-edi.json';path.write_text(json.dumps(metadata,indent=2)+'\n')
    command=report['graph_command'].copy();command[command.index('--metadata')+1]=str(path)
    result=subprocess.run(command,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE);assert result.returncode!=0 and b'_build_source_file_index' in result.stderr,(result.returncode,result.stderr)
    rejected.append(dict(name='free-EDI-bit-removal',diagnostic=result.stderr.decode().strip()))
    scratch=work/'native';shutil.copytree(source,scratch)
    from caseio import prepare
    prepare(json.loads((pair/'original/case-definition.json').read_text()),scratch)
    schema=json.loads((scratch/'configuration.json').read_text());row=next(r for r in schema['functions'] if r['name']=='append_source_name');row['parameters'][0]['native_pointee_width']=2;(scratch/'configuration.json').write_text(json.dumps(schema,indent=2)+'\n')
    commands=scratch/'commands.gdb';commands.write_text(commands.read_text().replace(str(source),str(scratch)));(scratch/'state.json').unlink()
    receipt=json.loads((pair/'original/receipt.json').read_text());env=os.environ.copy()
    for key in ('RECON_TRACE_FD','RECON_TRACE_FILTER','RECON_TRACE_SEEN'):env.pop(key,None)
    env.update(VC510_CVPACK_ARGV0=receipt['canonical_argv0'],RECON_PROGRAM_PATH=receipt['actual_target_windows'],VC510_CVPACK_RESOURCES=str(scratch/'resources.rmr'),VC510_CVPACK_WIDEN=str(scratch/'widen.rwp'))
    begin=time.perf_counter();result=subprocess.run(['gdb','-q','-nx','-batch','-x',str(commands),str(BINARY/'build/native/CVPACK')],cwd=scratch,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15);elapsed=time.perf_counter()-begin
    reason='actual pointer formal pointee extent differs';assert result.returncode!=0 and reason.encode() in result.stderr and not (scratch/'state.json').exists(),(result.returncode,result.stderr)
    rejected.append(dict(name='actual-DWARF-wrong-counted-name-pointee',diagnostic=reason,seconds=elapsed))
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,own_unwritten_offset_and_sentinel_word_positive=True,baseline=baseline['source_index_context'],
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(source/'receipt.json'),audit_sha256=sha(audit),source_sha256=sha(BINARY/'src/source-index.c'),
 checker_sha256=sha(Path(__file__)),context_validator_sha256=sha(BINARY/'scripts/source-index-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(source/'gdb-state.py'),
 reused=dict(path='cases/hello-debug-dll/evidence/0099-finalize-derived-types/type-table-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0099-finalize-derived-types/type-table-checks.json'),scope='Existing12free/historical typed state and overlap guards; fresh source-frame/new-generation/ABI/freeEDI paths tested here.'),seconds=time.perf_counter()-start)
(pair/'source-entry-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
