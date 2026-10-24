#!/usr/bin/env python3
"""Explicit native GDB image-helper ABI/state observation, never a fixture."""
import argparse,hashlib,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-image-checksum'
def main():
    p=argparse.ArgumentParser();p.add_argument('--trial',action='append',default=[]);p.add_argument('--support',action='append',default=[]);a=p.parse_args();cfg=json.loads((BINARY/'functions.json').read_text());allowed={n[1:] for n in cfg['accepted']}|set(a.trial);support={n for n in ['__fileno','__filelength'] if n in cfg['accepted'] or n in a.support}
    d=Path(tempfile.mkdtemp(prefix='image-checksum-gdb-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(d,'S:\\vc1.0\\link\\direct-image-checksum');shutil.copyfile(CASE/'inputs/client.exe',work/'client.exe');(d/'stages').mkdir()
    source=r'''
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];out=Path(OUTPUT)
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4))) if n else []
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snap(name,event):
 esp=val('$esp');phase=val('checksum_test_phase');workspace=val('&checksum_test_workspace.image');state=words(workspace,9);n=4 if name=='map_bind_image' else (1 if name in ['make_import_section_read_only','recon_UnmapViewOfFile','recon_CloseHandle','recon_filelength','recon_fileno'] else 3)
 e=dict(event=event,name=name,phase=phase,formal_words=words(esp+4,n) if event!='return' else [],esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),caller=words(esp,1)[0],workspace_address=workspace,workspace=state,workspace_guards=mem(workspace-4,56).hex(),names_address=val('&checksum_test_names'),names=mem(val('&checksum_test_names'),64).hex(),output_address=val('&checksum_test_outputs'),outputs=mem(val('&checksum_test_outputs'),16).hex(),errors=[val('*checksum_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],fp_control=val('$fctrl'),fp_status=val('$fstat'),fp_tag=val('$ftag'),fp_raw=gdb.execute('info float',to_string=True))
 live=phase==2 or (phase==1 and event=='return') or (phase==3 and event!='return');e['mapping_live']=int(live)
 if name=='recon_fileno':e['stream_descriptor']=words(e['formal_words'][0]+16,1)[0] if event!='return' else None
 if live:
  assert state[7]<=16;e['headers']=mem(state[5],248).hex();e['sections']=mem(state[8],state[7]*40).hex()
 return e
class Return(gdb.Breakpoint):
 def __init__(self,before):self.before=before;super().__init__('*'+str(before['caller']),internal=True)
 def stop(self):
  e=snap(self.before['name'],'return');e['formal_words']=self.before['formal_words'];
  if e['name']=='recon_fileno':e['stream_descriptor']=words(e['formal_words'][0]+16,1)[0]
  e['actual_caller']=self.before.get('actual_caller');events.append(e);persist();popped=8 if e['name'] in ['recon_UnmapViewOfFile','recon_CloseHandle'] else 4
  assert e['esp']==self.before['esp']+popped and all(e[r]==self.before[r] for r in ['ebx','esi','edi','ebp']);self.enabled=False;return False
class Entry(gdb.Breakpoint):
 def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
 def stop(self):
  caller=gdb.newest_frame().older();owner=caller.name() if caller else None
  if self.name in ['recon_filelength','recon_fileno'] and owner!='checksum_image_file':return False
  if self.name in ['recon_UnmapViewOfFile','recon_CloseHandle'] and owner!='__wrap_main':return False
  accepted=(('__filelength' if self.name=='recon_filelength' else '__fileno') in SUPPORT if self.name in ['recon_filelength','recon_fileno'] else self.name in ALLOWED or self.name in ['map_bind_image','recon_UnmapViewOfFile','recon_CloseHandle'])
  e=snap(self.name,'entry' if accepted else 'frontier');e['actual_caller']=owner;events.append(e);persist()
  if not accepted:return True
  Return(e);return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited);gdb.execute('set pagination off');gdb.execute('set confirm off')
for n in ['checksum_image_file','make_import_section_read_only','map_bind_image','recon_UnmapViewOfFile','recon_CloseHandle','recon_filelength','recon_fileno']:Entry(n)
gdb.execute('run '+STAGES+' '+REPORT+' >'+STDOUT+' 2>'+STDERR);persist();assert events
'''
    for k,v in dict(OUTPUT=str(d/'state.json'),ALLOWED=allowed,SUPPORT=support,STAGES=str(d/'stages'),REPORT=str(d/'results.bin'),STDOUT=str(d/'stdout.bin'),STDERR=str(d/'stderr.bin')).items():source=source.replace(k,repr(v))
    (d/'observe.py').write_text(source);env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,TZ='PST8PDT',LC_ALL='C')
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO']:env.pop(k,None)
    exe=BINARY/'build/native/image-checksum';command=['gdb','-q','-nx','-batch','-ex','file '+str(exe),'-ex','source '+str(d/'observe.py')];t=time.perf_counter();r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=15);elapsed=time.perf_counter()-t
    (d/'gdb.stdout').write_bytes(r.stdout);(d/'gdb.stderr').write_bytes(r.stderr);(d/'receipt.json').write_text(json.dumps(dict(command=command,seconds=elapsed,status=r.returncode,trial=a.trial,support=a.support,elf_sha256=hashlib.sha256(exe.read_bytes()).hexdigest()),indent=2)+'\n');print(d,flush=True);assert r.returncode==0,(r.stdout,r.stderr)
if __name__=='__main__':main()
