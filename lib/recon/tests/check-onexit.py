import importlib.util,subprocess,sys,tempfile,os,json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('status',root/'status.py')
status=importlib.util.module_from_spec(spec);spec.loader.exec_module(status)
binary=str(Path(sys.argv[1]).resolve())
original=json.loads((root/'tests/onexit-original.json').read_text())
assert original['source_sha256']==hashlib.sha256((root/'tests/onexit-probe.c').read_bytes()).hexdigest()
rows={r['mode']:r for r in original['runs']}
with tempfile.TemporaryDirectory(prefix='onexit-',dir='/root/.ai-tmp') as directory:
    for mode,code in [('exit',1100),('return',37),('process',1100)]:
        with status.TerminalStatus() as receipt:
            env=dict(os.environ,**receipt.environment()) if mode!='return' else os.environ.copy()
            p=subprocess.Popen([binary,mode],cwd=directory,env=env,pass_fds=(receipt.fd,),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            out,err=p.communicate(timeout=10)
            assert p.returncode==code&255
            if mode!='return': assert receipt.read(p.pid,p.returncode)==code
        artifact=(Path(directory)/'exit-output.bin').read_bytes()
        expected=b'C pending=6 size=0\r\nB pending=12 size=0\r\nA pending=18 size=0\r\n'
        assert not out and err==(expected if mode!='process' else b'')
        assert artifact==(b'beforeafterCafterBafterA' if mode!='process' else b'')
        assert err==bytes.fromhex(rows[mode]['stderr'])
        assert artifact==bytes.fromhex(rows[mode]['artifact'])
    for mode in ('null','dll'):
        p=subprocess.run([binary,mode],cwd=directory,capture_output=True)
        assert p.returncode==125
print('onexit callback identity/LIFO/interleaved atexit/preflush state/status and 2 controls passed')
