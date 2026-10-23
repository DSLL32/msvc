import subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    output=Path(tmp)/'stdout.bin'
    with output.open('wb') as stream:
        p=subprocess.run([binary],stdout=stream,stderr=subprocess.PIPE)
    assert p.returncode==0 and not p.stderr,(p.returncode,p.stderr)
    assert output.read_bytes()==b'CVTRES : warning CVT4001: machine type not specified; assumed IX86\r\nARM /WINDOWSCE -7',output.read_bytes()
print('mixed printf/vprintf share pending stdout buffer, correct counts/actual arguments/order/CRLF')
