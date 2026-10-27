from pathlib import Path
import subprocess,sys
for binary in sys.argv[1:]:
    p=subprocess.run([str(Path(binary).resolve())],capture_output=True)
    assert p.returncode==125 and not p.stdout,(binary,p.returncode,p.stdout,p.stderr)
    assert b'unreviewed OLE32 CoUninitialize' in p.stderr
print('OLE32 void WINAPI(void) C/C++ typing and unconditional denial pass; no COM behavior')
