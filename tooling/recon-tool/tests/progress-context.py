#!/usr/bin/env python3
"""Selected progress context across inspect, units and a real native SELF exec.
Synthetic report controls only; no original/Wine is executed.
"""
import hashlib,json,os,subprocess,tempfile,shutil,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
TOOL=Path(os.environ.get('RECON_TOOL_TEST_EXE',ROOT/'target/i686-unknown-linux-gnu/debug/recon-tool')).resolve()
FIXTURE=ROOT/'tests/native-self'
WORK=Path(tempfile.mkdtemp(prefix='progress-context-',dir=Path.home()/'.ai-tmp'))
start=time.monotonic()
def run(args,ok=True,error=None):
 p=subprocess.run(list(map(str,args)),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 assert (p.returncode==0)==ok,p.stdout
 if error:assert error in p.stdout,p.stdout
 return p.stdout
try:
 run(['make','-C',FIXTURE,'-j4','cases','RECON_TOOL='+str(TOOL),'-o',TOOL])
 source=(FIXTURE/'harness/case.h').read_text().replace('RECON_IO,RECON_EXIT,0,setup','RECON_IO,RECON_FRONTIER,0x1000,setup')
 (WORK/'stale.c').write_text('#define REQUEST_COUNT 1\n'+source)
 case=WORK/'stale.so'
 run(['gcc','-m32','-fPIC','-shared','-I'+str(ROOT/'include'),WORK/'stale.c',FIXTURE/'.build/libbinary.so','-Wl,-rpath,'+str(FIXTURE/'.build'),'-o',case])
 value=dict(args=['T:\\self.exe'],stdin='text:payload\ntail\n',stdout='text:parent\nchild\nerrno-exit:99\nparent-status:0\nerrno-exit:99\n',stderr='text:child diagnostic\n',exit_code=0,reject_unexpected_files=True,timeout_ms=2000,
 env={'RECON_SELF_SPAWN':'1','RECON_PROGRAM_PATH':'T:\\self.exe','SELF_MARKER':'inherited','SELF_CASE':'success','SELF_ERRNO':'1'},logical_cwd='S:\\native-self',assets=[dict(path='marker',data='text:A')])
 (WORK/'input.json').write_text(json.dumps(value));capture=WORK/'input.bin'
 run([TOOL,'capture','pack',WORK/'input.json',capture])
 run([TOOL,'inspect',case],False,'frontier must name an unimplemented')
 info=json.loads(run([TOOL,'inspect',case,'--progress']))
 receipt=dict(case=info['case'],oracle_written=True,terminal='exit:0',first_entry_order=[0x1000,0x1100,0x1200],identity={k:info[k] for k in ['case','binary','original_path','original_sha256','execution','first_entry_seen']},capture_sha256=hashlib.sha256(capture.read_bytes()).hexdigest())
 report=WORK/'original.json';report.write_text(json.dumps(receipt))
 selected=json.loads(run([TOOL,'inspect',case,'--capture',capture,'--original-report',report]))
 assert selected['terminal']==0 and selected['first_entry_order']==[0x1000,0x1100,0x1200]
 run([TOOL,'run',case,'--capture',capture],False,'frontier must name an unimplemented')
 native=WORK/'native.json'
 run([TOOL,'run',case,'--capture',capture,'--original-report',report,'--report',native])
 actual=json.loads(native.read_text())[0]
 assert actual['pass'] and len(actual['process_requests']['self_children'])==1
 assert actual['process_requests']['self_children'][0]['terminal']=='exit:0'
 assert not Path(actual['workspace']).exists()
 print(f'selected stale-descriptor inspect + genuine native SELF PASS {time.monotonic()-start:.3f} s; no original execution')
finally:shutil.rmtree(WORK)
