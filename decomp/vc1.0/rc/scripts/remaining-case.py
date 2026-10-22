"""Isolated launch and precise verbose-PID validation for the frozen cases."""
import importlib.machinery
import hashlib
import tempfile
import json
from pathlib import Path
import re
import shutil
import subprocess
from binary import PE,imports

fixture=importlib.machinery.SourceFileLoader('remaining_capture',
    str(Path('../fixtures/rc-remaining/capture.py').resolve())).load_module()
state=None


def prepare():
    global state
    state=fixture.prepare()


def run(label,executable,arguments,work,stem,path_fallback,names):
    assert state is not None
    env,_,controlled_path=state;env=dict(env)
    wt=fixture.wt
    pe=PE(executable)
    if label=='original':
        input_rva,output_rva=0x1b024,0x1b028
    else:
        input_rva,output_rva=names['_temporary_input']-pe.base,names['_temporary_output']-pe.base
    registration=next(r['start']-pe.base for r in imports(pe) if r['name']=='import:SetConsoleCtrlHandler')
    if path_fallback:
        directory=fixture.SCRATCH/('paired-launch-'+label)
        directory.mkdir(exist_ok=True)
        assert not (directory/'rcpp.exe').exists()
        target=directory/'rc.exe';shutil.copyfile(executable,target)
        assert not (work/'rcpp.exe').exists()
    else:target=Path(executable).resolve()
    env.update(TEMP=wt.windows(work),TMP=wt.windows(work))
    subprocess.run(['/usr/bin/wine','build/debug-run.exe',wt.windows(target),arguments,
                    wt.windows(str(stem)+'.stdout'),wt.windows(str(stem)+'.stderr'),
                    wt.windows(str(stem)+'.events'),wt.windows(work),controlled_path if path_fallback else '',
                    '%x'%registration,'%x'%input_rva,'%x'%output_rva],env=env,
                   stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True,timeout=90)
    archive(label,stem,work,arguments,target)
    return Path(str(stem)+'.events').read_text()


def verify_verbose(label, directory=Path('build')):
    stem=directory/label
    proof=Path(str(stem)+'.events.process').read_text()
    pid=int(re.fullmatch(r'pid (\d+) tid \d+\nnative_temporary_input RC\d{6}\nnative_temporary_output RD\d{6}\n',proof)[1])
    digits=('%06d'%pid)[-6:]
    assert 'native_temporary_input RC'+digits+'\n' in proof
    assert 'native_temporary_output RD'+digits+'\n' in proof
    raw=Path(str(stem)+'.stdout').read_bytes()
    assert raw==fixture.verbose_bytes(pid), (label,'full verbose stream differs beyond validated native PID fields')
    return dict(pid=pid,bytes=len(raw),fields=[dict(offset=o,length=6,native_digits=digits) for o in (161,173)])


def retain_verbose(label):
    proof=verify_verbose(label)
    destination=Path('build/verbose-native')
    destination.mkdir(exist_ok=True)
    for suffix in ('stdout','stderr','events','events.process'):
        shutil.copyfile(Path('build')/(label+'.'+suffix),destination/(label+'.'+suffix))
    (destination/(label+'.verified.json')).write_text(json.dumps(proof,indent=2)+'\n')


def archive(label,stem,work,arguments,target):
    """Retain every raw run separately; do not overwrite historical PID evidence."""
    root=fixture.SCRATCH/'raw-runs'
    root.mkdir(exist_ok=True)
    metadata=Path(str(stem)+'.events.process').read_text()
    pid=int(metadata.split()[1])
    destination=Path(tempfile.mkdtemp(prefix=label+'-pid'+str(pid)+'-',dir=root))
    files={}
    for suffix in ('stdout','stderr','events','events.process'):
        source=Path(str(stem)+'.'+suffix)
        shutil.copyfile(source,destination/suffix)
        data=source.read_bytes()
        files[suffix]=dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
    shutil.copytree(work,destination/'case-files')
    data=Path(target).read_bytes()
    (destination/'run.json').write_text(json.dumps(dict(label=label,pid=pid,arguments=arguments,
        target=str(target),target_sha256=hashlib.sha256(data).hexdigest(),raw=files),indent=2)+'\n')
