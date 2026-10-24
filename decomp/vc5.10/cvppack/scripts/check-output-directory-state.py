#!/usr/bin/env python3
"""Current output versus input-history directory and callback ABI controls."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0102-packed-file-writer-sort'
def module(name):
 spec=importlib.util.spec_from_file_location(name.replace('-','_'),BINARY/'scripts'/(name+'.py'));result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
decoder=module('raw-state');switches=module('switch-data')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);rejected=[]
def reject(name,mutate,reason,raw=events):
 state=copy.deepcopy(native);mutate(state)
 try:compare(state,raw)
 except (AssertionError,KeyError,IndexError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('wrong-history-row-count',lambda s:s['directory_before_writer'].update(count=69),'historical input directory owner/capacity differs')
reject('wrong-history-allocation-capacity',lambda s:s['directory_before_writer'].update(capacity=900),'historical input directory owner/capacity differs')
reject('wrong-live-sort-owner',lambda s:s['directory_sort_input'].update(address=0),'output directory actual owner aliases differ')
reject('wrong-sort-element-count',lambda s:s['directory_sort_input'].update(count=57),'output directory qsort count/width differs')
reject('wrong-sort-element-width',lambda s:s['directory_sort_input'].update(width=8),'output directory qsort count/width differs')
reject('wrong-native-comparator-identity',lambda s:s['directory_sort_input'].update(callback=0),'output directory actual typed comparator identity differs')
reject('truncated-sort-input-capacity',lambda s:s['directory_sort_input'].update(bytes=s['directory_sort_input']['bytes'][:-2]),'output directory complete initialized sort input missing')
def byte(state,key,offset):
 raw=bytearray.fromhex(state[key]['bytes']);raw[offset]^=1;state[key]['bytes']=raw.hex()
reject('changed-writer-module-kind',lambda s:byte(s,'directory_sort_input',0),'output directory rebuilt rows differ from actual live module inputs')
reject('changed-writer-owned-data-pointer',lambda s:byte(s,'directory_sort_input',4),'output directory rebuilt rows differ from actual live module inputs')
reject('changed-writer-retained-history-tail',lambda s:byte(s,'directory_sort_input',839),'output directory writer changed retained initialized tail')
root=int.from_bytes(bytes.fromhex(native['globals']['directory_entries']),'little')
def current_byte(state,offset):
 raw=bytearray.fromhex(state['memory'][str(root)]);raw[offset]^=1;state['memory'][str(root)]=raw.hex()
reject('changed-current-row-live-owner',lambda s:current_byte(s,4),'output directory current row lacks actual live input owner')
def reorder(state):
 raw=bytearray.fromhex(state['memory'][str(root)]);raw[12:24],raw[24:36]=raw[24:36],raw[12:24];state['memory'][str(root)]=raw.hex()
reject('different-callback-prefix-permutation',reorder,'canonical/native qsort first-callback directory permutation differs')
reject('changed-inactive-current-tail',lambda s:current_byte(s,839),'qsort changed inactive initialized input tail')
reject('truncated-current-directory',lambda s:s['memory'].update({str(root):s['memory'][str(root)][:-2]}),'output directory current initialized capacity incomplete')
reject('changed-callback-argument-role',lambda s:s['parameters']['first'].update(index=2),'output directory first callback argument roles differ')
reject('unknown-callback-owner',lambda s:s.update(frontier_caller='cvpack_main'),'unmodeled target consumer after main free')
line=next(line for line in events.splitlines() if line.startswith('directory-qsort-arguments '));fields=line.split();fields[3]='8';raw=events.replace(line,' '.join(fields),1)
reject('raw-original-wrong-qsort-width',lambda s:None,'output directory qsort count/width differs',raw)
fields=line.split();fields[4]='004063f4';raw=events.replace(line,' '.join(fields),1)
reject('raw-original-wrong-qsort-callback',lambda s:None,'output directory actual typed comparator identity differs',raw)
# Full-width original argument slots, not copied frontier metadata alone.
original=decoder.decode(events);marker=events.index('frontier 004063f0\n');match=re.search(r'^memory %08x ([0-9a-f]+)$'%original['esp'],events[marker:],re.M);assert match
where=marker+match.start(1)+8;replacement=(original['stack'][1]+12).to_bytes(4,'little').hex();raw=events[:where]+replacement+events[where+8:]
reject('raw-original-different-callback-row',lambda s:None,'typed ABI arguments differ',raw)
# Import inventories are checked per site in addition to whole graph sets.
report=json.loads((pair/'original/audit.json').read_text());graph=copy.deepcopy(report['graph']);del graph['functions']['_flush_packed_file']['register_imports']['original']['resolved_register_calls'][str(0x406e3e)]
try:switches.check_import_sites(graph)
except AssertionError as error:
 assert 'writer cached printf site inventory differs' in str(error);rejected.append(dict(name='omitted-second-cached-printf-site',diagnostic=str(error)))
else:raise AssertionError('accepted missing secondprintf')
with tempfile.TemporaryDirectory(prefix='cvpack-output-directory-',dir=Path.home()/'.ai-tmp') as temporary:
 work=Path(temporary);scratch=work/'native';shutil.copytree(pair/'native',scratch)
 from caseio import prepare
 prepare(json.loads((pair/'original/case-definition.json').read_text()),scratch)
 schema=json.loads((scratch/'configuration.json').read_text());row=next(r for r in schema['functions'] if r['name']=='compare_packed_directory_entries');row['parameters'].append(dict(name='third',kind='integer',slot=3));(scratch/'configuration.json').write_text(json.dumps(schema,indent=2)+'\n')
 commands=scratch/'commands.gdb';commands.write_text(commands.read_text().replace(str(pair/'native'),str(scratch)));(scratch/'state.json').unlink()
 receipt=json.loads((pair/'original/receipt.json').read_text());env=os.environ.copy()
 for key in ('RECON_TRACE_FD','RECON_TRACE_FILTER','RECON_TRACE_SEEN'):env.pop(key,None)
 env.update(VC510_CVPACK_ARGV0=receipt['canonical_argv0'],RECON_PROGRAM_PATH=receipt['actual_target_windows'],VC510_CVPACK_RESOURCES=str(scratch/'resources.rmr'),VC510_CVPACK_WIDEN=str(scratch/'widen.rwp'))
 begin=time.perf_counter();result=subprocess.run(['gdb','-q','-nx','-batch','-x',str(commands),str(BINARY/'build/native/CVPACK')],cwd=scratch,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15);elapsed=time.perf_counter()-begin
 reason='incomplete frontier ABI contract';assert result.returncode!=0 and reason.encode() in result.stderr and not (scratch/'state.json').exists(),(result.returncode,result.stderr)
 rejected.append(dict(name='actual-DWARF-fictitious-third-argument',diagnostic=reason,seconds=elapsed))
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,baseline=baseline['output_directory'],seconds=time.perf_counter()-start,
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),audit_sha256=sha(pair/'original/audit.json'),checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/output-directory-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),source_sha256=sha(BINARY/'src/writer.c'),
 reused=dict(source_family_controls=dict(path='cases/hello-debug-dll/evidence/0101-append-source-name-complete/source-complete-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0101-append-source-name-complete/source-complete-checks.json'),scope='29 new-generation/temp-free/own-unwritten/output controls remain; new directory/history/callback paths fresh here.'),edi_ablation=dict(path='cases/hello-debug-dll/writer-preservation-diagnostic.json',sha256=sha(BINARY/'cases/hello-debug-dll/writer-preservation-diagnostic.json'),scope='12 reviewed EDI single-bit removal controls; static-only snapshots/qualified claims retained.')))
(pair/'directory-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
