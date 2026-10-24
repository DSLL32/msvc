#!/usr/bin/env python3
"""Fresh scanner/API checks, reusing only an unchanged validated older prefix."""
import argparse,copy,hashlib,json,tempfile,time
from pathlib import Path
import checksum_state,multibyte_state
from provenance import sha

parser=argparse.ArgumentParser();parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args();started=time.perf_counter()
project=Path(__file__).resolve().parents[1];candidate=json.loads((args.original/'candidate.json').read_text());report=json.loads((project/'build/trial-audit.json').read_text())
positive=multibyte_state.validate(args.original,args.native,candidate,report)
# The complete old-prefix validator was run immediately above. Mutations below
# change ONLY these new-component files, none read by that earlier validator.
mutable=('locale-native-records.json','scanner-init-entries.json','scanner-init-returns.json','state.json',
 'locale-native-mb-after-mbctype.bin','locale-native-after-ctype.bin','locale-native-after-old-ctype.bin',
 'locale-native-next-result.bin','locale-native-next-CRT-environment.bin')
prefix_hashes={f.name:sha(f)for f in args.native.iterdir()if f.is_file()and f.name not in mutable}
prefix_result={'status':'matching genuine public CRT entry; no locale behavior accepted'}
original_validator=checksum_state.validate
reused=0

def unchanged_prefix(original,native,current,current_report,public_state=None):
 global reused
 assert original==args.original and current==candidate and current_report==report
 assert all(sha(native/name)==digest for name,digest in prefix_hashes.items())
 assert public_state==json.loads((args.native/'locale-native-setlocale-entry-state.json').read_text())
 reused+=1;return dict(prefix_result)

labels=['classes-last-byte','flags-last-byte','saved-table-byte','omitted-API','duplicate-API','MB-codepage','MB-gate','locale-codepage','MBmax','MB-full-tail','ctype-EOF','old-table-retained','result-lifetime','locale-formal-omitted','MB-formal-type','locale-raw-argument','MB-raw-argument','wrong-provider-entry','API-stack-cleanup','MB-return-result','locale-result-owner','scanner-formal','scanner-entry','scanner-stack-cleanup','scanner-callee-save','Invoke-argc','private-env-terminator']
try:
 checksum_state.validate=unchanged_prefix
 with tempfile.TemporaryDirectory(prefix='multibyte-state-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as temporary:
  directory=Path(temporary)
  for file in args.native.iterdir():
   if file.is_file():(directory/file.name).symlink_to(file.resolve())
  baseline={name:(args.native/name).read_bytes()for name in mutable}
  for label in labels:
   for name,raw in baseline.items():
    path=directory/name
    if path.is_symlink():path.unlink()
    path.write_bytes(raw)
   records=json.loads(baseline['locale-native-records.json']);state=json.loads(baseline['state.json']);entries=json.loads(baseline['scanner-init-entries.json']);returns=json.loads(baseline['scanner-init-returns.json'])
   if label in ('classes-last-byte','flags-last-byte','saved-table-byte'):
    key='classes'if label.startswith('classes')else'flags';region='snapshot'if label=='saved-table-byte'else'initialized';state['compiler_state']['multibyte_state'][region][key][-1]^=1
   elif label=='omitted-API':records.pop(2)
   elif label=='duplicate-API':records.insert(2,copy.deepcopy(records[2]))
   elif label=='MB-codepage':records[3]['mb_codepage']=0
   elif label=='MB-gate':records[3]['mb_gate']=1
   elif label=='locale-codepage':records[1]['lc_codepage']=0
   elif label=='MBmax':records[3]['mb_cur_max']=2
   elif label in ('MB-full-tail','ctype-EOF','old-table-retained','result-lifetime','private-env-terminator'):
    name={'MB-full-tail':'locale-native-mb-after-mbctype.bin','ctype-EOF':'locale-native-after-ctype.bin','old-table-retained':'locale-native-after-old-ctype.bin','result-lifetime':'locale-native-next-result.bin','private-env-terminator':'locale-native-next-CRT-environment.bin'}[label]
    raw=bytearray(baseline[name]);raw[-1 if label in('MB-full-tail','private-env-terminator')else 0]^=1;(directory/name).write_bytes(raw)
   elif label=='locale-formal-omitted':records[0]['formal_inventory'].pop()
   elif label=='MB-formal-type':records[2]['formal_inventory'][0]['type']='unsigned int'
   elif label=='locale-raw-argument':records[0]['words'][1]=1
   elif label=='MB-raw-argument':records[2]['words'][1]=0xfffffffd
   elif label=='wrong-provider-entry':records[2]['pc']+=1
   elif label=='API-stack-cleanup':records[3]['esp']+=4
   elif label=='MB-return-result':records[3]['eax']=1
   elif label=='locale-result-owner':records[1]['addresses']['result']+=1
   elif label=='scanner-formal':entries[0]['formal_inventory']=[dict(name='invented',type='int')]
   elif label=='scanner-entry':entries[0]['pc']+=1
   elif label=='scanner-stack-cleanup':returns[0]['esp']+=4
   elif label=='scanner-callee-save':returns[0]['preserved']['esi']^=1
   elif label=='Invoke-argc':records[-1]['invocation']['args']['argc']-=1
   else:raise AssertionError(label)
   for name,value in [('locale-native-records.json',records),('state.json',state),('scanner-init-entries.json',entries),('scanner-init-returns.json',returns)]:
    (directory/name).write_text(json.dumps(value))
   try:multibyte_state.validate(args.original,directory,candidate,report)
   except(AssertionError,ValueError,KeyError,IndexError):continue
   raise AssertionError('accepted '+label)
finally:checksum_state.validate=original_validator
print(json.dumps(dict(status='PASS',fresh_controls=len(labels),rejected=labels,reused_unchanged_prefix_checks=reused,prefix_inputs_sha256=hashlib.sha256(json.dumps(prefix_hashes,sort_keys=True).encode()).hexdigest(),seconds=time.perf_counter()-started,scope='One fresh full positive then new scanner/API semantic denials only; unchanged older-prefix files remain hash-identical and reuse that passing proof'),indent=2))
