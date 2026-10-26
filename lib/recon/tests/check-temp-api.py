"""Current-directory temp creation, collisions and scope; native only."""
import os,pathlib,subprocess,sys,tempfile,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
with tempfile.TemporaryDirectory() as temporary:
    root=pathlib.Path(temporary);cwd=root/'case';cwd.mkdir()
    env=dict(os.environ,RECON_DRIVE_S=temporary,RECON_CANONICAL_CWD='S:\\case')
    for mode in ('','live','capacity','null','wrap','env-overlap','unbound','environment','unique','prefix','directory','overlap','file-null','file-wrap'):
        p=subprocess.run([binary]+([mode] if mode else []),cwd=cwd,env=env,capture_output=True)
        expected=0 if admitted and mode in ('','live') else 125
        assert p.returncode==expected and not p.stdout,(mode,p)
        assert b'unsupported native operation:' in p.stderr if expected else not p.stderr,(mode,p)
        assert not list(cwd.iterdir()),(mode,list(cwd.iterdir()))
print('canonical TMP path, exclusive empty creation, collision sentinel, returned number/path, reopen/write/delete, buffer tails, errors/full FP; twelve scope denials PASS %.6fs'%(time.monotonic()-start))
