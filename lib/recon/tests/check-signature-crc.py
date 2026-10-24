import hashlib,json,struct,subprocess,sys,tempfile,zlib
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve());reference=Path(sys.argv[2]);tablebytes=(reference/'signature-provider-table-000.bin').read_bytes()
assert hashlib.sha256((reference/'receipt.json').read_bytes()).hexdigest()=='e51a5362c6bebc64f287e69292360ff64d591da1bb829a40c50d4d7c8c7bb9d2'
assert len(tablebytes)==1024 and tablebytes==(reference/'signature-provider-table-after-000.bin').read_bytes()
receipt=json.loads((reference/'receipt.json').read_text())
for filename in ('signature-provider-table-000.bin','signature-provider-table-after-000.bin','signature-reference-return-registers.bin'):
 data=(reference/filename).read_bytes();record=receipt['artifacts'][filename]
 assert len(data)==record['size'] and hashlib.sha256(data).hexdigest()==record['sha256']
table=struct.unpack('<256I',tablebytes)
# Test oracle uses the authenticated canonical table; production uses only the
# polynomial. No provider table is installed in native target state/source.
for index,value in enumerate(table):
 generated=index
 for bit in range(8):generated=(generated>>1)^(0xedb88320 if generated&1 else 0)
 assert generated==value
registers=struct.unpack('<16I',(reference/'signature-reference-return-registers.bin').read_bytes());actual=registers[3];assert actual==0xc2280058
def expected(data,seed):
 original=seed
 for byte in data:seed=(seed>>8)^table[(seed^byte)&255]
 assert seed==(zlib.crc32(data,original^0xffffffff)^0xffffffff)
 return seed
rows=[(b'C',0xffffffff,actual)]
for seed in (0,0xffffffff):
 for byte in range(256):
  data=bytes([byte]);rows.append((data,seed,expected(data,seed)))
for data in (b'',b'C',b'CPP',b'123456789',bytes(range(256)),bytes(range(255,-1,-1)),bytes([255])*1024):
 for seed in (0,1,0x12345678,0x80000000,0xffffffff):rows.append((data,seed,expected(data,seed)))
with tempfile.TemporaryDirectory()as temporary:
 vectors=Path(temporary)/'vectors.bin';vectors.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<3I',seed,len(data),result)+data for data,seed,result in rows))
 for mode in ('','live','zero-poison','negative','min','null','wrap'):
  result=subprocess.run([binary,str(vectors),mode],capture_output=True)
  supported=mode in ('','live','zero-poison');assert result.returncode==(0 if supported else 125),(mode,result.returncode,result.stdout,result.stderr)
  if supported:
   assert not result.stderr and result.stdout==(b'zero count no read, seed/errors/fullFP preserved\n'if mode=='zero-poison'else b'canonical CRC vectors/input tails/errors/fullFP preserved\n')
  else:assert b'recon: unsupported native operation:'in result.stderr
print(str(len(rows))+' canonical/actualC vectors x empty/live; zero-no-read and4denials PASS. CPP rows are offline algebra, not C1XX original oracle.')
