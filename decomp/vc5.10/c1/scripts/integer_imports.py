"""Finite cached-public-import identity/liveness, no imported-body credit."""
import copy,hashlib,json
from pathlib import Path
from provenance import ROOT,PROJECT,sha
from allocation_imports import pe_read,export_rva

def check_static():
 p=json.loads((PROJECT/'cases/minimal/integer-imports.json').read_text())
 validate_correction(p)
 for key in ('target','provider','sdk'):assert sha(ROOT/p[key]['path'])==p[key]['sha256']
 target=ROOT/p['target']['path'];provider=ROOT/p['provider']['path'];sdk=ROOT/p['sdk']['path']
 assert p['sdk']['declaration']in sdk.read_text()
 caller=p['caller'];assert caller['hot']==[0x132f9,0x1335b]and caller['cold']==[0x30a17,0x30a3c]and caller['register']=='edi'
 assert pe_read(target,0x132f9,98).hex()==caller['hot_bytes']and pe_read(target,0x30a17,37).hex()==caller['cold_bytes']
 assert pe_read(target,0x13306,6)==bytes.fromhex('8b3d2cd16510')and all(pe_read(target,r,2)==b'\xff\xd7'for r in (0x13311,0x30a28))
 finite=p['finite_preservation'];assert finite['domain']==dict(MB_CUR_MAX=1,input_unsigned_byte=[0,255])and finite['register']=='edi'
 assert finite['provider_export_rva']==export_rva(provider,'isspace')==0x1c0be
 assert pe_read(provider,0x1c0be,42).hex()==finite['provider_bytes']
 witness=finite['existing_own_SBCS_reference'];assert sha(PROJECT/witness['path'])==witness['sha256']
 return p

def derive_correction(old,current,packet):
 assert packet['kind']=='unexecuted static public import correction'and packet['allowed_JSON_paths']==['other_public_imports[1]']
 expected=copy.deepcopy(old);assert expected['other_public_imports'][1]==packet['old_record']
 expected['other_public_imports'][1]=packet['new_record'];assert expected==current,'any delta beyond the unexecuted atoi-to-atol label is forbidden'
 assert packet['new_record']['name']=='atol'and packet['new_record']['iat_rva']==0x5d07c and packet['new_record']['provider_rva']==0x5d30
 assert hashlib.sha256(json.dumps(current['finite_preservation'],sort_keys=True).encode()).hexdigest()==packet['unchanged_isspace_witness_sha256']
 return current

def validate_correction(current):
 packet=json.loads((PROJECT/'cases/minimal/integer-imports-derivation.json').read_text());old=Path(packet['old_path'])
 assert sha(old)==packet['old_sha256']and sha(PROJECT/packet['new_path'])==packet['new_sha256']
 assert sha(Path(packet['parent_review_path']))==packet['parent_review_sha256']
 derive_correction(json.loads(old.read_text()),current,packet)

def check_report(report):
 r=report['functions']['_parse_option_integer']['register_imports'];assert r['require_all_register_calls_resolved']is True
 assert r['original']['resolved_register_calls']=={str(0x10600000+site):dict(dll='msvcrt.dll',name='isspace')for site in (0x13311,0x30a28)}
 for side in ('original','rebuilt'):assert r[side]['unresolved_register_calls']==r[side]['other_unresolved_calls']==[]
 assert r['rebuilt']['resolved_register_calls']=={}
 assert sorted(i['name']for i in r['rebuilt']['direct_import_calls'].values())==['atol','isdigit','isspace']
