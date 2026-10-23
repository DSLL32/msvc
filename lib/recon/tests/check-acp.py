import os,subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
for profile in (None,'1252','65001','1252x'):
    env=dict(os.environ);env.pop('RECON_ACP',None)
    if profile is not None:env['RECON_ACP']=profile
    p=subprocess.run([binary],env=env,capture_output=True)
    assert p.returncode==(0 if profile=='1252' else 125),(profile,p.returncode,p.stderr)
print('captured ACP1252/errors exact; missing/unknown profiles denied')
