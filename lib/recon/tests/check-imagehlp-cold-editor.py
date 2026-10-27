from pathlib import Path
import subprocess,sys
for binary in sys.argv[1:]:
    p=subprocess.run([str(Path(binary).resolve())],capture_output=True)
    assert p.returncode==0 and not p.stdout,(binary,p.returncode,p.stdout,p.stderr)
    assert p.stderr.count(b'recon: unsupported native operation:')==9,(binary,p.stderr)
print('IMAGEHLP WINAPI3/5/11, stdcall callback, checksum ABI and no-access/no-callback/output denials pass')
