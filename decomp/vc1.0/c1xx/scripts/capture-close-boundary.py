"""Explicit original/native close call observation; outside cached fixtures."""
import argparse
import fcntl
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from case_paths import paths, CAPTURE_ROOT, canonical_parts, SCRATCH, native_work
from binary import PE
from win32_indexes import imports

BINARY = Path(__file__).resolve().parents[1]

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original_capture', type=Path)
    args = parser.parse_args()
    specification = importlib.util.spec_from_file_location('c1xx_tool', BINARY/'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(tool)
    config = json.loads((BINARY/'functions.json').read_text())
    before = json.loads((args.original_capture/'receipt.json').read_text())
    assert before['candidate'] == '_close_source_file' and before['terminal'] == '__close'
    assert before['configuration_sha256'] == sha(BINARY/'functions.json')
    original = PE(tool.TC/'bin/C1XX3232.EXE')
    assert sha(tool.TC/'bin/C1XX3232.EXE') == config['target']['sha256']
    records = json.loads((BINARY/'runtime.json').read_text()) + imports(original)
    names = ('__close', 'import:CloseHandle', '__free_osfhnd')
    selected = [next(r for r in records if r['name'] == name) for name in names]
    directory = Path(tempfile.mkdtemp(prefix='close-boundary-', dir=CAPTURE_ROOT))
    plan = directory/'restore-plan.txt'
    plan.write_text(''.join('%x %x %x %s\n' % (r['start'], original.offset(r['start']), r['end']-r['start'], r['name']) for r in selected))
    _,fixture,_,_=paths(config,'cpp_basic')
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    lock=(CAPTURE_ROOT/'capture.lock').open('a'); fcntl.flock(lock,fcntl.LOCK_EX)
    canonical=phase['canonical_working_directory']
    original_work=SCRATCH.joinpath(*canonical_parts(canonical))
    original_work.mkdir(parents=True,exist_ok=True)
    for entry in original_work.iterdir():
        assert entry.is_file() and not entry.is_symlink(),str(entry)
        entry.unlink()

    flags = phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC/'bin'),include=tool.windows(tool.TC/'include'))
    program = phase['command_line_template'].format(bin=tool.windows(tool.TC/'bin'))
    work = original_work
    for item in phase['inputs']:shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
    command = ['wine',tool.windows(BINARY/'build/reference-close-observer.exe'),tool.windows(args.original_capture/'masked-original.exe'),'',
        tool.windows(directory/'original-stdout.bin'),tool.windows(directory/'original-stderr.bin'),tool.windows(directory/'original-events.txt'),
        canonical,'MSC_CMD_FLAGS='+flags,program]
    env = tool.environment()
    env.update(C1XX_CLOSE_PLAN=tool.windows(plan),C1XX_CLOSE_LOG=tool.windows(directory/'original-observation.txt'),C1XX_CLOSE_IMAGE=tool.windows(tool.TC/'bin/C1XX3232.EXE'))
    started=time.perf_counter()
    subprocess.run(command,env=env,check=True,timeout=90,stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    original_seconds=time.perf_counter()-started
    shutil.copytree(original_work,directory/'original-case')
    fcntl.flock(lock,fcntl.LOCK_UN);lock.close()
    observations=(directory/'original-observation.txt').read_text()
    observed={key:int(re.search(r'^'+key+r' (\d+)$',observations,re.M)[1]) for key in ('descriptor','open_flag','result','closed_flag')}
    observed['runtime_order']=re.findall(r'^runtime (\S+) argument ',observations,re.M)
    observed['return_va']=int(re.search(r'^return_va ([0-9a-f]+)$',observations,re.M)[1],16)
    assert observed['runtime_order']==list(names),observations
    assert observed['return_va']==0x1e9bc and observed['open_flag']&1 and observed['result']==0 and observed['closed_flag']==0
    work,drive_environment=native_work(directory/'native-drive',canonical)
    for item in phase['inputs']:shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
    commands=['break recon_close','run > '+str(directory/'native-stdout.bin')+' 2> '+str(directory/'native-stderr.bin'),'python','import gdb,json',
        'descriptor=int(gdb.parse_and_eval("fd"))','before=int(gdb.parse_and_eval("recon_osfile[fd]"))','end','finish','python',
        'observed={"descriptor":descriptor,"open_flag":before,"result":int(gdb.parse_and_eval("(int)$eax")),"closed_flag":int(gdb.parse_and_eval("recon_osfile[%d]"%descriptor)),"caller":gdb.newest_frame().name()}',
        'open('+repr(str(directory/'native-observation.json'))+',"w").write(json.dumps(observed,indent=2)+"\\n")','end']
    script=directory/'native.gdb';script.write_text('\n'.join(commands)+'\n')
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
    env.update(MSC_CMD_FLAGS=flags,RECON_PROGRAM_PATH=program,TZ='PST8PDT')
    env.update(drive_environment)
    started=time.perf_counter()
    result=subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/C1XX3232.EXE')],env=env,cwd=work,stdin=subprocess.DEVNULL,capture_output=True,text=True,timeout=60)
    native_seconds=time.perf_counter()-started
    (directory/'native-gdb.log').write_text(result.stdout+result.stderr)
    assert result.returncode==0,result.stdout+result.stderr
    native=json.loads((directory/'native-observation.json').read_text())
    assert native['caller']=='close_source_file'
    assert {key:observed[key] for key in ('descriptor','open_flag','result','closed_flag')}=={key:native[key] for key in ('descriptor','open_flag','result','closed_flag')},(observed,native)
    receipt=dict(scope='explicit source-file-close CRT/import boundary; not an application acceptance',target_sha256=config['target']['sha256'],
        configuration_sha256=sha(BINARY/'functions.json'),accepted_before=len(config['accepted']),runtime_order=list(names),records=selected,
        original=observed,native=native,original_capture=str(args.original_capture),original_capture_sha256=sha(args.original_capture/'receipt.json'),
        observer_source_sha256=sha(BINARY/'src/reference-close-observer.c'),observer_exe_sha256=sha(BINARY/'build/reference-close-observer.exe'),
        native_exe_sha256=sha(BINARY/'build/native/C1XX3232.EXE'),original_seconds=original_seconds,native_seconds=native_seconds)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')

if __name__=='__main__':main()
