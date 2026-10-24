"""One explicit _getcwd entry/return capture; never a native fixture."""
import argparse
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from binary import PE
from case_paths import paths, CAPTURE_ROOT, canonical_parts, SCRATCH, native_work
from runtime_boundary_loader import load_boundary
BINARY = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original_capture', type=Path)
    parser.add_argument('--reuse-original', type=Path)
    args = parser.parse_args()
    boundary = load_boundary(); sha = boundary.sha
    spec = importlib.util.spec_from_file_location('cwd_tool', BINARY/'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(spec); spec.loader.exec_module(tool)
    config = json.loads((BINARY/'functions.json').read_text())
    before = json.loads((args.original_capture/'receipt.json').read_text())
    assert before['candidate'] == '_browser_working_directory' and before['terminal'] == '__getcwd'
    assert before['configuration_sha256'] == sha(BINARY/'functions.json')
    identity = boundary.identities(canonical=True, names=['__getcwd'])
    image = PE(tool.TC/'bin/C1XX3232.EXE')
    directory = Path(tempfile.mkdtemp(prefix='cwd-boundary-', dir=CAPTURE_ROOT))
    plan = directory/'restore-plan.txt'
    row = identity['records'][0]['record']
    plan.write_text('%x %x %x __getcwd\n' % (row['start'], image.offset(row['start']), row['end']-row['start']))
    _, fixture, _, _ = paths(config, 'member_registry_browser')
    phase = json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    canonical = phase['canonical_working_directory']
    flags = phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC/'bin'), include=tool.windows(tool.TC/'include'))
    program = phase['command_line_template'].format(bin=tool.windows(tool.TC/'bin'))
    if args.reuse_original:
        for filename in ('original-observation.txt', 'original-events.txt', 'original-stdout.bin', 'original-stderr.bin'):
            shutil.copy2(args.reuse_original/filename, directory/filename)
        shutil.copytree(args.reuse_original/'original-case', directory/'original-case')
        original_seconds = 0.0
    else:
        with (CAPTURE_ROOT/'capture.lock').open('a') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            work = SCRATCH.joinpath(*canonical_parts(canonical)); work.mkdir(parents=True, exist_ok=True)
            for item in work.iterdir():
                assert item.is_file() and not item.is_symlink()
                item.unlink()
            for item in phase['inputs']: shutil.copyfile(fixture/item['path'], work/Path(item['path']).name)
            command = ['wine', tool.windows(BINARY/'build/reference-cwd-observer.exe'), tool.windows(args.original_capture/'masked-original.exe'), '', tool.windows(directory/'original-stdout.bin'), tool.windows(directory/'original-stderr.bin'), tool.windows(directory/'original-events.txt'), canonical, 'MSC_CMD_FLAGS='+flags, program, '', '', tool.windows(args.original_capture/'entries.plan')]
            env = tool.environment(); env.update(C1XX_CWD_PLAN=tool.windows(plan), C1XX_CWD_LOG=tool.windows(directory/'original-observation.txt'), C1XX_CWD_IMAGE=tool.windows(tool.TC/'bin/C1XX3232.EXE'))
            started = time.perf_counter()
            subprocess.run(command, env=env, check=True, timeout=60, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            original_seconds = time.perf_counter()-started
            shutil.copytree(work, directory/'original-case')
    print(directory, flush=True)
    raw = (directory/'original-observation.txt').read_text()
    original = {k:int(re.search(r'^'+k+r' ([0-9a-f]+)$',raw,re.M)[1],16) for k in ('return_va','buffer')}
    original.update({k:int(re.search(r'^'+k+r' (\d+)$',raw,re.M)[1]) for k in ('capacity','returned_buffer')})
    for stage in ('before','after'):
        match = re.search(r'^'+stage+r' errno (\d+) doserrno (\d+) last_error (\d+) control ([0-9a-f]+) status ([0-9a-f]+) tag ([0-9a-f]+) next ([0-9a-f]+) remaining (-?\d+) bytes ([0-9a-f]{512})$',raw,re.M)
        assert match, 'missing exact readable original buffer/error/FP receipt'
        v = match.groups(); original[stage] = dict(errno=int(v[0]),doserrno=int(v[1]),last_error=int(v[2]),control=int(v[3],16),status=int(v[4],16),tag=int(v[5],16),next=int(v[6],16),remaining=int(v[7]),bytes=v[8])
    work, drive_env = native_work(directory/'native-drive', canonical)
    for item in phase['inputs']: shutil.copyfile(fixture/item['path'],work/Path(item['path']).name)
    commands = ['break recon_getcwd', 'run > '+str(directory/'native-stdout.bin')+' 2> '+str(directory/'native-stderr.bin'), 'python', 'import gdb,json',
        'memory=gdb.selected_inferior()',
        'buffer=int(gdb.parse_and_eval("out")); capacity=int(gdb.parse_and_eval("capacity"))',
        'caller=gdb.newest_frame().older().name()',
        'def state():',
        '    return dict(errno=int(gdb.parse_and_eval("(int)errno")),doserrno=int(gdb.parse_and_eval("recon_doserrno")),last_error=int(gdb.parse_and_eval("\'win32.c\'::last_error")),control=int(gdb.parse_and_eval("$fctrl"))&65535,status=int(gdb.parse_and_eval("$fstat"))&65535,tag=int(gdb.parse_and_eval("$ftag"))&65535,next=int(gdb.parse_and_eval("compiler_arena.pools[2].next")),remaining=int(gdb.parse_and_eval("compiler_arena.pools[2].remaining")),bytes=bytes(memory.read_memory(buffer,256)).hex())',
        'before=state()', 'end', 'finish', 'python',
        'observed=dict(buffer=buffer,capacity=capacity,caller=caller,returned_buffer=int(int(gdb.parse_and_eval("(unsigned)$eax"))==buffer),before=before,after=state())',
        'open('+repr(str(directory/'native-observation.json'))+',"w").write(json.dumps(observed,indent=2)+"\\n")','end']
    script = directory/'native.gdb'; script.write_text('\n'.join(commands)+'\n')
    env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL','_CL_','LINK','_LINK_'): env.pop(name,None)
    env.update(MSC_CMD_FLAGS=flags,RECON_PROGRAM_PATH=program,TZ='PST8PDT'); env.update(drive_env)
    started = time.perf_counter()
    run = subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/C1XX3232.EXE')],cwd=work,env=env,text=True,capture_output=True,timeout=60)
    native_seconds = time.perf_counter()-started
    (directory/'native-gdb.log').write_text(run.stdout+run.stderr)
    assert run.returncode == 0, run.stdout+run.stderr
    native = json.loads((directory/'native-observation.json').read_text())
    assert original['return_va'] == 0x69ae8 and native['caller'] == 'browser_working_directory'
    assert original['capacity'] == native['capacity'] == 256
    assert original['returned_buffer'] == native['returned_buffer'] == 1
    expected = canonical.encode('latin1')+b'\0'
    for side in (original,native):
        before = side['before']; after = side['after']
        assert before['next'] == after['next'] == side['buffer']+256
        assert before['remaining'] == after['remaining']
        for key in ('errno','doserrno','last_error','control','status','tag'): assert before[key] == after[key], (key,side)
        assert bytes.fromhex(after['bytes'])[:len(expected)] == expected
        assert bytes.fromhex(before['bytes'])[len(expected):] == bytes.fromhex(after['bytes'])[len(expected):]
    assert original['after']['bytes'] == native['after']['bytes']
    for key in ('control','status','tag'): assert original['before'][key] == native['before'][key]
    assert {p.name:p.read_bytes() for p in (directory/'original-case').iterdir() if p.is_file()} == {p.name:p.read_bytes() for p in work.iterdir() if p.is_file()}
    for stream in ('stdout.bin','stderr.bin'): assert (directory/('original-'+stream)).read_bytes() == (directory/('native-'+stream)).read_bytes()
    receipt = dict(kind='getcwd',case='member_registry_browser',target_sha256=config['target']['sha256'],configuration_sha256=sha(BINARY/'functions.json'),accepted_before=len(config['accepted']),runtime_order=['__getcwd'],identity=identity,original=original,native=native,canonical_working_directory=canonical,original_capture=str(args.original_capture),original_capture_sha256=sha(args.original_capture/'receipt.json'),original_seconds=original_seconds,native_seconds=native_seconds,scope='Actual 256B caller pool2 allocation, exact cwd+NUL, returned buffer owner, untouched defined tail, per-process errors and x87 preservation; no CRT source')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')
if __name__ == '__main__': main()
