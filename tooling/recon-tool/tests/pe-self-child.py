#!/usr/bin/env python3
"""Explicit original-only authored PE SELF controls; never a fixture dependency."""
import hashlib,json,os,re,struct,subprocess,time,tempfile
from pathlib import Path
PACKAGE=Path(__file__).resolve().parents[1]
REPO=Path(os.environ.get('RECON_TOOL_TEST_REPO', PACKAGE.parents[1])).resolve()
TOOL=Path(os.environ.get('RECON_TOOL_TEST_EXE', PACKAGE/'target/i686-unknown-linux-gnu/release/recon-tool')).resolve()
HELPER=Path(os.environ.get('RECON_PE_SELF_HELPER', PACKAGE/'.build/wine-broker.exe')).resolve()
scratch=Path.home()/'.ai-tmp/msvc2'
scratch.mkdir(parents=True,exist_ok=True)
ROOT=Path(tempfile.mkdtemp(prefix='pe-self-control-',dir=scratch))
WORK=ROOT/'control'
WORK.mkdir(exist_ok=True)
def run(name,cmd,cwd=WORK,ok=True):
    start=time.monotonic();p=subprocess.run([str(v) for v in cmd],cwd=cwd,capture_output=True)
    (WORK/(name+'.log')).write_bytes(p.stdout+p.stderr)
    (WORK/(name+'-command.json')).write_text(json.dumps({'command':[str(v) for v in cmd],'status':p.returncode,'seconds':time.monotonic()-start},indent=2)+'\n')
    if ok and p.returncode:raise RuntimeError(name+': '+(p.stdout+p.stderr).decode(errors='replace'))
    return p

def windows(path):
    for root,drive in [(REPO,'R'),(Path.home()/'.ai-tmp/msvc2','S')]:
        if path.is_relative_to(root):return drive+':\\'+str(path.relative_to(root)).replace('/','\\')
    raise ValueError(path)
run('compile',[TOOL,'original','tool','--repo',REPO,'1.0-nt','CL.EXE','--','/nologo','/c','/Od','/G3','/Fo'+windows(ROOT/'control.obj'),windows(PACKAGE/'tests/pe-self-child.c')])
run('link',[TOOL,'original','tool','--repo',REPO,'1.0-nt','LINK.EXE','--','/nologo','/nodefaultlib','/entry:start@0','/subsystem:console','/base:0x400000','/out:'+windows(ROOT/'control.exe'),'/map:'+windows(ROOT/'control.map'),windows(ROOT/'control.obj'),'KERNEL32.LIB'])
exe=ROOT/'control.exe';data=exe.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0];base=struct.unpack_from('<I',data,pe+52)[0];entry=struct.unpack_from('<I',data,pe+40)[0]
symbols={m[1]:int(m[2],16)-base for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', (ROOT/'control.map').read_text(),re.M)}
table=pe+24+struct.unpack_from('<H',data,pe+20)[0]
text_end=None;text_start=None
for i in range(struct.unpack_from('<H',data,pe+6)[0]):
 off=table+40*i
 if data[off:off+8].rstrip(b'\0')==b'.text':
  size,rva=struct.unpack_from('<II',data,off+8);text_start=rva;text_end=rva+size
assert text_end and text_start
apps=['control_common','control_pending','control_child','control_parent']
addresses=sorted(v for v in symbols.values() if text_start<=v<text_end)
records=[]
for name in apps:
 start=symbols['_'+name];end=next(v for v in addresses if v>start)
 records.append(f'{{.name="{name}",.entry_rva={start},.code=(ReconExtent[]){{{{{start},{end}}}}},.code_count=1,.definition=RECON_APP,.implementation={0 if name=="control_pending" else 1},.native_symbol="{name}"}}')
records.append(f'{{.name="authored-support",.entry_rva={entry},.code=(ReconExtent[]){{{{{text_start},{symbols["_control_common"]}}},{{{entry},{text_end}}}}},.code_count=2,.definition=RECON_CRT}}')
binary='#include "recon-tool.h"\nstatic const ReconFunction functions[]={'+',\n'.join(records)+'};\n'
binary+=f'RECON_EXPORT const ReconBinary binary_descriptor={{RECON_ABI_VERSION,sizeof(ReconBinary),"pe-self-control","{exe}","{hashlib.sha256(data).hexdigest()}",{entry},functions,RECON_COUNT(functions),0,0}};\n'
(WORK/'binary.c').write_text(binary)
(WORK/'app.c').write_text(r'''
#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <recon.h>
static int state=1;
int control_common(int value) { return value+1; }
int control_pending(void) { __asm__ volatile("int3"); return 8; }
void control_child(const char *mode) {
 if(state!=1 || control_common(4)!=5)recon_exit(31);
 write(1,"child\n",6);
 if(!strcmp(mode,"frontier"))control_pending();
 recon_exit(7);
}
void control_parent(const char *mode) {
 const char *args[]={recon_pgmptr,"child",mode,NULL};
 int result;
 if(state!=1 || control_common(2)!=3)recon_exit(32);
 state=9;write(1,"parent\n",7);
 result=recon_spawnve(0,recon_pgmptr,args,NULL);
 if(result!=7 || state!=9)recon_exit(35);
 write(1,"done\n",5);recon_exit(0);
}
''')
(WORK/'case.c').write_text(r'''
#include "recon-tool.h"
#include "recon-process.h"
#include <stdlib.h>
#include <string.h>
extern void control_child(const char*),control_parent(const char*);
static int io(int argc,char **argv) {
 if(argc==3 && !strcmp(argv[1],"child"))control_child(argv[2]);
 if(argc!=2)return 90;
 control_parent(argv[1]);return 91;
}
static int policy(const ReconProcessRequest *r,ReconProcessResponse *out) {
 unsigned i;const char *path=getenv("RECON_PROGRAM_PATH");
 if(!path || r->index || r->mode || r->protocol!=RECON_PROCESS_RSP1 || r->argc!=3 ||
 strcmp(r->file,path) || strcmp(r->argv[0],path) || strcmp(r->argv[1],"child") ||
 strcmp(r->argv[2],"success") || strcmp(r->cwd,r->workspace_cwd) ||
 r->environment_count!=r->prepared_environment_count)return 0;
 for(i=0;i<r->environment_count;++i)if(strcmp(r->environment[i],r->prepared_environment[i]))return 0;
 out->action=RECON_PROCESS_SELF;return 1;
}
RECON_EXPORT const ReconProcessPolicy case_process_policy={RECON_PROCESS_POLICY_VERSION,sizeof(ReconProcessPolicy),1,policy};
RECON_EXPORT const ReconCase case_descriptor={RECON_ABI_VERSION,sizeof(ReconCase),"pe-self-control","authored same-image child",0,0,0,0,RECON_IO,RECON_EXIT,0,0,io,0,0,0};
''')
common=['gcc','-m32','-fPIC','-O0','-g','-DRECON_NATIVE_TEST','-I'+str(PACKAGE/'include'),'-I'+str(REPO/'lib/recon/include'),'-I'+str(REPO/'lib/recon/native')]
run('app',[*common,'-finstrument-functions','-c',WORK/'app.c','-o',WORK/'app.o'])
run('binary',[*common,'-c',WORK/'binary.c','-o',WORK/'binary.o'])
objects=[WORK/'app.o',WORK/'binary.o']
for source in ['crt','win32','process','compat','entry','decimal','trace']:
 obj=WORK/(source+'.o');run('support-'+source,[*common,'-c',REPO/('lib/recon/src/'+source+'.c'),'-o',obj]);objects.append(obj)
run('native-link',['gcc','-m32','-shared','-Wl,-Bsymbolic-functions','-Wl,-soname,libbinary.so','-o',WORK/'libbinary.so',*objects,'-ldl'])
run('case',['gcc',*common[1:],'-shared','-Wl,-z,defs',WORK/'case.c',WORK/'libbinary.so','-Wl,-rpath,'+str(WORK),'-o',WORK/'case.so'])
inputs=dict(args=[windows(exe),'success'],env={'RECON_SELF_SPAWN':'1','RECON_PROGRAM_PATH':'T:\\self.exe'},reject_unexpected_files=True,timeout_ms=5000)
(WORK/'inputs.json').write_text(json.dumps(inputs))
run('pack',[TOOL,'capture','pack',WORK/'inputs.json',WORK/'inputs.bin'])
if os.environ.get('RECON_PE_SELF_PREPARE_ONLY')!='1':
 for mode in ['success','frontier','wrong','default']:
  f=dict(inputs);f['args']=[windows(exe),'success' if mode=='default' else mode]
  if mode=='wrong':f['input_files']=[dict(path='wrong.exe',data='hex:'+data.hex())]
  (WORK/(mode+'-input.json')).write_text(json.dumps(f));run(mode+'-pack',[TOOL,'capture','pack',WORK/(mode+'-input.json'),WORK/(mode+'-input.bin')])
  cmd=[TOOL,'original','capture',WORK/'case.so','--repo',REPO,'--helper',HELPER,'--capture',WORK/(mode+'-input.bin'),'--output',WORK/(mode+'.bin'),'--report',WORK/(mode+'.json'),'--progress']
  if mode!='default':cmd+=['--pe-self-child']
  result=run(mode+'-original',cmd,ok=mode!='wrong')
  report=json.loads((WORK/(mode+'.json')).read_text())
  if mode=='wrong':
   assert result.returncode and 'pe-self rejected process' in report['error'] and not (WORK/'wrong.bin').exists(),report
  elif mode=='default':
   assert report['terminal']=='exit:0' and report.get('pe_self_child') is None
   assert report['first_entry_order']==[symbols['_control_parent'],symbols['_control_common']],report
  else:
   family=report['pe_self_child'];assert len(family['processes'])==2 and all(p['signaled'] for p in family['processes'])
   if mode=='frontier':
    assert report['terminal']==f'frontier:{symbols["_control_pending"]:x}'
    assert family['frontier_pid']==family['processes'][1]['pid'] and all(p['terminated'] for p in family['processes'])
   else:
    assert report['first_entry_order']==[symbols['_control_parent'],symbols['_control_common'],symbols['_control_child']],report
    assert report['terminal']=='exit:0' and family['frontier_pid'] is None and not any(p['terminated'] for p in family['processes'])
    assert family['processes'][1]['exit_status']==7
    run('positive-native',[TOOL,'run',WORK/'case.so','--capture',WORK/'success.bin','--original-report',WORK/'success.json','--report',WORK/'native.json'])
    native=json.loads((WORK/'native.json').read_text())[0];assert native['pass'] and native['first_entry_order']==report['first_entry_order']
  print(mode,report.get('terminal',report.get('error','error'))[:180])
print('control prepared',WORK)
