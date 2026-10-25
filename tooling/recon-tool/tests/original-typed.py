#!/usr/bin/env python3
"""One authored typed-DLL direct probe; explicit originals only."""
from pathlib import Path
import hashlib,json,os,re,struct,subprocess,tempfile,time
TOOL=Path(__file__).resolve().parents[1]
ROOT=TOOL.parents[1]
EXE=Path(os.environ.get('RECON_TOOL', TOOL/'target/i686-unknown-linux-gnu/release/recon-tool')).resolve()
LAUNCH=EXE
WORK=Path(tempfile.mkdtemp(prefix='recon-tool-typed-', dir=Path.home()/'.ai-tmp/msvc2'))
START=time.monotonic()
def win(path):
 path=path.resolve()
 for base,drive in [(ROOT,'R'),(Path.home()/'.ai-tmp/msvc2','S')]:
  if path.is_relative_to(base):return drive+':\\'+str(path.relative_to(base)).replace('/','\\')
 raise ValueError(path)
def call(args,**kw):
 p=subprocess.run([str(a) for a in args],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,**kw)
 if p.returncode:print(p.stdout.decode(errors='replace'));p.check_returncode()
 return p.stdout
def original(tool,args):return call([LAUNCH,'original','tool','1.0-nt',tool,'--',*args],cwd=WORK)
source=WORK/'target.c';target=WORK/'target.exe';targetmap=WORK/'target.map'
source.write_text(r'''
#include <windows.h>
typedef struct Owner { unsigned first,second,tag; } Owner;
Owner *current_owner;
int __fastcall transform(Owner *owner,int bias,unsigned delta) {
    owner->first += bias+delta;return owner->first+owner->second;
}
void __cdecl marker(Owner *owner) { owner->tag=0;ExitProcess(8); }
void __stdcall start(void) {
    Owner owner;DWORD n;owner.first=2;owner.second=5;owner.tag=0x12345678;
    current_owner=&owner;WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),"ready\n",6,&n,0);
    marker(&owner);ExitProcess(9);
}
''')
original('CL.EXE',['/nologo','/c','/Od','/G3','/Fo'+win(WORK/'target.obj'),win(source)])
original('LINK.EXE',['/nologo','/nodefaultlib','/entry:start@0','/subsystem:console','/out:'+win(target),'/map:'+win(targetmap),win(WORK/'target.obj'),'KERNEL32.LIB'])
data=target.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0];base=struct.unpack_from('<I',data,pe+24+28)[0];entry=struct.unpack_from('<I',data,pe+24+16)[0]
symbols={m[1]:int(m[2],16)-base for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',targetmap.read_text(),re.M)}
print('original symbols',symbols)
marker=symbols['_marker'];transform=symbols['@transform@12'];owner=symbols['_current_owner']
helper=WORK/'probe.c';helperdll=WORK/'probe.dll'
helper.write_text('''#include "recon-original-direct.h"
typedef struct Owner { unsigned first,second,tag; } Owner;
typedef int (__fastcall *Transform)(Owner *,int,unsigned);
__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context) {
    Owner *owner=*(Owner **)RECON_ORIGINAL_ADDRESS(context,%d);
    DWORD *stack=(DWORD *)context->original_esp;
    unsigned result[2];
    if(context->marker_rva!=%d || (Owner *)stack[1]!=owner || owner->first!=2 || owner->second!=5 || owner->tag!=0x12345678) recon_original_finish(context,0);
    result[0]=((Transform)RECON_ORIGINAL_ADDRESS(context,%d))(owner,4,6);
    result[1]=owner->first;
    recon_original_finish(context,result[0]==17 && result[1]==12 && owner->tag==0x12345678 && recon_original_emit(context,"probe.bin",result,sizeof(result)));
}
'''%(owner,marker,transform))
call([EXE,'original','build-direct',helper,'--repo',ROOT,'--output',helperdll])
call([EXE,'original','build-helper','--repo',ROOT])
NATIVE=WORK/'native';CASE=NATIVE/'cases/0000-typed';CASE.mkdir(parents=True,exist_ok=True);(NATIVE/'.build').mkdir(exist_ok=True);(CASE/'.build').mkdir(exist_ok=True)
(NATIVE/'app.c').write_text(r'''
#include <unistd.h>
typedef struct Owner { unsigned first,second,tag; } Owner;
Owner *current_owner;
int __attribute__((fastcall)) fixture_transform(Owner *owner,int bias,unsigned delta) {
#ifdef PENDING
    __asm__ volatile("int3");
#endif
    owner->first+=bias+delta;return owner->first+owner->second;
}
void fixture_marker(Owner *owner) { owner->tag=0;_exit(8); }
void fixture_start(void) { Owner owner={2,5,0x12345678};current_owner=&owner;write(1,"ready\n",6);fixture_marker(&owner);_exit(9); }
''')
(CASE/'case.c').write_text('''#include "recon-tool.h"
#include "recon-native.h"
#include <stdio.h>
typedef struct Owner { unsigned first,second,tag; } Owner;
extern Owner *current_owner;
extern int __attribute__((fastcall)) fixture_transform(Owner *,int,unsigned);
extern void fixture_marker(Owner *),fixture_start(void);
static void callback(Owner *owner) {
    unsigned expected[2],result[2];FILE *f;int pass;
    if(current_owner!=owner || owner->tag!=0x12345678)recon_native_entry_finish(0);
    result[0]=fixture_transform(owner,4,6);result[1]=owner->first;
    f=fopen("probe.bin","rb");pass=f && fread(expected,1,sizeof(expected),f)==sizeof(expected);if(f)fclose(f);
    if(!pass)recon_native_entry_finish(0);
    recon_native_entry_finish(result[0]==expected[0] && result[1]==expected[1] && owner->tag==0x12345678);
}
static void invoke(void *unused) { (void)unused;fixture_start(); }
static int direct(void) { return recon_native_at_entry(fixture_marker,callback,invoke,0)==1; }
static const uint32_t order[]={%d,%d,%d};
const ReconCase case_descriptor={.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),.name="typed-original",.description="genuine stack owner and typed fastcall",.first_entry_order=order,.first_entry_count=3,.execution=RECON_DIRECT,.terminal=RECON_EXIT,.direct=direct};
'''%(entry,marker,transform))
records=[]
for orig,native in [('_start@0','fixture_start'),('_marker','fixture_marker'),('@transform@12','fixture_transform')]:
 rva=symbols[orig];end=min(x for x in symbols.values() if x>rva)
 records.append('{.name="'+orig+'",.entry_rva='+str(rva)+',.code=(const ReconExtent[]){{'+str(rva)+','+str(end)+'}},.code_count=1,.definition=RECON_APP,.implementation=RECON_IMPLEMENTED,.native_symbol="'+native+'"}')
for name,provider in [('_ExitProcess@4','ExitProcess'),('_WriteFile@20','WriteFile'),('_GetStdHandle@4','GetStdHandle')]:
 rva=symbols[name]
 records.append('{.name="'+name+'",.entry_rva='+str(rva)+',.code=(const ReconExtent[]){{'+str(rva)+','+str(rva+6)+'}},.code_count=1,.definition=RECON_IMPORT_NAME,.import_dll="KERNEL32.dll",.import_name="'+provider+'"}')
(NATIVE/'binary.c').write_text('#include "recon-tool.h"\nstatic const ReconFunction f[]={'+','.join(records)+'};\nconst ReconBinary binary_descriptor={.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconBinary),.name="typed",.original_path="'+str(target)+'",.original_sha256="'+hashlib.sha256(data).hexdigest()+'",.original_entry_rva='+str(entry)+',.functions=f,.function_count='+str(len(records))+'};\n')
flags=['gcc','-m32','-g','-O0','-fPIC','-DRECON_NATIVE_TEST','-I'+str(ROOT/'tooling/recon-tool/include'),'-I'+str(ROOT/'lib/recon/include')]
objects=[]
for path,instrument in [(NATIVE/'app.c',True),(NATIVE/'binary.c',False),(ROOT/'lib/recon/src/trace.c',False),(ROOT/'tooling/recon-tool/native/entry-hook.c',False)]:
 obj=NATIVE/'.build'/(path.stem+'.o');call([*flags,*(['-finstrument-functions'] if instrument else []),'-c',path,'-o',obj]);objects.append(obj)
call(['gcc','-m32','-shared','-Wl,-Bsymbolic-functions','-Wl,-soname,libbinary.so','-o',NATIVE/'.build/libbinary.so',*objects,'-ldl'])
call([*flags,'-shared',CASE/'case.c',NATIVE/'.build/libbinary.so','-Wl,-rpath,$ORIGIN/../../../.build','-o',CASE/'.build/case.so'])
inputjson=WORK/'input.json';inputjson.write_text(json.dumps({'args':['typed'],'stdout':'text:ready\n','stderr':'text:','exit_code':0}))
inputbin=WORK/'input.bin';call([EXE,'capture','pack',inputjson,inputbin])
capture=WORK/'capture.bin';report=WORK/'report.json'
t=time.monotonic()
output=call([EXE,'original','capture',CASE,'--repo',ROOT,'--capture',inputbin,'--helper',TOOL/'.build/wine-broker.exe','--direct-helper',helperdll,'--marker-rva',marker,'--progress','--output',capture,'--report',report,'--replace'])
print(output.decode());print('typed capture',time.monotonic()-t)
result=json.loads(report.read_text());assert result['terminal']=='direct:pass';assert result['first_entry_order']==[entry,marker,transform]
print(call([EXE,'run',CASE,'--capture',capture]).decode())
print('typed original/native PASS, total',time.monotonic()-START)

registry=NATIVE/'binary.c';registry.write_text(registry.read_text().replace('.implementation=RECON_IMPLEMENTED,.native_symbol="fixture_transform"','.implementation=RECON_UNIMPLEMENTED,.native_symbol="fixture_transform"'))
case_source=CASE/'case.c';case_source.write_text(case_source.read_text().replace('.terminal=RECON_EXIT','.terminal=RECON_FRONTIER,.frontier_rva='+str(transform)))
call([*flags,'-DPENDING','-finstrument-functions','-c',NATIVE/'app.c','-o',NATIVE/'.build/app.o'])
call([*flags,'-c',registry,'-o',NATIVE/'.build/binary.o'])
call(['gcc','-m32','-shared','-Wl,-Bsymbolic-functions','-Wl,-soname,libbinary.so','-o',NATIVE/'.build/libbinary.so',*objects,'-ldl'])
call([*flags,'-shared',case_source,NATIVE/'.build/libbinary.so','-Wl,-rpath,$ORIGIN/../../../.build','-o',CASE/'.build/case.so'])
frontier=WORK/'frontier.bin';frontier_report=WORK/'frontier-report.json'
print(call([EXE,'original','capture',CASE,'--repo',ROOT,'--capture',inputbin,'--helper',TOOL/'.build/wine-broker.exe','--direct-helper',helperdll,'--marker-rva',marker,'--progress','--output',frontier,'--report',frontier_report,'--replace']).decode())
r=json.loads(frontier_report.read_text());assert r['terminal']=='frontier:'+format(transform,'x');assert r['first_entry_order']==[entry,marker,transform]
print(call([EXE,'run',CASE,'--capture',frontier]).decode())
print('typed masked frontier PASS')

# Returning, false assertions and malformed receipts must never save a capture.
for label, body in [
    ('assert-false', 'recon_original_finish(context, 0);'),
    ('returned', '(void)context;'),
    ('bad-receipt', 'context->result_capacity=0; recon_original_finish(context, 1);'),
]:
    negative_source=WORK/(label+'.c')
    negative_source.write_text('#include "recon-original-direct.h"\n'
        '__declspec(dllexport) void __stdcall case_original_at_marker(ReconOriginalProbe *context) {'+body+'}\n')
    negative_helper=WORK/(label+'.dll')
    call([EXE,'original','build-direct',negative_source,'--repo',ROOT,'--output',negative_helper])
    destination=WORK/(label+'.bin')
    result=subprocess.run([str(a) for a in [EXE,'original','capture',CASE,'--repo',ROOT,
        '--capture',inputbin,'--helper',TOOL/'.build/wine-broker.exe',
        '--direct-helper',negative_helper,'--marker-rva',marker,'--progress',
        '--output',destination]],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (WORK/(label+'.log')).write_bytes(result.stdout)
    assert result.returncode and not destination.exists(), label
# A helper that does not install VEH must not claim the implemented marker as
# a pending App frontier. This is an intentional test-only support variant.
no_handler=WORK/'no-handler-support.c'
no_handler.write_text((TOOL/'wine/direct-support.c').read_text().replace(
    '!add_handler(1, probe_exception)', '!((LPVOID)1)'))
original('CL.EXE',['/nologo','/c','/Od','/G3','/I'+win(TOOL/'include'),
    '/Fo'+win(WORK/'no-handler.obj'),win(no_handler)])
original('LINK.EXE',['/nologo','/dll','/nodefaultlib','/entry:DllMain@12',
    '/def:'+win(WORK/'probe.def'),'/out:'+win(WORK/'no-handler.dll'),
    win(WORK/'probe-case.obj'),win(WORK/'no-handler.obj'),'LIBC.LIB','KERNEL32.LIB'])
result=subprocess.run([str(a) for a in [EXE,'original','capture',CASE,'--repo',ROOT,
    '--capture',inputbin,'--helper',TOOL/'.build/wine-broker.exe',
    '--direct-helper',WORK/'no-handler.dll','--marker-rva',marker,'--progress',
    '--output',WORK/'no-handler.bin']],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
assert result.returncode and b'unexpected typed original INT3' in result.stdout
assert not (WORK/'no-handler.bin').exists()
print('typed assertion/return/receipt/missing-handler failures PASS; scratch:', WORK)
