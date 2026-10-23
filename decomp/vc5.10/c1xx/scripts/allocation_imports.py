"""Finite VirtualAlloc/ordinary wrapper witness; common Rust owns CFG/dataflow."""
import hashlib,json,struct,subprocess,re
from functools import lru_cache
from pathlib import Path
P=Path(__file__).resolve().parents[1];ROOT=P.parents[2]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
@lru_cache(maxsize=8)
def pe_data(path):return path.read_bytes()
def pe_read(path,rva,size):
 data=pe_data(path);pe=struct.unpack_from('<I',data,0x3c)[0];n=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0]
 for i in range(n):
  at=pe+24+optional+i*40;virtual,rawsize,raw=struct.unpack_from('<III',data,at+12)
  if virtual<=rva and rva+size<=virtual+rawsize:return data[raw+rva-virtual:raw+rva-virtual+size]
 raise AssertionError('unbacked PE bytes')
def export_rva(path,wanted):
 data=path.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0];export=struct.unpack_from('<I',data,pe+24+96)[0]
 directory=pe_read(path,export,40);count=struct.unpack_from('<I',directory,24)[0];functions,names,ordinals=struct.unpack_from('<III',directory,28)
 for i in range(count):
  name_rva=struct.unpack('<I',pe_read(path,names+4*i,4))[0];text=pe_read(path,name_rva,512).split(b'\0',1)[0]
  if text.decode('ascii')==wanted:
   ordinal=struct.unpack('<H',pe_read(path,ordinals+2*i,2))[0]
   return struct.unpack('<I',pe_read(path,functions+4*ordinal,4))[0]
 raise AssertionError('missing export')

def check_static():
 packet=json.loads((P/'cases/minimal/allocation-preservation-v1.json').read_text());assert packet['schema']==1
 target=ROOT/'toolchains/5.10/bin/C1XX.DLL';assert packet['target_sha256']==sha(target)
 sdk=packet['sdk'];header=ROOT/sdk['path'];assert sha(header)==sdk['sha256']
 assert '\n'.join(header.read_text().splitlines()[sdk['first_line']-1:sdk['last_line']])==sdk['text']
 assert sdk['text']=='WINBASEAPI\nLPVOID\nWINAPI\nVirtualAlloc(\n    LPVOID lpAddress,\n    DWORD dwSize,\n    DWORD flAllocationType,\n    DWORD flProtect\n    );\n'
 for row in packet['providers']:
  file=Path(row['path']);assert file.stat().st_size==row['bytes'] and sha(file)==row['sha256']
  assert pe_read(file,row['rva'],len(bytes.fromhex(row['entry_bytes'])))==bytes.fromhex(row['entry_bytes'])
 assert packet['fatal']['bytes']=='8b44240450e84600000083c404c3'
 assert pe_read(target,0x91a45,14)==bytes.fromhex(packet['fatal']['bytes'])
 return packet

def check_report(report):
 metadata=json.loads((P/'graph-metadata.json').read_text())
 assert report['register_import_inputs']['preservation_contracts']==metadata['register_imports']['preservation_contracts']
 function=report['functions']['_reserve_work_buffer'];result=function['register_imports']
 assert result['require_all_register_calls_resolved'] is True
 assert result['original']['resolved_register_calls']=={str(0x10438cdf):dict(dll='kernel32.dll',name='VirtualAlloc'),str(0x10438cf8):dict(dll='kernel32.dll',name='VirtualAlloc')}
 assert result['original']['unresolved_register_calls']==result['original']['other_unresolved_calls']==[]
 assert result['rebuilt']['resolved_register_calls']=={} and result['rebuilt']['unresolved_register_calls']==result['rebuilt']['other_unresolved_calls']==[]
 assert len(result['rebuilt']['direct_import_calls'])==2
 assert all(row==dict(dll='kernel32.dll',name='VirtualAlloc')for row in result['rebuilt']['direct_import_calls'].values())
