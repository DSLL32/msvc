"""Current-directory temp creation, collisions and scope; native only."""
import os,pathlib,subprocess,sys,tempfile,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
changed=len(sys.argv)>3 and sys.argv[3]=='changed'
with tempfile.TemporaryDirectory() as temporary:
    root=pathlib.Path(temporary);cwd=root/'case';cwd.mkdir()
    env=dict(os.environ,RECON_DRIVE_S=temporary,RECON_CANONICAL_CWD='S:\\case')
    ordinary=('', 'live','capacity','null','wrap','env-overlap','unbound','environment','unique','prefix','directory','overlap','file-null','file-wrap','hello','hello-live','short-prefix','long-prefix','prefix-wrap','hello-overlap')
    checks=[([mode] if mode else [],mode in ('','live','hello','hello-live','short-prefix','long-prefix')) for mode in ordinary]
    prefixes=['','a','ab','xyz','xyz-long','x'*259,'abc/ignored','a/b','a:b','*','\x01','\x7f','xyz\x80','x'*260]
    checks += [(['ascii-prefix',text],i<7) for i,text in enumerate(prefixes)]
    if changed:checks=[(['hello'],True),(['hello-overlap'],False)]+[(['ascii-prefix',text],i<7) for i,text in enumerate(prefixes)]
    for args,valid in checks:
        mode=args[0] if args else ''
        p=subprocess.run([binary]+args,cwd=cwd,env=env,capture_output=True)
        expected=0 if admitted and valid else 125
        assert p.returncode==expected and not p.stdout,(mode,p)
        assert b'unsupported native operation:' in p.stderr if expected else not p.stderr,(mode,p)
        assert not list(cwd.iterdir()),(mode,list(cwd.iterdir()))
print('canonical TMP path, exclusive empty creation, collision sentinel, returned number/path, reopen/write/delete, buffer tails, errors/semantic FP; bounded ASCII prefix empty/short/three/long/first3chars, leaf/span denials PASS %.6fs'%(time.monotonic()-start))
