from pathlib import Path
import subprocess,sys
for binary in sys.argv[1:]:
    result=subprocess.run([str(Path(binary).resolve())],capture_output=True)
    assert result.returncode==0 and not result.stdout,(binary,result)
    assert result.stderr.count(b'recon: unsupported native operation:')==3,(binary,result.stderr)
print('C/C++ size_t cdecl typing, NULL/invalid denial and owned-buffer no-write retention pass')
