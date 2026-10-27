import pathlib,subprocess,time,json,os,pty,termios,tempfile
root=pathlib.Path(__file__).resolve().parents[4];lib=root/'lib/recon';out=pathlib.Path.home()/'.ai-tmp/msvc2/native-prepare';out.mkdir(parents=True,exist_ok=True)
rows=[];start=time.monotonic()
for cc in ['gcc','clang']:
 exe=out/cc;t=time.monotonic();command=[cc,'-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST','-DRECON_CRT_PROFILE_WATCOM_100A',f'-I{lib}/include',f'-I{lib}/native',str(lib/'tests/native-prepare/probe.c'),*[str(lib/'src'/f'{n}.c') for n in ['crt','win32','process','compat','entry','decimal']],'-pthread','-ldl','-o',str(exe)]
 p=subprocess.run(command,capture_output=True);(out/f'{cc}-build.log').write_bytes(p.stdout+p.stderr);assert p.returncode==0,p.stderr
 build=time.monotonic()-t;t=time.monotonic()
 for mode,terminal,status in [('prepare',False,0),('legacy',False,0),('rebind',False,125),('tty',True,0),('wide',True,125),('unprepared',True,125),('fake',False,125)]:
  master=slave=None
  if terminal:
   master,slave=pty.openpty();attrs=termios.tcgetattr(slave);attrs[1]&=~termios.OPOST;termios.tcsetattr(slave,termios.TCSANOW,attrs);stdout=slave
  else: stdout=subprocess.PIPE
  p=subprocess.run([str(exe),mode],env={'FIRST':'one','SECOND':'two'},stdin=subprocess.DEVNULL,stdout=stdout,stderr=subprocess.PIPE)
  if terminal:
   os.close(slave)
   try: data=os.read(master,4096)
   except OSError:data=b''
   os.close(master)
  else:data=p.stdout
  (out/f'{cc}-{mode}.log').write_bytes(data+p.stderr);assert p.returncode==status,(cc,mode,p.returncode,data,p.stderr)
  assert data==(b'tty\r\n' if mode=='tty' else b''),(mode,data)
 rows.append(dict(compiler=cc,build_s=build,checks_s=time.monotonic()-t))
result=dict(groups=rows,total_s=time.monotonic()-start);(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
