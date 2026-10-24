"""Private native S tree bound to the original child's actual logical cwd."""
import json
from pathlib import Path
from provenance import PROJECT,ROOT,sha

def prepare(original,output,environment):
 receipt=json.loads((original/'receipt.json').read_text())
 assert receipt['kind']=='explicit-original-capture'and receipt['probe']=='normal'and receipt['exit']==125
 raw=(original/'fullpath-entry-cwd.bin').read_bytes();assert len(raw)%2==0 and raw[-2:]==b'\0\0'
 logical=raw[:-2].decode('utf-16le');assert logical[:3].upper()=='S:\\'and '\0'not in logical
 suffix=logical[3:].replace('\\','/').strip('/')
 assert suffix.startswith('vc5.10/c1/')and all(part not in ('','.','..')for part in suffix.split('/'))
 root=output/'drive-S';work=root/suffix;work.mkdir(parents=True,exist_ok=False)
 source=original/'test.c';expected=PROJECT/'cases/minimal/inputs/test.c';assert sha(source)==sha(expected)
 target=work/'test.c';target.write_bytes(source.read_bytes());assert sha(target)==sha(source)
 environment['RECON_DRIVE_S']=str(root);environment['RECON_DRIVE_R']=str(ROOT);environment['RECON_CANONICAL_CWD']=logical
 assert work.resolve().is_relative_to(output.resolve())and not work.resolve().is_relative_to(original.resolve())
 record=dict(schema=1,kind='native-only private S namespace with actual original logical cwd',original_receipt_sha256=sha(original/'receipt.json'),original_cwd_sha256=sha(original/'fullpath-entry-cwd.bin'),logical_cwd=logical,physical_cwd=str(work),private_S_root=str(root),R_root=str(ROOT),inputs=[dict(logical_path=logical.rstrip('\\')+'\\test.c',original_path=str(source),physical_path=str(target),sha256=sha(target))],unknown_S_paths='private tree only; no fallback to original raw directory',host_controls='RECON_* stay outside bound target CRT namespace; Wine prefix and R/S mappings untouched')
 (output/'native-path-binding.json').write_text(json.dumps(record,indent=2)+'\n');return work
