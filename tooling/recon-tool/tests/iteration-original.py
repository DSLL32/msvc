#!/usr/bin/env python3
"""One real authored PE progress pair through iteration; no worker cases.

Explicit original-only test. Uses shared original-tool commands and the existing
qualified broker; native comparison and the graph auditor are real.
"""
import hashlib,json,os,re,shutil,struct,subprocess,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
REPO=Path(os.environ.get('RECON_TOOL_TEST_REPO',ROOT.parents[1])).resolve()
TOOL=Path(os.environ.get('RECON_TOOL_TEST_EXE',ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool')).resolve()
WORK=Path(tempfile.mkdtemp(prefix='iteration-original-',dir=Path.home()/'.ai-tmp/msvc2'))
start=time.monotonic()
def run(args,cwd=WORK):
 p=subprocess.run(list(map(str,args)),cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 if p.returncode:raise AssertionError((args,p.stdout))
 return p.stdout
def win(p):return 'S:\\'+str(p.resolve().relative_to(Path.home()/'.ai-tmp/msvc2')).replace('/','\\')
def write(p,text):
 p=WORK/p;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text);return p
try:
 source=write('original.c','''__declspec(dllimport) void __stdcall ExitProcess(unsigned);
int pending(void){return 9;} int owner(void){return pending();}
void __stdcall start(void){ExitProcess(owner());}\n''')
 run([TOOL,'original','tool','1.0-nt','CL.EXE','--repo',REPO,'--','/nologo','/c','/Od','/Fo'+win(WORK/'target.obj'),win(source)])
 run([TOOL,'original','tool','1.0-nt','LINK.EXE','--repo',REPO,'--','/nologo','/nodefaultlib','/entry:start@0','/subsystem:console','/out:'+win(WORK/'target.exe'),'/map:'+win(WORK/'target.map'),win(WORK/'target.obj'),'KERNEL32.LIB'])
 image=(WORK/'target.exe').read_bytes();pe=struct.unpack_from('<I',image,0x3c)[0];base=struct.unpack_from('<I',image,pe+24+28)[0];entry=struct.unpack_from('<I',image,pe+24+16)[0]
 symbols={m[1]:int(m[2],16)-base for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(WORK/'target.map').read_text(),re.M)}
 sections=[]
 for i in range(struct.unpack_from('<H',image,pe+6)[0]):
  off=pe+24+struct.unpack_from('<H',image,pe+20)[0]+i*40
  rva,size,raw=struct.unpack_from('<III',image,off+12);sections.append((rva,size,raw))
 def at(rva):
  for a,n,p in sections:
   if a<=rva<a+n:return image[p+rva-a:p+n]
  raise AssertionError(hex(rva))
 rows=[]
 for name,symbol,status,definition in [('owner','_owner',1,4),('pending','_pending',0,4),('startup','_start@0',0,3)]:
  a=symbols[symbol];b=min((v for v in symbols.values() if v>a),default=a+len(at(a)))
  # Last text public must end at actual virtual .text length, not next DATA public.
  for i in range(struct.unpack_from('<H',image,pe+6)[0]):
   off=pe+24+struct.unpack_from('<H',image,pe+20)[0]+i*40
   n,v=struct.unpack_from('<II',image,off+8)
   if v<=a<v+n:b=min(b,v+n)
  rows.append(f'{{.name="{name}",.entry_rva={a},.code=(const ReconExtent[]){{{{{a},{b}}}}},.code_count=1,.definition={definition},.implementation={status},.native_symbol='+('"'+name+'"' if definition==4 else '0')+f',.original_symbol="{symbol}"}}')
 for symbol,rva in symbols.items():
  if at(rva)[:2]==b'\xff\x25':
   name=symbol.lstrip('_').split('@')[0]
   rows.append(f'{{.name="import:{name}",.entry_rva={rva},.code=(const ReconExtent[]){{{{{rva},{rva+6}}}}},.code_count=1,.definition=RECON_IMPORT_NAME,.import_dll="kernel32.dll",.import_name="{name}"}}')
 write('binary.c','#include "recon-tool.h"\nstatic const ReconFunction f[]={'+','.join(rows)+'};\n'+f'const ReconBinary binary_descriptor={{.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconBinary),.name="iteration-original",.original_path="{WORK}/target.exe",.original_sha256="{hashlib.sha256(image).hexdigest()}",.original_entry_rva={entry},.functions=f,.function_count=RECON_COUNT(f)}};\n')
 write('src/app.c','int pending(void){__asm__ volatile("int3");return 0;} int owner(void){return pending();}\n')
 case=WORK/'cases/0000-progress';case.mkdir(parents=True)
 write('cases/0000-progress/case.c','#include "recon-tool.h"\nextern int owner(void); static int io(int n,char **v){return owner();}\n'+f'const ReconCase case_descriptor={{.abi_version=RECON_ABI_VERSION,.struct_size=sizeof(ReconCase),.name="iteration-original",.description="authored real progress pair",.execution=RECON_IO,.io=io,.terminal=RECON_FRONTIER,.frontier_rva={symbols["_owner"]}}};\n')
 write('input.json',json.dumps({'args':['authored.exe'],'partial':{'stdout':'text:','stderr':'text:','reject_unexpected_files':True}}))
 run([TOOL,'capture','pack','input.json','inputs.bin'])
 write('empty-runtime.json','[]\n')
 write('policy.json',json.dumps({'ret_checks':{'schema':1,'functions':{'owner':{'expected_cleanup':[0]}}}}))
 write('Makefile',f'''RECON_TOOL_DIR := {ROOT}
RECON_TOOL := {TOOL}
BINARY_SOURCES := src/app.c
BINARY_SUPPORT_SOURCES := {REPO}/lib/recon/src/trace.c
NATIVE_CPPFLAGS := -DRECON_NATIVE_TEST
ORIGINAL_TOOLCHAIN := 1.0-nt
ORIGINAL_SOURCES := original.c
ORIGINAL_CC := $(RECON_TOOL) original tool 1.0-nt CL.EXE --repo {REPO} --
ORIGINAL_LINK := $(RECON_TOOL) original tool 1.0-nt LINK.EXE --repo {REPO} --
ORIGINAL_LDFLAGS := /nologo /nodefaultlib /entry:start@0 /subsystem:console
ORIGINAL_LDLIBS = $(ORIGINAL_TOOLCHAIN_ROOT)/lib/KERNEL32.LIB
ORIGINAL_OUTPUT := build/rebuilt.exe
ORIGINAL_MAP := build/rebuilt.map
RECON_POLICY := policy.json
RECON_IMAGE_BASE := {base}
RECON_AUDIT_PROFILE := cl
RECON_AUDIT_REGISTRY_RET_CHECKS := 1
RECON_ITERATION_ENABLED := 1
RECON_ITERATION_INPUT := inputs.bin
RECON_ITERATION_METADATA_PREREQUISITES :=
RECON_AUDIT_FLAGS := --discovery cfg --check-ownership --check-extents --check-references --rebuilt-runtime empty-runtime.json
include $(RECON_TOOL_DIR)/recon-tool.mk
GRAPH_AUDIT_BIN := {REPO}/tooling/graph-audit/target/release/graph-audit
''')
 print(run([TOOL,'original','iterate','cases/0000-progress','--repo',REPO,'--all-restored','--state',WORK.with_name(WORK.name+'-journal')]))
 result=json.loads(next((WORK.with_name(WORK.name+'-journal')).glob('*/result.json')).read_text())
 assert result['checked'] and result['native_pass'] is True and result['original_launches']['root']==1,result
 assert result['exercised_owners']==['owner']
 print(f'iteration real PE graph/capture/native PASS ({time.monotonic()-start:.3f} s incl authored period build); one target capture')
 print('retained:',WORK)
except BaseException:
 print('failure retained:',WORK);raise
