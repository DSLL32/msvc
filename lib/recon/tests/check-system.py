import os,subprocess,sys,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from spawn import SpawnBroker
binary=str(Path(sys.argv[1]).resolve())
command=r'copy provider.obj S:\vc1.0\link\rom-local-imports\repro\.'
processor=r'C:\windows\system32\cmd.exe'
with tempfile.TemporaryDirectory(prefix='system-',dir='/root/.ai-tmp') as directory:
    work=Path(directory);(work/'provider.obj').write_bytes(b'frozen object')
    def replay(request):
        assert request['mode']==0 and request['file']==processor
        assert request['argv']==[processor,'/c',command]
        assert request['cwd']==directory
        assert (work/'provider.obj').read_bytes()==b'frozen object'
        (work/'copy.obj').write_bytes(b'frozen object')
        return dict(status=0,stdout=b'',stderr=b'')
    for callback,expected in [(replay,0),(lambda r:None,125),(lambda r:7,125),(lambda r:dict(status=-1,errno=2),125)]:
        broker=SpawnBroker(callback)
        try:
            p=subprocess.run([binary,command],cwd=work,env=dict(os.environ,COMSPEC=processor,**broker.environment()),pass_fds=(broker.fileno(),),capture_output=True,timeout=10)
        finally: broker.close()
        broker.check();assert p.returncode==expected,(p.returncode,p.stderr)
        if expected==0: assert not p.stdout and not p.stderr and (work/'copy.obj').read_bytes()==b'frozen object'
    for args,env in [([command],dict(os.environ,COMSPEC=processor)),([],dict(os.environ,COMSPEC=processor)),([command],dict(os.environ,COMSPEC=''))]:
        env.pop('RECON_SPAWN_FD',None)
        p=subprocess.run([binary,*args],cwd=work,env=env,capture_output=True)
        assert p.returncode==125
print('captured system boundary and 6 fail-closed controls passed; no shell execution')
