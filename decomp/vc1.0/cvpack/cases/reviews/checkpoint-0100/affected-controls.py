from pathlib import Path
import tempfile,shutil,subprocess,json,time
root=Path('/root/.ai-tmp/vc1.0-cvpack');start=time.perf_counter()
for label,expected in [('output-pointer','native raw output address differs from local base'),('section-rva','observed object bytes differ')]:
 with tempfile.TemporaryDirectory(prefix='0100-control-',dir=root)as tmp:
  native=Path(tmp)/'native';shutil.copytree(root/'0100-corrected-native',native);path=native/'hello-debug-dll/gdb.json';state=json.loads(path.read_text())
  if label=='output-pointer':state['parameters']['symbol']['address']+=1
  else:
   row=state['walks']['strip_section_rvas'][0];raw=bytearray.fromhex(row['bytes']);raw[4]^=1;row['bytes']=raw.hex()
  path.write_text(json.dumps(state));result=subprocess.run(['python3','scripts/accept.py','--check-only','--function','initialize_strip_sections','--case','hello-debug-dll','--original',str(root/'0100-corrected-original'),'--native',str(native)],capture_output=True)
  assert result.returncode and expected.encode()in result.stderr,(label,result.stdout,result.stderr)
  print('rejected:',label,expected)
print('elapsed',time.perf_counter()-start)
