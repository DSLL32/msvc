import hashlib,json,os,struct,subprocess,sys,tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from mktemp import MktempObservation
v=json.loads((Path(__file__).parent/'mktemp-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'mktemp-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    for n in range(4):
        with MktempObservation('.\\XXXXXX','a12345',(111 if n&1 else 0,77,123 if n&2 else 0)) as obs:
            p=subprocess.Popen([binary,str(n)],cwd=tmp,env=dict(os.environ,**obs.environment()),pass_fds=(obs.fd,),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            out,err=p.communicate();assert p.returncode==0 and not err and out==bytes.fromhex(v['stdout_hex'][n]),(out,err,p.returncode)
            row=obs.read(p.pid);assert row['after']=='.\\a12345'
            raw=os.pread(obs.fd,obs.size,0)
            for offset,value in ((0,0),(8,p.pid+1),(12,0),(12,2),(20,row['result']+1),(28,1),(36,0),(44,123)):
                changed=bytearray(raw);struct.pack_into('<I',changed,offset,value);os.pwrite(obs.fd,changed,0)
                try:obs.read(p.pid)
                except ValueError:pass
                else:raise AssertionError((offset,value,'accepted mutated receipt'))
            os.pwrite(obs.fd,raw,0)
    for mode in ('template','prestate','collision','second','suffix','size','child'):
        with MktempObservation('.\\XXXXXX','a12345',(0,77,0)) as obs:
            env=dict(os.environ,**obs.environment());args=[binary,'0']
            if mode=='template':env['RECON_MKTEMP_TEMPLATE']='otherXXXXXX'
            if mode=='prestate':env['RECON_MKTEMP_PRESTATE']='1,77,0'
            if mode=='collision':(Path(tmp)/'a12345').write_bytes(b'collision')
            if mode in ('second','child'):args+=[mode]
            if mode=='suffix':env['RECON_MKTEMP_SUFFIX']='abcdefg'
            if mode=='size':os.ftruncate(obs.fd,1)
            p=subprocess.run(args,cwd=tmp,env=env,pass_fds=(obs.fd,),capture_output=True)
            assert p.returncode==125,(mode,p.returncode,p.stderr)
            if mode=='collision':(Path(tmp)/'a12345').unlink()
    with MktempObservation('.\\XXXXXX','a12345',(0,77,0)) as obs:
        try:obs.read(os.getpid())
        except ValueError:pass
        else:raise AssertionError('missing invocation accepted')
print('canonical mktemp alias/template/errors; strict invocation/receipt/collision controls pass')
