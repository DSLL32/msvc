"""Finite EBP import-liveness evidence for the encountered path initializer."""
import json
from provenance import PROJECT,ROOT,sha
from allocation_imports import pe_read,export_rva

def check_static():
 packet=json.loads((PROJECT/'cases/minimal/path-fold-imports.json').read_text())
 for key in ('target','provider','sdk'):
  item=packet[key];assert sha(ROOT/item['path'])==item['sha256']
 target=ROOT/packet['target']['path'];provider=ROOT/packet['provider']['path']
 assert packet['sdk']['declaration'] in (ROOT/packet['sdk']['path']).read_text()
 caller=packet['caller'];assert caller['hot']==[0x12f3e,0x12f87]and caller['cold']==[0x307d2,0x307dc]
 assert pe_read(target,0x12f3e,73)==bytes.fromhex(caller['bytes'])
 assert pe_read(target,0x307d2,10)==bytes.fromhex(caller['cold_bytes'])
 assert pe_read(provider,0xa5d3,54)==bytes.fromhex(packet['finite_preservation']['provider_hot_bytes'])
 assert export_rva(provider,'_mbctolower')==0xa5d3
 assert packet['finite_preservation']['register']=='ebp'and packet['finite_preservation']['input_domain']==[0,255]
 assert packet['import_identity']['iat_rva']==0x5d05c and packet['import_identity']['import_by']=='name'
 assert pe_read(target,0x12f40,6)==bytes.fromhex('8b2d5cd06510')and pe_read(target,0x12f65,2)==b'\xff\xd5'
 return packet

def check_report(report):
 result=report['functions']['_initialize_path_fold_table']['register_imports']
 assert result['require_all_register_calls_resolved']is True
 assert result['original']['resolved_register_calls']=={str(0x10612f65):dict(dll='msvcrt.dll',name='_mbctolower')}
 for side in ('original','rebuilt'):
  assert result[side]['unresolved_register_calls']==result[side]['other_unresolved_calls']==[]
 assert result['rebuilt']['resolved_register_calls']=={}
 assert list(result['rebuilt']['direct_import_calls'].values())==[dict(dll='msvcrt.dll',name='_mbctolower')]
