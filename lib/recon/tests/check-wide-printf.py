import os,subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
expected=(b'adding resource -- type:"CUSTOMTYPE", name:"NAMEDDATA", name:"NAMEDKNOWN", '
          b'duplicate resource -- type:"CUSTOMTYPE", name:"NAMEDDATA", language:0x0409\r\nZ|!|beef|%')
with tempfile.TemporaryDirectory() as tmp:
    output=Path(tmp)/'output.bin'
    with output.open('wb') as f:p=subprocess.run([binary],stdout=f,stderr=subprocess.PIPE)
    assert p.returncode==0 and not p.stderr,(p.returncode,p.stderr)
    assert output.read_bytes()==expected,output.read_bytes()
    for mode in ('width','grammar','nonascii','locale','codepage','null','char'):
        p=subprocess.run([binary],env=dict(os.environ,RECON_WIDE_PRINTF_NEGATIVE=mode),capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
print('five actual WCHAR16/mixed-argument returns and pending-byte counts exact; changed arguments/order/CRLF and7 unsupported scopes checked')
