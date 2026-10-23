from pathlib import Path
import subprocess,tempfile,shutil,re,time
r=Path('/root/.ai-tmp/vc1.0-cvpack');start=time.perf_counter()
with tempfile.TemporaryDirectory(prefix='0106-control-',dir=r)as tmp:
 original=Path(tmp)/'original';shutil.copytree(r/'0106-original',original,symlinks=True);p=original/'events';s=p.read_text();changed=re.sub(r'^(registers .* ebx )[0-9a-f]+',r'\g<1>00000001',s,flags=re.M);assert changed!=s;p.write_text(changed)
 child=subprocess.run(['python3','scripts/accept.py','--check-only','--function','read_coff_auxiliary','--case','hello-debug-dll','--original',str(original),'--native',str(r/'0106-native')],capture_output=True);assert child.returncode and b'output pointer differs from original live allocation carrier'in child.stderr,child.stderr
print('rejected: mismatched original malloc-result carrier vs constructor output pointer');print('elapsed',time.perf_counter()-start)
