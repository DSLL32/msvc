import pathlib,subprocess,time,json,os
root=pathlib.Path(__file__).resolve().parents[4];lib=root/'lib/recon';out=pathlib.Path.home()/'.ai-tmp/msvc2/canonical-system/results';out.mkdir(parents=True,exist_ok=True)
rows=[];start=time.monotonic()
for cc in ['gcc','clang']:
 exe=out/cc;t=time.monotonic();command=[cc,'-m32','-std=gnu89','-O0','-g','-DRECON_NATIVE_TEST','-DRECON_CRT_PROFILE_MSVCRT_510','-DRECON_DEFAULT_FMODE=0',f'-I{lib}/include',f'-I{lib}/native',str(lib/'tests/native-system/probe.c'),*[str(lib/'src'/f'{n}.c') for n in ['crt','win32','process','compat','entry','decimal']],'-pthread','-ldl','-o',str(exe)]
 p=subprocess.run(command,capture_output=True);(out/f'{cc}-build.log').write_bytes(p.stdout+p.stderr);assert p.returncode==0,p.stderr
 build=time.monotonic()-t;t=time.monotonic()
 for mode,status in [('',0),('limit',0),('null',125),('unaligned',125),('wrap',125)]:
  p=subprocess.run([str(exe)]+([mode] if mode else []),cwd=out,env={'TMP':'.','MSC_CMD_FLAGS':'only'},capture_output=True);(out/f'{cc}-{mode or "normal"}.log').write_bytes(p.stdout+p.stderr);assert p.returncode==status,(cc,mode,p.returncode,p.stderr)
 rows.append(dict(compiler=cc,build_s=build,checks_s=time.monotonic()-t))
result=dict(groups=rows,total_s=time.monotonic()-start);(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
