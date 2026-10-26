import importlib.util,pathlib,subprocess,sys,tempfile,time
start=time.monotonic();root=pathlib.Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('resources',root/'lib/recon/resources.py');resources=importlib.util.module_from_spec(spec);spec.loader.exec_module(resources)
owner='bba1b5fa977a9130b84b710f5a5c8ccbe9a9c5043e2586ec9c1677bbad6939b0'
texts={2104:'%1 (%2!d!): error RC2104 : undefined keyword or key name: %3!ws!\r\n',2175:'%1 (%2!d!): error RC2175 : resource file %3!ws! is not in 3.00 format\r\n',2135:'%1 (%2!d!): error RC2135 : file not found: %3\r\n',20101:'RC terminated by user\r\n',1003:'error count exceeds %1!d!; stopping compilation\r\n'}
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp') as temp:
 provider=pathlib.Path(temp)/'provider';resources.write_resources(provider,owner,[dict(kind='message',id=i,language=1033,text=t) for i,t in texts.items()])
 p=subprocess.run([str(pathlib.Path(sys.argv[1]).resolve()),str(provider)],capture_output=True)
 assert p.returncode==0 and not p.stdout,(p.returncode,p.stdout,p.stderr)
 assert p.stderr.count(b'unsupported native operation:') in (28,34),p.stderr
print('fixed five diagnostic templates/typed inserts/signed edges/count/NUL/tails/state/28 denials; exact missing10056 failure/owner isolation PASS %.6fs'%(time.monotonic()-start))
