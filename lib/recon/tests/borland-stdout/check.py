import pathlib,subprocess,time,json,concurrent.futures,tempfile,argparse,os,struct,pty,termios
root=pathlib.Path(__file__).resolve().parents[4];lib=root/'lib/recon';parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path);args=parser.parse_args();base=pathlib.Path.home()/'.ai-tmp/msvc2/borland-stdout-check';base.mkdir(parents=True,exist_ok=True);out=args.output or pathlib.Path(tempfile.mkdtemp(prefix='run-',dir=base));out.mkdir(parents=True,exist_ok=True)
def contents(length):return (b'A%\nB\r\n'*((length+5)//6))[:length]
def group(item):
 cc,profile=item;folder=out/f'{cc}-{profile}';folder.mkdir();exe=folder/'probe';defs=[]
 if profile=='borland':defs=['-DRECON_BORLAND_400_STDOUT']
 elif profile=='510':defs=['-DRECON_CRT_PROFILE_MSVCRT_510']
 elif profile=='watcom':defs=['-DRECON_CRT_PROFILE_WATCOM_100A']
 flags=['-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST',f'-I{lib}/include',f'-I{lib}/native','-Werror=implicit-function-declaration'];start=time.monotonic()
 p=subprocess.run([cc,*flags,*defs,str(lib/'tests/borland-stdout/probe.c'),str(lib/'tests/borland-stdout/consumer.c'),*[str(lib/'src'/f'{n}.c') for n in ['compat','crt','decimal','entry','process','win32']],'-pthread','-ldl','-o',str(exe)],capture_output=True);(folder/'build.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,p.stderr;build=time.monotonic()-start;start=time.monotonic()
 def run(mode,lengths,visible,code=0,kind='file'):
  master=slave=None;stream=None
  if kind=='file':stream=tempfile.TemporaryFile(dir=folder);sink=stream
  elif kind=='pty':
   master,slave=pty.openpty();settings=termios.tcgetattr(slave);settings[1]&=~termios.OPOST;termios.tcsetattr(slave,termios.TCSANOW,settings);sink=slave
  elif kind=='null':sink=subprocess.DEVNULL
  elif kind=='full':stream=open('/dev/full','wb');sink=stream
  else:sink=subprocess.PIPE
  fd=os.memfd_create('status',0);os.ftruncate(fd,24)
  try:
   p=subprocess.Popen([str(exe),mode,str(code),*map(str,lengths)],stdout=sink,stderr=subprocess.PIPE,env={'RECON_STATUS_FD':str(fd)},pass_fds=(fd,));captured,err=p.communicate(timeout=10);receipt=struct.unpack('<6I',os.pread(fd,24,0))
   if kind=='pty':
    os.close(slave);data=b''
    while True:
     try:part=os.read(master,4096)
     except OSError:break
     if not part:break
     data+=part
    os.close(master)
   elif kind=='file':stream.seek(0);data=stream.read()
   elif kind in ['null','full']:data=b''
   else:data=captured
  finally:
   os.close(fd)
   if stream:stream.close()
  assert p.returncode==code&255,(mode,lengths,kind,p.returncode,err)
  assert receipt==(0x31535452,1,p.pid,1,code,0),(mode,lengths,kind,receipt)
  expected=b''.join(map(contents,lengths))[:visible].replace(b'\n',b'\r\n')
  if kind in ['null','full']:expected=b''
  assert data==expected,(mode,lengths,kind,len(data),len(expected));assert err==(b'atexit\n' if mode=='normal' else b''),(mode,err)
 if profile=='borland':
  # Explicit source-derived boundary predictions, independent of helper code:
  # a512-byte write fits exactly, the513th byte's33-byte formatter chunk
  # preflushes480; a new2-byte call preflushes an existing511-byte payload.
  for lengths,visible in [([0],0),([80],0),([511],0),([512],0),([513],480),([511,1],0),([511,2],511),([400,112,1],512),([560],480),([79,1,80,352,1],512),([65535],65280)]:
   for kind in ['file','pipe']:run('quick',lengths,visible,kind=kind)
  run('normal',[513,5],518);run('flush',[513,5],518)
  run('quick',[80],80,kind='pty');run('quick',[513],513,kind='pty');run('normal',[513,5],518,kind='pty')
  run('quick',[513,5],0,kind='null');run('write-error',[],0,kind='full');run('closed',[],0)
  for code in [1100,0xffffffff]:run('quick',[80],0,code)
  for mode in ['format','null','wrap','unterminated','dirty','state','overlap']:
   with tempfile.TemporaryFile(dir=folder) as stream:
    p=subprocess.run([str(exe),mode,'0'],stdout=stream,stderr=subprocess.PIPE);stream.seek(0);data=stream.read()
   assert p.returncode==125 and not data and b'Borland400' in p.stderr,(mode,p.returncode,p.stderr,data)
 else:
  for mode in ['printf','quick']:
   p=subprocess.run([str(exe),mode],capture_output=True);assert p.returncode==125 and b'Borland400' in p.stderr
 cxx='g++' if cc=='gcc' else 'clang++';p=subprocess.run([cxx,'-m32','-std=c++98','-nostdinc++','-DRECON_NATIVE_TEST',*defs,f'-I{lib}/include',f'-I{lib}/native','-fsyntax-only',str(lib/'tests/borland-stdout/typing.cpp')],capture_output=True);(folder/'typing.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,p.stderr
 return dict(compiler=cc,profile=profile,build_s=build,checks_s=time.monotonic()-start)
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:groups=list(pool.map(group,[('gcc','borland'),('clang','borland'),('gcc','default'),('gcc','510'),('clang','watcom')]))
result=dict(groups=groups,total_s=time.monotonic()-start);(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
