import sys,json,hashlib,time,copy,shutil
from pathlib import Path
root=Path('/root/msvc2/decomp/vc5.10/link');sys.path.insert(0,str(root/'scripts'))
from boundary_state import compare
scratch=Path('/root/.ai-tmp/msvc2/vc5.10/link');out=scratch/'mapped-read-refresh-controls';out.mkdir(exist_ok=False)
o=scratch/'hello-win32-masked-gd8nhj6p';n=scratch/'native-bfqohoxf';frontier='_is_mac_resource_file@4'
original,=[json.loads(x)for x in(o/'events.jsonl').read_text().splitlines()if json.loads(x)['event']=='trap'];native=json.loads((n/'state.json').read_text())
premises={}
for path in sorted((root/'scripts').glob('*.py')):
 data=path.read_bytes();relative=path.relative_to(root);dest=out/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data);premises[str(relative)]=hashlib.sha256(data).hexdigest()
for name in ('mapped-read-contract.json','mapped-read-output-contract.json','archive-signature-literal.json','mac-resource-input-contract.json','input-loader-contract.json','input-reader-contract.json'):
 path=root/'cases/hello-win32'/name;data=path.read_bytes();dest=out/name;dest.write_bytes(data);premises['cases/hello-win32/'+name]=hashlib.sha256(data).hexdigest()
shutil.copyfile(scratch/'mapped-read-state-controls.json',out/'prior-251-controls.json')
start=time.perf_counter();compare(frontier,o,n,original,native);positive=time.perf_counter()-start;checks=[]
mutations=[('wrong RET12',lambda s:s['archive_read_returns'][0].__setitem__('return_stack',0)),('wrong signature output',lambda s:s['archive_read_returns'][0]['output'].__setitem__('after','00'*8)),('wrong live record',lambda s:s['file_records'].__setitem__(14,s['file_records'][14][:56]+'00000000'+s['file_records'][14][64:])),('wrong AL',lambda s:s['archive_returns'][0].__setitem__('raw_result',s['archive_returns'][0]['raw_result']|1)),('missing reader return',lambda s:s['archive_read_returns'].clear()),('wrong physical frontier input',lambda s:s['mac_resource_entry']['formals'].__setitem__(0,15))]
negative=time.perf_counter()
for label,change in mutations:
 state=copy.deepcopy(native);change(state)
 try:compare(frontier,o,n,original,state)
 except (AssertionError,KeyError):checks.append(label);continue
 raise AssertionError(label)
report=dict(original=str(o),native=str(n),positive=True,fresh_negatives=checks,fresh_count=len(checks),reused_count=251,reused_receipt_sha256=hashlib.sha256((out/'prior-251-controls.json').read_bytes()).hexdigest(),reuse_basis='Exact unchanged predicates/generator/contracts frozen here after the prior successful 251-control campaign; shared-header-only native rebuild. No retrospective rerun or invented elapsed.',premise_sha256=premises,elapsed_seconds=dict(positive=positive,negative=time.perf_counter()-negative,total=time.perf_counter()-start))
(out/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['elapsed_seconds']));print(out)
