import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'searchenv-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'searchenv-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    root=Path(tmp);work=root/'work';work.mkdir();drive=root/'drive';library=drive/'toolchains/5.10/lib';library.mkdir(parents=True)
    source=library/'kernel32.lib';source.write_bytes(b'owned library input')
    env=dict(os.environ,RECON_DRIVE_R=str(drive),LIB=v['actual_CRT_LIB'])
    p=subprocess.run([binary],cwd=work,env=env,capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stdout,p.stderr)
    for name,digest in v['buffers'].items():assert hashlib.sha256((work/name).read_bytes()).hexdigest()==digest,name
    assert sorted(x.name for x in work.iterdir())==sorted(v['buffers'])
    assert source.read_bytes()==b'owned library input'
    alternate=drive/'alternate';alternate.mkdir();(alternate/'different.lib').write_bytes(b'changed input')
    changed=dict(env,OTHERDIR='R:\\alternate')
    p=subprocess.run([binary,'different.lib','OTHERDIR'],cwd=work,env=changed,capture_output=True)
    assert p.returncode==0 and p.stdout==b'R:\\alternate\\different.lib\r\n' and not p.stderr,(p.returncode,p.stdout,p.stderr)
    assert (alternate/'different.lib').read_bytes()==b'changed input'
    for mode in ('overlap','locale','name','list','quote','relative','missing','missing-variable','length','nonascii','current'):
        changed=env.copy()
        if mode=='list':changed['LIB']+=';R:\\other'
        if mode=='quote':changed['LIB']='"'+changed['LIB']+'"'
        if mode=='relative':changed['LIB']='relative'
        if mode=='missing':changed['LIB']='R:\\missing'
        if mode=='missing-variable':changed.pop('LIB')
        if mode=='length':changed['LIB']='R:\\'+'x'*255
        if mode=='nonascii':changed['LIB']='R:\\café'
        if mode=='current':(work/'kernel32.lib').write_bytes(b'current-directory')
        p=subprocess.run([binary,mode],cwd=work,env=changed,capture_output=True)
        assert p.returncode==125 and b'unsupported' in p.stderr,(mode,p.returncode,p.stderr)
        if mode=='current':(work/'kernel32.lib').unlink()
print('4 canonical searchenv error triples/full260-byte output tails; changed file/variable and11 unknown-scope denials pass')
