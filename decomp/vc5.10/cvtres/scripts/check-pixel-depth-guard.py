#!/usr/bin/env python3
"""One native-only direct rejection probe; no public input/reachability claim."""
import argparse,json,os,subprocess,tempfile,time
from pathlib import Path
from mask import BINARY,sha
from evidence import verify_native_int3
p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args();start=time.perf_counter();exe=BINARY/'build/native/CVTRES';assert exe.exists()
with tempfile.TemporaryDirectory(prefix='pixel-depth-guard-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
 out=Path(temporary);provider=out/'resources'
 subprocess.run(['python3','scripts/prepare-resources.py','--output',str(provider)],cwd=BINARY,check=True,stdout=subprocess.DEVNULL)
 statefile=out/'state.json';script=out/'probe.py'
 script.write_text('''import gdb,json
gdb.execute("start")
try:gdb.execute("call read_pixel((const unsigned char *)0, 0, (unsigned short)3)")
except gdb.error:pass
frame=gdb.newest_frame()
state=dict(function=frame.name(),pc=int(frame.pc()),stop_signal=int(gdb.parse_and_eval("$_siginfo.si_signo")))
open(STATE,"w").write(json.dumps(state))
'''.replace('STATE',repr(str(statefile))))
 env=os.environ.copy();env.update(CVTRES_RESOURCE_PROVIDER=str(provider),RECON_PROGRAM_PATH=r'R:\toolchains\5.10\bin\CVTRES.EXE');env.pop('CVTRES_INITIAL_TRAP',None);env.pop('RECON_TRACE_FD',None)
 result=subprocess.run(['gdb','-q','-nx','-batch',str(exe),'-ex','set pagination off','-ex','set confirm off','-ex','set args /help','-ex','source '+str(script)],cwd=out,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
 assert result.returncode==0 and statefile.exists(),(result.stdout+result.stderr).decode()
 state=json.loads(statefile.read_text());assert state['stop_signal']==5 and state['function']=='read_pixel','unsupported depth did not trap in read_pixel';assert b'Program received signal SIGTRAP' in result.stdout
 verify_native_int3(state,exe)
 record=dict(schema=1,passed=True,scope='One explicit native debugger call with unsupported depth3 and no pixel dereference; no original capture or public16bpp reachability claim.',native_sha256=sha(exe.read_bytes()),state=state,raw_gdb_stdout_sha256=sha(result.stdout),raw_gdb_stderr_sha256=sha(result.stderr),raw_gdb_stdout=result.stdout.decode(),source_sha256=sha((BINARY/'src/pixels.c').read_bytes()),checker_sha256=sha(Path(__file__).read_bytes()),seconds=time.perf_counter()-start)
 a.output.write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(dict(passed=True,state=state,seconds=record['seconds']),indent=2))
