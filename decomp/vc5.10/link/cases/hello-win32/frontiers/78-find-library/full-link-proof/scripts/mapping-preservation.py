"""One ordinary WINAPI EDI contract, with all discovery delegated to Rust."""
import hashlib
import json
from pathlib import Path
import struct
from binary import PE,u16,u32
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
OWNER='_initialize_file_mapping@12'
PACKET=BINARY/'cases/hello-win32/mapping-import-preservation.json'


def sha(data):return hashlib.sha256(data).hexdigest()


def check(original,packet=None):
    packet=json.loads(PACKET.read_text())if packet is None else packet
    assert packet['schema']==1 and packet['owner']==OWNER
    assert sha(original.data)==packet['original_sha256']
    body=packet['body'];assert(body['start'],body['end'])==(0x40ccf0,0x40cdef)
    assert sha(original.read(body['start'],255))==body['sha256']
    assert packet['identity']=='kernel32.dll!CloseHandle'and packet['preserves']==['edi']
    provider=packet['provider'];p=BINARY/provider['path'];data=p.read_bytes()
    assert len(data)==provider['bytes']and sha(data)==provider['sha256']
    receipt=BINARY/provider['receipt'];assert sha(receipt.read_bytes())==provider['receipt_sha256']
    assert json.loads(receipt.read_text())['files'][p.name]==dict(bytes=len(data),sha256=sha(data))
    assert provider['module_index']==2 and p.name=='module-002.bin'
    pe=PE(p);directory=pe.base+u32(pe.data,pe.optional+96)
    assert pe.string(pe.base+u32(pe.read(directory+12,4),0))==provider['dll_name']=='KERNEL32.dll'
    base,_,count,functions,names,ordinals=struct.unpack_from('<6I',pe.read(directory,40),16)
    entries=[]
    for i in range(count):
        name=pe.string(pe.base+u32(pe.read(pe.base+names+i*4,4),0))
        if name=='CloseHandle':
            ordinal=u16(pe.read(pe.base+ordinals+i*2,2),0)
            entries.append((base+ordinal,u32(pe.read(pe.base+functions+4*ordinal,4),0)))
    assert entries==[(provider['export_ordinal'],provider['export_rva'])]
    assert pe.read(pe.base+entries[0][1],16).hex()==provider['entry_bytes']and provider['forwarder']is None
    required=[('toolchains/5.10/include/WINBASE.H',[2403,2404,2405,2406,2407,2408]),('toolchains/5.10/include/WINDEF.H',[100])]
    assert len(packet['declarations'])==2
    for decl,(path,numbers)in zip(packet['declarations'],required):
        assert decl['path']==path
        data=(ROOT/path).read_bytes();assert sha(data)==decl['sha256'];lines=data.decode('latin1').splitlines()
        assert decl['lines']==[dict(line=i,text=lines[i-1])for i in numbers]
    assert ''.join(x['text'].strip()for x in packet['declarations'][0]['lines'])=='WINBASEAPIBOOLWINAPICloseHandle(HANDLE hObject);'
    assert packet['declarations'][1]['lines'][0]['text'].strip()=='#define WINAPI      __stdcall'
    assert packet['original_cached_sites']==[0x40cdb5,0x40cdbb]
    assert packet['original_direct_sites']==[0x40cd0f,0x40cd1e,0x40cd44,0x40cd50,0x40cd60,0x40cd95,0x40cda2,0x40cdc9,0x40cdd3]
    return dict(qualified_import_contracts=1,preserved_register_bits=1,provider_sha256=provider['sha256'])


def contracts():
    return [dict(target={'import':dict(dll='kernel32.dll',name='CloseHandle')},preserves=['edi'],evidence=dict(path='cases/hello-win32/mapping-import-preservation.json',sha256=sha(PACKET.read_bytes())))]


def check_resolution(report,rebuilt):
    packet=json.loads(PACKET.read_text());bound=report['register_import_inputs']
    assert bound['original_sha256']==packet['original_sha256']and bound['rebuilt_sha256']==sha(rebuilt.data)
    assert bound['preservation_contracts']==contracts()
    sides=report['functions'][OWNER]['register_imports'];left=sides['original'];right=sides['rebuilt']
    for side in (left,right):assert side['unresolved_register_calls']==side['other_unresolved_calls']==[]
    assert set(map(int,left['resolved_register_calls']))==set(packet['original_cached_sites'])
    assert set(map(int,left['direct_import_calls']))==set(packet['original_direct_sites'])
    assert not right['resolved_register_calls']and len(right['direct_import_calls'])==11
    assert all(i==dict(dll='kernel32.dll',name='CloseHandle')for i in left['resolved_register_calls'].values())
    def names(calls):return sorted(v['dll']+'!'+v['name']for v in calls.values())
    assert sorted(names(left['resolved_register_calls'])+names(left['direct_import_calls']))==names(right['direct_import_calls'])
    return dict(original_cached=2,original_direct=9,rebuilt_direct=11,opaque=0)
