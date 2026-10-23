from pathlib import Path
import tempfile,shutil,subprocess,json,time
root=Path('/root/.ai-tmp/vc1.0-cvpack');start=time.perf_counter()
for label,expected in [('header','concrete application global differs'),('missing-global','consumed global observation inventory differs'),('trace','complete entry trace differs'),('artifact','full artifact differs')]:
 with tempfile.TemporaryDirectory(prefix='0099-control-',dir=root)as tmp:
  native=Path(tmp)/'native';shutil.copytree(root/'0099-native',native);work=native/'hello-debug-dll'
  if label in('header','missing-global'):
   path=work/'gdb.json';state=json.loads(path.read_text())
   if label=='header':state['globals']['strip_file_header']='00'+state['globals']['strip_file_header'][2:]
   else:state['globals'].pop('strip_file_header')
   path.write_text(json.dumps(state))
  elif label=='trace':
   path=work/'trace';path.write_text(path.read_text().replace('strip_debug\n',''))
  else:
   path=work/'work/hello-debug.dll';raw=bytearray(path.read_bytes());raw[0]^=1;path.write_bytes(raw)
  result=subprocess.run(['python3','scripts/accept.py','--check-only','--function','strip_debug','--case','hello-debug-dll','--original',str(root/'0099-original'),'--native',str(native)],capture_output=True)
  assert result.returncode and expected.encode()in result.stderr,(label,result.stdout,result.stderr)
  print('rejected:',label,expected)
print('elapsed',time.perf_counter()-start)
