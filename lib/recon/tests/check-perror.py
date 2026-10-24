import pathlib,subprocess,sys,tempfile
exe=str(pathlib.Path(sys.argv[1]).resolve());profile=sys.argv[2]
message=b'No such file or directory\r\n'
def run(mode,live=False,pipe=False):
 with tempfile.TemporaryDirectory() as d:
  path=pathlib.Path(d)/'stderr'
  if pipe:
   p=subprocess.run([exe,mode]+(['live']if live else[]),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
   return p,p.stderr
  with path.open('wb')as f:p=subprocess.run([exe,mode]+(['live']if live else[]),stdout=subprocess.PIPE,stderr=f)
  return p,path.read_bytes()
if profile=='default':
 for mode,prefix in [('actual',b'missing/log.txt: '),('null',b''),('empty',b''),('ascii',b'A prefix: ')]:
  for live in (False,True):
   p,data=run(mode,live)
   assert p.returncode==0 and data==prefix+message and b'108FP PASS'in p.stdout,(mode,live,p.returncode,p.stdout,data)
 modes=['errno','dos','last','mbcp','mbgate','lccp','mbmax','locale','ctype','binary','unregistered','handle','nhandle','append','readonly','nonascii','write-failure']
else:modes=['actual','null','empty','ascii']
for mode in modes:
 p,data=run(mode)
 assert p.returncode==125 and not p.stdout,(mode,p.returncode,p.stdout,data)
 if mode not in ('write-failure','readonly'):
  assert data.startswith(b'recon: unsupported native operation:'),(mode,data)
if profile=='default':
 p,data=run('actual',pipe=True)
 assert p.returncode==125 and b'perror registered text stderr'in data,(p.returncode,data)
print(f'perror {profile}: exact physical CRLF/prefix branches/fullFILE-lowio-inputs-tables/errors/full108FP, {len(modes)} scope denials PASS')
