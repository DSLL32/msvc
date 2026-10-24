"""One genuine public cdecl callee preserves the cached ESI import identity."""
import json
from provenance import PROJECT,ROOT,sha
from allocation_imports import pe_read,export_rva

def check_static():
 p=json.loads((PROJECT/'cases/minimal/resolver-imports.json').read_text())
 for name in ('target','provider','sdk'):assert sha(ROOT/p[name]['path'])==p[name]['sha256']
 provider=ROOT/p['provider']['path'];target=ROOT/p['target']['path'];body=p['provider']
 assert body['export_rva']==export_rva(provider,'_mbsstr')==0x24638 and body['end_rva']==0x246d4
 assert pe_read(provider,0x24638,156).hex()==body['bytes']
 assert pe_read(provider,0x2463f,1)==b'\x56'
 for address in (0x24658,0x246c2,0x246c9,0x246ce):assert pe_read(provider,address,1)==b'\x5e'
 assert p['sdk']['declaration']in(ROOT/p['sdk']['path']).read_text()
 assert p['caller']==dict(name='_resolve_option_path',code_chunks=[[0x1335b,0x1344d],[0x30a3c,0x30af0]],iat_rva=0x5d044,load_rva=0x30a65,call_rvas=[0x30a75,0x30a99],preserved_register='esi',saved_at_export_offset=7,restored_at_return_rvas=[0x24658,0x246c2,0x246c9,0x246ce])
 assert pe_read(target,0x30a65,6)==bytes.fromhex('8b3544d06510')
 assert all(pe_read(target,address,2)==b'\xff\xd6'for address in (0x30a75,0x30a99))
 return p

def check_report(report):
 r=report['functions']['_resolve_option_path']['register_imports']
 assert r['require_all_register_calls_resolved']is True
 assert r['original']['resolved_register_calls']=={str(0x10600000+site):dict(dll='msvcrt.dll',name='_mbsstr')for site in (0x30a75,0x30a99)}
 for side in ('original','rebuilt'):assert r[side]['unresolved_register_calls']==r[side]['other_unresolved_calls']==[]
 assert r['rebuilt']['resolved_register_calls']=={}
