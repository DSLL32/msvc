import os,subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
expected=(Path(__file__).parent/'editor-api-original.stdout').read_bytes()
with tempfile.TemporaryDirectory(prefix='editor-api-',dir='/root/.ai-tmp') as d:
    namespace=Path(d);work=namespace/'recon-maintenance/editor-api-probe';work.mkdir(parents=True)
    env=dict(os.environ,RECON_DRIVE_S=str(namespace),RECON_CANONICAL_CWD=r'S:\recon-maintenance\editor-api-probe')
    p=subprocess.run([binary],cwd=work,env=env,capture_output=True,check=True)
    assert not p.stderr and p.stdout==expected,(p.stdout,expected,p.stderr)
    p=subprocess.run([binary,'unsupported'],cwd=work,env=env,capture_output=True)
    assert p.returncode==125
    for canonical in (None,r'S:\missing'):
        changed=dict(env)
        if canonical is None: changed.pop('RECON_CANONICAL_CWD')
        else: changed['RECON_CANONICAL_CWD']=canonical
        p=subprocess.run([binary],cwd=work,env=changed,capture_output=True)
        assert p.returncode==125
print('NTDLL HIGHLOW/ABSOLUTE/zero count and fullpath exact original bytes, unsupportedtype and canonicalcwd controls passed')
