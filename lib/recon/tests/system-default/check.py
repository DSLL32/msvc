import pathlib,subprocess,time,json,tempfile,argparse
root=pathlib.Path(__file__).resolve().parents[4];lib=root/'lib/recon';p=argparse.ArgumentParser();p.add_argument('--output',type=pathlib.Path);args=p.parse_args();base=pathlib.Path.home()/'.ai-tmp/msvc2/system-default-check';base.mkdir(parents=True,exist_ok=True);out=args.output or pathlib.Path(tempfile.mkdtemp(prefix='run-',dir=base));out.mkdir(parents=True,exist_ok=True);rows=[];start=time.monotonic()
for cc in ['gcc','clang']:
 folder=out/cc;folder.mkdir();exe=folder/'probe';t=time.monotonic();p=subprocess.run([cc,'-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST',f'-I{lib}/include',f'-I{lib}/native',str(lib/'tests/system-default/probe.c'),*[str(lib/'src'/f'{n}.c') for n in ['crt','win32','compat','decimal','entry','process']],'-pthread','-ldl','-o',str(exe)],capture_output=True);(folder/'build.log').write_bytes(p.stdout+p.stderr);assert not p.returncode,p.stderr;build=time.monotonic()-t;t=time.monotonic()
 for mode,status in [('',0),('null',125),('wrap',125),('unaligned',125)]:
  p=subprocess.run([str(exe)]+([mode] if mode else []),capture_output=True,env={});(folder/f'{mode or "host"}.log').write_bytes(p.stdout+p.stderr);assert p.returncode==status,(cc,mode,p.returncode,p.stderr)
 rows.append(dict(compiler=cc,build_s=build,checks_s=time.monotonic()-t))
result=dict(total_s=time.monotonic()-start,groups=rows);(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
