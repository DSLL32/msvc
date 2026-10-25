#!/usr/bin/env python3
"""Explicit Wine integration tests for the shared transport, never native fixtures."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

TOOL_ROOT = Path(__file__).resolve().parents[1]
REPO = TOOL_ROOT.parents[1]
TOOL = TOOL_ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool'
SHARED_SCRATCH = Path.home()/'.ai-tmp/msvc2'
SHARED_SCRATCH.mkdir(exist_ok=True)
WORK = Path(tempfile.mkdtemp(prefix='recon-tool-original-',dir=SHARED_SCRATCH))
ENV=dict(os.environ,WINEPREFIX=str(Path.home()/'.ai-tmp/vc1.0-capture/wine'),WINEARCH='win32',WINEDEBUG='-all',
         INCLUDE=r'R:\toolchains\1.0-nt\include',LIB=r'R:\toolchains\1.0-nt\lib',WINEPATH=r'R:\toolchains\1.0-nt\bin')

def windows(path):
    path=Path(path).resolve()
    for base,drive in [(REPO,'R'),(SHARED_SCRATCH,'S')]:
        if path.is_relative_to(base):return drive+':\\'+str(path.relative_to(base)).replace('/','\\')
    raise ValueError(path)

def command(args,**kwargs):
    result=subprocess.run([str(a) for a in args],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,**kwargs)
    if result.returncode:
        print(result.stdout.decode(errors='replace'))
        result.check_returncode()
    return result.stdout

def win_tool(name,args):
    return command(['wine','R:\\toolchains\\1.0-nt\\bin\\'+name,*args],cwd=WORK,env=ENV)

def build_original(source,name,dll=False,definition=None):
    obj=WORK/(name+'.obj');exe=WORK/(name+('.dll' if dll else '.exe'));mapfile=WORK/(name+'.map')
    win_tool('CL.EXE',['/nologo','/c','/Od','/G3','/Fo'+windows(obj),windows(source)])
    args=['/nologo','/nodefaultlib','/entry:'+('DllMain@12' if dll else 'start@0'),'/subsystem:console',
          '/out:'+windows(exe),'/map:'+windows(mapfile),windows(obj),'KERNEL32.LIB']
    if dll:args+=['/dll','/def:'+windows(definition)]
    win_tool('LINK.EXE',args)
    data=exe.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0];base=struct.unpack_from('<I',data,pe+24+28)[0]
    entry=struct.unpack_from('<I',data,pe+24+16)[0]
    symbols={m[1]:int(m[2],16)-base for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',mapfile.read_text(),re.M)}
    count,optional=struct.unpack_from('<H',data,pe+6)[0],struct.unpack_from('<H',data,pe+20)[0]
    sections=[]
    for i in range(count):
        off=pe+24+optional+i*40
        rva,size,raw=struct.unpack_from('<III',data,off+12)
        sections.append((rva,size,raw))
    def at(rva):
        for section,size,raw in sections:
            if section<=rva<section+size:return data[raw+rva-section:raw+size]
        raise ValueError(hex(rva))
    return exe,data,entry,symbols,at

NATIVE_SOURCE=r'''
#include <stdio.h>
#include <stdlib.h>
int probe_sum(int *values,int count) {
#ifdef PROGRESS
    __asm__ volatile("int3");
#endif
    int i,total=0;for(i=0;i<count;++i)total+=values[i];values[0]=total;return total;
}
void fixture_start(void) {
    int values[2]={2,5};unsigned char bytes[2]={0,255};FILE *f;
    if(probe_sum(values,2)!=7||values[0]!=7)exit(9);
    f=fopen("out.bin","wb");fwrite(bytes,1,2,f);fclose(f);fputs("sum=7\n",stdout);exit(0);
}
'''
DLL_NATIVE=r'''
#include <stdio.h>
static int attached;
int __attribute__((stdcall)) fixture_DllMain(void *module,unsigned reason,void *reserved) {
    if(reason==1)++attached;else if(reason==0)--attached;return 1;
}
int __attribute__((stdcall)) fixture_ProbeSum(int *values,int count) {
    int i,total=0;if(attached!=1)return -1;for(i=0;i<count;++i)total+=values[i];values[0]=total;return total;
}
int dll_adapter(int argc,char **argv) {
    int values[2]={2,5};unsigned char bytes[2]={0,255};FILE *f;
    if(!fixture_DllMain((void*)1,1,0)||fixture_ProbeSum(values,2)!=7)return 12;
    f=fopen("out.bin","wb");fwrite(bytes,1,2,f);fclose(f);fputs("sum=7\n",stdout);
    return fixture_DllMain((void*)1,0,0)?0:13;
}
'''

def native_library(name,exe,data,entry,symbols,at,dll=False,progress=False):
    root=WORK/name;build=root/'.build';build.mkdir(parents=True)
    app=[('_DllMain@12','fixture_DllMain'),('_ProbeSum@8','fixture_ProbeSum')] if dll else [('_start@0','fixture_start'),('_probe_sum','probe_sum')]
    sorted_symbols=sorted(symbols.values())
    records=[]
    for original,native in app:
        rva=symbols[original];end=next((v for v in sorted_symbols if v>rva),rva+len(at(rva)))
        status='RECON_UNIMPLEMENTED' if progress and original=='_probe_sum' else 'RECON_IMPLEMENTED'
        records.append(f'{{.name="{original}",.entry_rva={rva},.code=(const ReconExtent[]){{{{{rva},{end}}}}},.code_count=1,.definition=RECON_APP,.implementation={status},.native_symbol="{native}"}}')
    for symbol,rva in symbols.items():
        if at(rva)[:2]==b'\xff\x25':
            records.append(f'{{.name="import:{symbol}",.entry_rva={rva},.code=(const ReconExtent[]){{{{{rva},{rva+6}}}}},.code_count=1,.definition=RECON_IMPORT_NAME,.import_dll="kernel32.dll",.import_name="{symbol.lstrip(chr(95)).split(chr(64))[0]}"}}')
    registry='#include "recon-tool.h"\nstatic const ReconFunction functions[]={\n'+',\n'.join(records)+'\n};\n'
    registry+=f'const ReconBinary binary_descriptor={{.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconBinary),.name="original-test",.original_path="{exe}",.original_sha256="{hashlib.sha256(data).hexdigest()}",.original_entry_rva={entry},.functions=functions,.function_count=RECON_COUNT(functions)}};\n'
    (root/'binary.c').write_text(registry);(root/'app.c').write_text(DLL_NATIVE if dll else NATIVE_SOURCE)
    includes=['-I'+str(TOOL_ROOT/'include')]
    common=['gcc','-m32','-fPIC','-O0','-g',*includes]
    command([*common,'-finstrument-functions',*(['-DPROGRESS'] if progress else []),'-c',root/'app.c','-o',build/'app.o'])
    command([*common,'-c',root/'binary.c','-o',build/'binary.o'])
    command([*common,'-DRECON_NATIVE_TEST','-c',REPO/'lib/recon/src/trace.c','-o',build/'trace.o'])
    command(['gcc','-m32','-shared','-Wl,-Bsymbolic-functions','-Wl,-soname,libbinary.so','-o',build/'libbinary.so',build/'app.o',build/'binary.o',build/'trace.o','-ldl'])
    return root,app

def build_case(root,number,symbols,dll=False,direct=False,progress=False):
    case=root/'cases'/number;build=case/'.build';build.mkdir(parents=True)
    entry=symbols['_DllMain@12' if dll else '_start@0'];probe=symbols['_ProbeSum@8' if dll else '_probe_sum']
    order=[probe] if direct else [entry,probe]
    code='#include "recon-tool.h"\n#include "recon-original.h"\n#include <stdio.h>\n'
    code+='extern int probe_sum(int*,int);\nextern void fixture_start(void);\n' if not dll else 'extern int dll_adapter(int,char**);\n'
    if not dll:code+='static int adapter(int argc,char **argv){fixture_start();return 0;}\n'
    if direct:
        code+='''static int direct_native(void){unsigned expected[2],values[2]={2,5};FILE *f=fopen("expected.bin","rb");if(!f)return 0;if(fread(expected,4,2,f)!=2)return 0;fclose(f);return probe_sum((int*)values,2)==7&&values[0]==expected[0]&&values[1]==expected[1];}\n'''
        code+=f'''int case_original_direct(ReconOriginal *ctx){{
            ReconOriginalRegisters initialized;ReconOriginalCallResult result;
            unsigned values[2]={{2,5}},arguments[2],observed[2];unsigned address;
            if(!ctx->until(ctx,{entry},&initialized))return 0;
            address=ctx->allocate(ctx,8);if(!address)return 0;
            if(!ctx->write(ctx,address,values,8))return 0;
            arguments[0]=address;arguments[1]=2;
            if(!ctx->call(ctx,{probe},0,0,arguments,2,&result))return 0;
            if(result.returned.eax!=7||result.returned.esp!=result.entry.esp+4||result.returned.ebx!=result.entry.ebx)return 0;
            if(!ctx->read(ctx,address,observed,8)||observed[0]!=7||observed[1]!=5)return 0;
            if(!ctx->release(ctx,address)||!ctx->emit(ctx,"expected.bin",observed,8))return 0;
            return 1;
        }}\n'''
    code+='static const uint32_t order[]={'+','.join(map(str,order))+'};\n'
    code+='const ReconCase case_descriptor={.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),.name="'+number+'",.description="authored shared transport test",.first_entry_order=order,.first_entry_count=RECON_COUNT(order),'
    code+=f'.execution={"RECON_DIRECT" if direct else "RECON_IO"},.terminal={"RECON_FRONTIER" if progress else "RECON_EXIT"},.frontier_rva={probe if progress else 0},'
    code+='.direct=direct_native' if direct else '.io='+('dll_adapter' if dll else 'adapter')
    code+='};\n';(case/'case.c').write_text(code)
    command(['gcc','-m32','-shared','-fPIC','-O0','-g','-I'+str(TOOL_ROOT/'include'),case/'case.c','-L'+str(root/'.build'),'-Wl,--no-as-needed','-lbinary','-Wl,-rpath,$ORIGIN/../../../.build','-o',build/'case.so'])
    return case,order

try:
    command([TOOL,'original','build-helper','--repo',REPO])
    exe,data,entry,symbols,at=build_original(TOOL_ROOT/'tests/original/fixture.c','fixture')
    root,_=native_library('native-exe',exe,data,entry,symbols,at)
    io_case,order=build_case(root,'0000-io',symbols)
    direct_case,direct_order=build_case(root,'0001-direct',symbols,direct=True)
    definition=WORK/'fixture.def';definition.write_text('LIBRARY fixture\nEXPORTS\n ProbeSum=_ProbeSum@8\n')
    dll,dll_data,dll_entry,dll_symbols,dll_at=build_original(TOOL_ROOT/'tests/original/dll.c','fixture-dll',True,definition)
    host,*_=build_original(TOOL_ROOT/'tests/original/host.c','host')
    dll_root,_=native_library('native-dll',dll,dll_data,dll_entry,dll_symbols,dll_at,dll=True)
    dll_case,dll_order=build_case(dll_root,'0000-dll',dll_symbols,dll=True)
    progress_root,_=native_library('native-progress',exe,data,entry,symbols,at,progress=True)
    progress_case,progress_order=build_case(progress_root,'0000-progress',symbols,progress=True)
    for case,expected,extra in [(io_case,order,[]),(direct_case,direct_order,[]),(dll_case,dll_order,['--host',host]),(progress_case,progress_order,['--progress'])]:
        source=WORK/(case.parent.parent.name+'-'+case.name+'.json');packed=source.with_suffix('.bin');report=source.with_suffix('.report.json')
        source.write_text(json.dumps(dict(args=['fixture'],timeout_ms=10000,reject_unexpected_files=True)))
        command([TOOL,'capture','pack',source,packed])
        oracle=case/'capture.bin'
        print(command([TOOL,'original','capture',case,'--repo',REPO,'--capture',packed,'--output',oracle,'--report',report,*extra]).decode())
        record=json.loads(report.read_text());assert record['first_entry_order']==expected,(record,expected)
        assert not Path(record['workspace']).exists()
        print(command([TOOL,'run',case]).decode())
    print('shared original EXE IO/direct, DLL host, masked-progress checks passed')
except BaseException:
    print('Failure scratch retained:',WORK)
    raise
else:
    shutil.rmtree(WORK)
