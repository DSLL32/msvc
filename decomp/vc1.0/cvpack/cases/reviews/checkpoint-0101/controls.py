from pathlib import Path
import tempfile,shutil,subprocess,json,time
r=Path('/root/.ai-tmp/vc1.0-cvpack');start=time.perf_counter()
with tempfile.TemporaryDirectory(prefix='0101-control-',dir=r)as tmp:
 native=Path(tmp)/'native';shutil.copytree(r/'0101-native',native);p=native/'hello-debug-dll/gdb.json';s=json.loads(p.read_text());raw=bytearray.fromhex(s['parameters']['symbol']['prebytes']);raw[0]^=1;s['parameters']['symbol']['prebytes']=raw.hex();p.write_text(json.dumps(s));result=subprocess.run(['python3','scripts/accept.py','--check-only','--function','read_coff_symbol','--case','hello-debug-dll','--original',str(r/'0101-original'),'--native',str(native)],capture_output=True);assert result.returncode and b'initialized stack output local differs'in result.stderr,result.stderr
print('rejected: changed fully defined COFF input record vs independent frozen bytes');print('elapsed',time.perf_counter()-start)
