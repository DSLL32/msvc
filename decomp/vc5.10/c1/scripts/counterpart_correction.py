"""Explicit root-approved reuse of unchanged original execution, no relabeling."""
import copy,json,re,struct
from pathlib import Path
from provenance import PROJECT,ROOT,sha
from allocation_imports import pe_read
PACKET=PROJECT/'cases/minimal/matcher-native-correction.json'
PREFIX='decomp/vc5.10/c1/'

def validate(original):
 if not PACKET.exists():return None
 packet=json.loads(PACKET.read_text());identity=sha(original/'receipt.json')
 if identity not in packet['originals']:return None
 assert packet['kind']=='unchanged original execution; corrected reconstructed counterpart and native observer only'
 row=packet['originals'][identity];assert row['path']==str(original.resolve())and row['receipt_sha256']==identity
 receipt=json.loads((original/'receipt.json').read_text());assert(receipt['kind'],receipt['mode'],receipt['exit'])==('explicit-original-capture','masked',125)
 assert receipt['rebuilt_sha256']==row['old_rebuilt_sha256']
 for name,key in [('audit-binding.json','original_binding_sha256'),('candidate.json','original_candidate_sha256'),('functions.json','original_config_sha256'),('ranges.txt','ranges_sha256'),('source-snapshot.json','helper_source_manifest_sha256'),('invoke-environment.bin','environment_sha256')]:assert sha(original/name)==row[key]
 candidate=json.loads((original/'candidate.json').read_text());assert candidate==json.loads((PROJECT/'cases/minimal/candidate.json').read_text())and candidate['functions'][-1]['name']=='_match_option'
 assert sha(PROJECT/'cases/minimal/trial.json')==row['original_config_sha256']
 assert sha(original/'dll-capture.exe')==receipt['helper_sha256']==row['helper_sha256']==sha(PROJECT/'build/dll-capture.exe')
 for path,value in json.loads((original/'source-snapshot.json').read_text()).items():assert sha(ROOT/path)==sha(Path(value['content']))==value['sha256']
 for path,value in receipt['source_hashes'].items():
  if path!='src/option-matcher.c':assert sha(PROJECT/path)==value
 old=packet['old_source'];current=packet['current_source'];assert sha(Path(old['path']))==old['sha256']==receipt['source_hashes']['src/option-matcher.c']
 assert current['path']=='src/option-matcher.c'and sha(PROJECT/current['path'])==current['sha256']
 oldtext=Path(old['path']).read_text();changes=[("match_option_suffix(pattern, text, '*')",'match_option_suffix(pattern, text, rule->action)'),('                return rule->result;\n            default:','                goto hash_original;\n            default:'),('unsigned marker)','unsigned action)')]
 for a,b in changes:assert oldtext.count(a)==1;oldtext=oldtext.replace(a,b)
 assert oldtext==(PROJECT/current['path']).read_text(),'any additional source correction forbidden'
 meta=packet['old_metadata'];assert sha(Path(meta['path']))==meta['sha256'];before=json.loads(Path(meta['path']).read_text());after=json.loads((PROJECT/'cases/minimal/audit-metadata.json').read_text());expected=copy.deepcopy(before);expected['functions']['_match_option']['rebuilt']=after['functions']['_match_option']['rebuilt'];assert expected==after
 metadata_key=PREFIX+'cases/minimal/audit-metadata.json';assert packet['allowed_audit_deltas'][metadata_key]==dict(old=meta['sha256'],new=sha(PROJECT/'cases/minimal/audit-metadata.json'))
 assert set(packet['allowed_audit_deltas'])=={PREFIX+'build/c1.dll',PREFIX+'build/c1.map',metadata_key}
 for key,value in packet['allowed_audit_deltas'].items():assert sha(ROOT/key)==value['new']
 build=packet['current_native_build'];assert sha(PROJECT/build['path'])==build['sha256']and sha(PROJECT/'build/native/c1-host')==build['elf_sha256']
 audit=packet['current_audit'];assert sha(PROJECT/audit['path'])==audit['sha256']and sha(PROJECT/'build/trial-audit.json')==audit['report_sha256']
 observer=packet['observer'];assert observer['path']=='scripts/option_match_probe.py'and sha(PROJECT/observer['path'])==observer['sha256']
 nd=Path(row['native_directory']);assert sha(nd/'receipt.json')==row['native_receipt_sha256'];nr=json.loads((nd/'receipt.json').read_text());assert nr['native_sha256']==build['elf_sha256']and nr['candidate_sha256']==row['original_candidate_sha256']and nr['probe']==receipt['probe']
 assert json.loads((nd/'system-info-binding.json').read_text())['original_receipt_sha256']==identity
 # Original metadata/data ownership is unchanged; current compiled counterpart
 # tables are independently checked against its actual linked bytes and MAP.
 text=(PROJECT/'build/c1.map').read_text();symbols={name:int(address,16)for name,address in re.findall(r'^\s+0001:\w+\s+(\S+)\s+(\w+)',text,re.M)}
 start,end=symbols['_match_option'],symbols['_match_option_suffix'];table=end-74;raw=pe_read(PROJECT/'build/c1.dll',start-0x10000000,end-start);jump=b'\xff\x24\x8d'+struct.pack('<I',table);assert raw.count(jump)==1
 targets=list(struct.unpack('<7I',pe_read(PROJECT/'build/c1.dll',table-0x10000000,28)));assert all(start<=v<table for v in targets)
 assert after['functions']['_match_option']['rebuilt']==dict(text_data=[[table,end]],switches={str(start+raw.index(jump)):targets})
 return dict(packet_sha256=sha(PACKET),original_receipt_sha256=identity,row=row,packet=packet)

def audit_inputs(original,inputs):
 proof=validate(original)
 if proof is None:return inputs
 result=dict(inputs)
 for key,value in proof['packet']['allowed_audit_deltas'].items():assert result[key]==value['old'];result[key]=value['new']
 return result
