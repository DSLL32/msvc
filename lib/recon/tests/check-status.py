import importlib.util,os,subprocess,sys
from pathlib import Path
spec=importlib.util.spec_from_file_location('status',Path(__file__).resolve().parents[1]/'status.py')
status=importlib.util.module_from_spec(spec);spec.loader.exec_module(status)
binary=str(Path(sys.argv[1]).resolve())
for mode in ('return','exit','win32','fork','exec'):
    for code in (0,1100,0xffffffff):
        with status.TerminalStatus() as receipt:
            p=subprocess.Popen([binary,mode,str(code)],env=dict(os.environ,**receipt.environment()),pass_fds=(receipt.fd,),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            out,err=p.communicate(timeout=10)
            assert not out and not err,(mode,out,err)
            assert receipt.read(p.pid,p.returncode)==code
for mode in ('missing','duplicate'):
    with status.TerminalStatus() as receipt:
        p=subprocess.Popen([binary,mode,'1100'],env=dict(os.environ,**receipt.environment()),pass_fds=(receipt.fd,),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        p.communicate(timeout=10)
        try: receipt.read(p.pid,p.returncode)
        except ValueError: pass
        else: raise AssertionError(mode)
for value in ('','bad','999999'):
    p=subprocess.run([binary,'return','0'],env=dict(os.environ,RECON_STATUS_FD=value),capture_output=True)
    assert p.returncode==125
assert subprocess.run([binary,'return','1100']).returncode==76
print('full DWORD status, unset mode, fork/exec ownership and 5 rejection controls passed')
