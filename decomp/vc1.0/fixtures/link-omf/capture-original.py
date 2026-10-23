#!/usr/bin/env python3
"""Replay frozen LINK fixture using untouched original toolchain only."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
import sys,json,hashlib,subprocess,shutil,fcntl
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"link/scripts"))
from case_paths import SCRATCH
HERE=Path(__file__).resolve().parent
wt=SourceFileLoader('link_wine_tool',str(HERE.parent.parent/'link/scripts/wine-tool.py')).load_module()
a=SourceFileLoader('fixture_artifacts',str(HERE/'artifacts.py')).load_module()
m=json.loads((HERE/'manifest.json').read_text())
root=SCRATCH/'vc1.0/link/shared-capture-tools'/HERE.name
root.mkdir(parents=True,exist_ok=True)
lease=(root/'capture.lock').open('a')
fcntl.flock(lease,fcntl.LOCK_EX)
subprocess.check_call([sys.executable,str(HERE.parent.parent/'link/scripts/wine-tool.py'),'--init'])
reports=[]
for key in (sys.argv[1:] or list(m['phases'])):
    p=m['phases'][key];work=root/key;work.mkdir(parents=True,exist_ok=True)
    env=wt.environment()
    for name in p['unset_environment']:env.pop(name,None)
    for name,value in p['environment'].items():env[name]=value.format(lib=wt.windows(wt.TC/'lib'))
    for entry in p['inputs']:
        data=(HERE/entry['path']).read_bytes()
        assert len(data)==entry['bytes'] and hashlib.sha256(data).hexdigest()==entry['sha256']
        (work/Path(entry['path']).name).write_bytes(data)
    for entry in p['outputs']:(work/entry['output_name']).unlink(missing_ok=True)
    assert hashlib.sha256((wt.TC/'bin/LINK.EXE').read_bytes()).hexdigest()==p['sha256']
    r=subprocess.run(['wine',wt.windows(wt.TC/'bin/LINK.EXE'),*p['arguments']],env=env,cwd=work,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=120)
    assert r.returncode==p['expected_exit'],(key,r.returncode,r.stdout,r.stderr)
    for name,b in [('stdout',r.stdout),('stderr',r.stderr)]:
        (work/(name+'.bin')).write_bytes(b)
        assert b==p['expected_'+name].encode('latin1'),(key,name,b)
    for entry in p['outputs']:
        data=(work/entry['output_name']).read_bytes();expected=(HERE/entry['path']).read_bytes()
        assert len(data)==entry['bytes']
        assert a.timestamp_fields(data,entry['artifact_kind'])==entry['ignored_fields']
        assert a.normalize_artifact(data,entry['artifact_kind'])==a.normalize_artifact(expected,entry['artifact_kind'])
        assert hashlib.sha256(a.normalize_artifact(data,entry['artifact_kind'])).hexdigest()==entry['normalized_sha256']
    if 'generated_program' in p:
        program=p['generated_program'];r=subprocess.run(['wine',wt.windows(work/p['outputs'][0]['output_name'])],env=env,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=20)
        assert (r.returncode,r.stdout,r.stderr)==(program['exit_status'],program['stdout'].encode(),program['stderr'].encode())
    reports.append(dict(phase=key,exit=p['expected_exit'],streams_exact=True,artifacts_exact_except_proved_clocks=True,generated_program_pass='generated_program' in p))
    print(HERE.name,key,'PASS',flush=True)
(root/'validation.json').write_text(json.dumps(reports,indent=2)+'\n')
