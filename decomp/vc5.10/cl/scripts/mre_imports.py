"""Exact declaration-only MSPDB50 boundary; no successful native API response."""
import argparse
import hashlib
import importlib.machinery
import json
from pathlib import Path
import shutil
import struct
import subprocess
from binary import PE
from import_residuals import qualified_slots

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def declarations(project=PROJECT,root=ROOT):
    proof=json.loads((project/'cases/minimal-c/mre-imports.json').read_text())
    target=PE(root/'toolchains/5.10/bin/CL.EXE');provider=PE(root/proof['provider'])
    assert hashlib.sha256(target.data).hexdigest()==proof['target_sha256']
    assert hashlib.sha256(provider.data).hexdigest()==proof['provider_sha256']
    start=provider.base+struct.unpack_from('<I',provider.data,provider.optional+96)[0]
    table=struct.unpack('<IIHH7I',provider.read(start,40));exports={}
    for i in range(table[7]):
        name=provider.string(provider.base+struct.unpack('<I',provider.read(provider.base+table[9]+4*i,4))[0])
        ordinal=struct.unpack('<H',provider.read(provider.base+table[10]+2*i,2))[0]
        rva=struct.unpack('<I',provider.read(provider.base+table[8]+4*ordinal,4))[0]
        exports[name]=(ordinal+table[5],rva)
    slots=qualified_slots(target)
    for row in proof['exports']:
        assert exports[row['export']]==(row['ordinal'],row['start_rva'])
        body=provider.read(provider.base+row['start_rva'],row['end_rva']-row['start_rva'])
        assert hashlib.sha256(body).hexdigest()==row['body_sha256']
        assert body[-3:]==b'\xc2'+struct.pack('<H',row['argument_bytes']),'external stdcall cleanup differs'
        assert slots[row['target_iat']]=={'dll':'mspdb50.dll','name':row['export']}
    expected='LIBRARY MSPDB50.DLL\nEXPORTS\n'+''.join('    %s @%d\n'%(r['export'],r['ordinal'])for r in proof['exports'])
    assert (project/'scripts/mspdb50.def').read_text()==expected,'declaration-only export set differs'
    return proof


def verify(project,root):
    proof=declarations(project,root)
    actual={r['name']for r in qualified_slots(PE(project/'build/cl.exe')).values()if r['dll']=='mspdb50.dll'}
    assert actual=={r['export']for r in proof['exports']},'rebuilt qualified MSPDB imports differ'
    flag=proof['first_dependency_scan']
    assert PE(root/'toolchains/5.10/bin/CL.EXE').read(flag['original_address'],4)==struct.pack('<I',flag['initial_dword'])
    identity=importlib.machinery.SourceFileLoader('cl_mre_map',str(project/'scripts/identify-runtime.py')).load_module()
    public=identity.public_map(project/'build/cl.map')
    from rules import ELF
    native=ELF(project/'build/native-trace/CL.EXE')
    assert PE(project/'build/cl.exe').read(public['_first_dependency_scan'],4)==struct.pack('<I',flag['initial_dword'])
    assert native.read(native.symbols['first_dependency_scan'],4)==struct.pack('<I',flag['initial_dword'])
    return {'exports':6,'calling_convention':'stdcall','native_successful_calls_authorized':0,
        'provider_sha256':proof['provider_sha256'],'return_qualification':'OneTimeInit EAX is ignored/unclaimed; no native dependency function returns.'}


def preserve_decorated_names(path,proof):
    # Short-import COFF NameType1 is IMPORT_NAME;2 is IMPORT_NAME_NOPREFIX.
    # dlltool's i386 default strips this provider's real leading underscore.
    # Change only that declaration flag, preserving all archive offsets/symbols.
    data=bytearray(path.read_bytes());assert data[:8]==b'!<arch>\n';cursor=8;seen=set()
    expected={row['export']:row['ordinal']for row in proof['exports']}
    while cursor<len(data):
        header=data[cursor:cursor+60];assert header[-2:]==b'`\n'
        size=int(header[48:58]);start=cursor+60;member=data[start:start+size]
        if member[:4]==b'\0\0\xff\xff':
            sig1,sig2,version,machine,timestamp,length,hint,kind=struct.unpack_from('<4H2I2H',member)
            assert (version,machine,kind)==(0,0x14c,8)
            names=bytes(member[20:]).split(b'\0');assert len(names)==3 and names[1]==b'MSPDB50.DLL' and names[2]==b''
            name=names[0].decode('ascii');assert name in expected and hint==expected[name] and name not in seen
            assert length==size-20;seen.add(name)
            struct.pack_into('<H',data,start+18,4)
        cursor=start+size+(size&1)
    assert cursor==len(data) and seen==set(expected)
    path.write_bytes(data)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-import-library',type=Path)
    parser.add_argument('--dlltool',default='llvm-dlltool')
    args=parser.parse_args();proof=declarations()
    if args.build_import_library:
        # LLVM selects its multicall mode from argv[0]; retain dlltool's name.
        tool=Path(shutil.which(args.dlltool)).absolute()
        command=[str(tool),'--no-leading-underscore','-m','i386','-d','scripts/mspdb50.def','-l',str(args.build_import_library)]
        subprocess.run(command,cwd=PROJECT,check=True)
        preserve_decorated_names(args.build_import_library,proof)
        Path(str(args.build_import_library)+'.json').write_text(json.dumps({'command':command,'tool_sha256':sha(tool),
            'definition_sha256':sha(PROJECT/'scripts/mspdb50.def'),'generator_sha256':sha(Path(__file__)),
            'name_type':'IMPORT_NAME: preserve verified decorated names verbatim','output_sha256':sha(args.build_import_library)},indent=2)+'\n')
    else:print(json.dumps(verify(PROJECT,ROOT),indent=2))
