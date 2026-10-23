"""Focused read-only rejection checks for the single decimal CRT binding."""
import copy,hashlib,json,tempfile,time
from pathlib import Path
import decimal_variant as binding
from binary import PE
from case_paths import CAPTURE_ROOT
started=time.perf_counter();row=binding.default_record();results=[]
assert binding.verify(row)['application_credit']==0
results.append(dict(test='actual-binding',result='verified'))
original=PE(binding.TC/'bin/C1XX3232.EXE')
class WrongBody:
 def __getattr__(self,name):return getattr(original,name)
 def read(self,address,count):
  data=original.read(address,count)
  if address==0x88520 and count==0x720:return bytes([data[0]^1])+data[1:]
  return data
try:binding.verify(row,original=WrongBody())
except AssertionError as error:results.append(dict(test='wrong-original-body',result='held',reason=str(error)))
else:raise AssertionError('wrong body passed')
directory=Path(tempfile.mkdtemp(prefix='decimal-compatible-negatives-',dir=CAPTURE_ROOT));proof=binding.BINARY/row['compatible_proof']
for name in ['wrong-callee','stale-proof']:
 value=json.loads(proof.read_text());changed=copy.deepcopy(row)
 if name=='wrong-callee':value['same_distinct_callees'][0]='_invented_callee'
 else:value['public_contract']['status']=1
 path=directory/(name+'.json');path.write_text(json.dumps(value,indent=2)+'\n')
 if name=='wrong-callee':changed['compatible_proof_sha256']=hashlib.sha256(path.read_bytes()).hexdigest()
 try:binding.verify(changed,proof_path=path)
 except AssertionError as error:results.append(dict(test=name,result='held',reason=str(error)))
 else:raise AssertionError(name+' passed')
receipt=dict(scope='Actual specific binding plus three bounded synthetic metadata/body negatives; read-only, no original execution or acceptance',proof_sha256=binding.sha(proof),checks=results,elapsed_seconds=time.perf_counter()-started)
(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(directory/'receipt.json')
