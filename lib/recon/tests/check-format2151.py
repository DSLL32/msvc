import importlib.util,pathlib,subprocess,sys,tempfile,time
start=time.monotonic();root=pathlib.Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('resources',root/'lib/recon/resources.py');resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
owner='62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6'
text='%1 (%2!d!): error RC2151 : cannot reuse string constants, %3!d!(0x%3!X!) - "%4!ws!" already defined. Cannot add %3!d!(0x%3!X!) - "%5!ws!".\r\n'
binary=str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp') as temporary:
    provider=pathlib.Path(temporary)/'provider'
    resources.write_resources(provider,owner,[dict(kind='message',id=2151,language=1033,text=text)])
    p=subprocess.run([binary,str(provider)],capture_output=True)
    assert p.returncode==0 and not p.stdout,(p.returncode,p.stdout,p.stderr)
    assert p.stderr.count(b'unsupported native operation:') in (27,28),p.stderr
print('registered2151 actual/WORD0-17-65535/repeated decimal+uppercasehex/signedline edges/ANSI+twoWCHAR spans/literal percent/count/NUL/tails/errors+semanticFP/27 denials/profile isolation PASS %.6fs'%(time.monotonic()-start))
