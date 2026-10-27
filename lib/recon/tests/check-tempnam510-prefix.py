from pathlib import Path
import os,subprocess,sys,tempfile,time,json
start=time.perf_counter();binary=str(Path(sys.argv[1]).resolve())
base=Path.home()/'.ai-tmp/msvc2/recon-maintenance/tempnam510-prefix';base.mkdir(parents=True,exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('RECON_MKTEMP_') and k!='RECON_TEMPNAM_PROFILE'}
with tempfile.TemporaryDirectory(dir=base) as directory:
    work=Path(directory);(work/'sentinel').write_bytes(b'keep')
    for mode in (None,'long','slash','colon','wild','quote','nonascii','newline','low','wrap'):
        result=subprocess.run([binary]+([mode] if mode else []),cwd=work,env=env,capture_output=True)
        expected=125 if mode else 0
        assert result.returncode==expected and not result.stdout,(mode,result)
        assert bool(result.stderr)==bool(expected),(mode,result)
        assert [p.name for p in work.iterdir()]==['sentinel'] and (work/'sentinel').read_bytes()==b'keep'
print(json.dumps({'status':'pass','seconds':time.perf_counter()-start,'scope':'NULL/empty/ordinary/full247B prefixes, actual owned write/reopen/remove, namespace/errors/semanticFP and9denials'}))
