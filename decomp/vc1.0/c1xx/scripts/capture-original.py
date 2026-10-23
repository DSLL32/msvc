"""Explicit masked-original C1XX capture, never called by native fixtures.

Produces new scratch evidence only. It does not update cached references and
never executes the reconstructed Win32 binary.
"""
import argparse
import hashlib
import fcntl
import re
import shutil
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time
from case_paths import paths, CAPTURE_ROOT, canonical_parts, SCRATCH
from binary import PE
from win32_indexes import imports, mask, logical

BINARY = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',default='cpp_basic')
    parser.add_argument('--accepted-count', type=int)
    parser.add_argument('--entry-plan', action='store_true', help='capture and assert the accepted application first-entry sequence')
    parser.add_argument('--candidate', help='explicit proposed function to unmask for fresh acceptance evidence')
    parser.add_argument('--pending-accepted', type=Path, help='explicit proposed prefix additions for a runtime boundary chain')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('c1xx_original_tool', BINARY / 'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)
    configuration = (BINARY / 'functions.json').read_bytes()
    config = json.loads(configuration)
    count = len(config['accepted']) if args.accepted_count is None else args.accepted_count
    assert 0 <= count <= len(config['accepted'])
    image = PE(tool.TC / 'bin/C1XX3232.EXE')
    assert hashlib.sha256(image.data).hexdigest() == config['target']['sha256']
    drivers = {r['start'] for r in config['driver']}
    records = [r for r in json.loads((BINARY / 'runtime.json').read_text()) if r['start'] not in drivers]
    records += imports(image) + config['driver']
    accepted = config['accepted'][:count]
    pending = json.loads(args.pending_accepted.read_text()) if args.pending_accepted else []
    assert not pending or count == len(config['accepted'])
    assert all(name not in accepted for name in pending) and len(set(pending)) == len(pending)
    assert all(any(r['name'] == name for r in records) for name in pending)
    accepted += pending
    count = len(accepted)
    if args.candidate:
        assert args.candidate not in accepted, 'candidate already accepted'
        assert any(r['name'] == args.candidate for r in records), 'unknown candidate'
        accepted = accepted + [args.candidate]
    CAPTURE_ROOT.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='original-capture-', dir=CAPTURE_ROOT))
    (directory / 'configuration.json').write_bytes(configuration)
    _,fixture,_,_=paths(config,args.case)
    phase = json.loads((fixture / 'manifest.json').read_text())['phases']['c1']
    canonical = phase['canonical_working_directory']
    work = SCRATCH.joinpath(*canonical_parts(canonical))
    lock = (CAPTURE_ROOT/'capture.lock').open('a')
    fcntl.flock(lock, fcntl.LOCK_EX)
    work.mkdir(parents=True, exist_ok=True)
    for existing in work.iterdir():
        assert existing.is_file() and not existing.is_symlink(), str(existing)
        existing.unlink()
    for item in phase['inputs']:
        data = (fixture / item['path']).read_bytes()
        assert len(data) == item['bytes'] and hashlib.sha256(data).hexdigest() == item['sha256']
        (work / Path(item['path']).name).write_bytes(data)
    executable = directory / 'masked-original.exe'
    mask(image, records, accepted, executable)
    plan_names=[]
    if args.entry_plan:
        sources={}
        for line in (BINARY/'cases/canonical-map.txt').read_text().splitlines():
            match=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+[0-9a-f]{8}\s+f\s+(\S+)',line)
            if match and ':' not in match[2]:sources[match[1]]=Path(match[2]).stem
        first=config['cases'][args.case].get('accepted_from',0)
        plan_names=[name for name in accepted[first:] if name in sources and sources[name]!='runtime']
        by_name={r['name']:r for r in records}
        data=bytearray(executable.read_bytes())
        rows=[]
        for name in plan_names:
            record=by_name[name]; offset=image.offset(record['start'])
            rows.append('%x %02x %s\n'%(record['start']-image.base,image.data[offset],name))
            data[offset]=0xcc
        executable.write_bytes(data)
        (directory/'entries.plan').write_text(''.join(rows))
    flags = phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC / 'bin'), include=tool.windows(tool.TC / 'include'))
    command = ['wine', tool.windows(BINARY / 'build/debug-run.exe'), tool.windows(executable), '',
               tool.windows(directory / 'stdout.bin'), tool.windows(directory / 'stderr.bin'),
               tool.windows(directory / 'events.txt'), canonical, 'MSC_CMD_FLAGS=' + flags,
               phase['command_line_template'].format(bin=tool.windows(tool.TC / 'bin')),
               ','.join('%x' % (s['original'] - image.base) for s in config['buffered_streams']),
               ','.join('%x:%s' % (s['original'] - image.base, 'p' if s.get('pointer') else 'v') for s in config['checkpoint_state'])]
    if args.entry_plan: command.append(tool.windows(directory/'entries.plan'))
    started = time.perf_counter()
    subprocess.run(command, env=tool.environment(), check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    capture_seconds=time.perf_counter()-started
    shutil.copytree(work,directory/'case')
    fcntl.flock(lock,fcntl.LOCK_UN); lock.close()
    receipt = dict(case=args.case,scope='explicit original-only reference diagnostic; no cache update',
        accepted_count=count, terminal=logical((directory / 'events.txt').read_text(), image, records),
        candidate=args.candidate, pending_accepted=pending, configuration_sha256=hashlib.sha256(configuration).hexdigest(),
        capture_seconds=capture_seconds, original_sha256=config['target']['sha256'],
        outputs={p.name: dict(bytes=p.stat().st_size, sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in work.iterdir() if p.is_file()})
    receipt['canonical_working_directory']=canonical
    if args.entry_plan:
        observed=re.findall(r'^entry (\S+) rva [0-9a-f]+ byte [0-9a-f]+$',(directory/'events.txt').read_text(),re.M)
        assert observed==plan_names,(observed,plan_names)
        receipt['first_entry_projection']=dict(accepted_first=first,accepted_last=len(accepted),
            accepted_prefix_sha256=hashlib.sha256(json.dumps(accepted,separators=(',',':')).encode()).hexdigest(),
            expected=plan_names,observed=observed,
            events_sha256=hashlib.sha256((directory/'events.txt').read_bytes()).hexdigest(),
            plan_sha256=hashlib.sha256((directory/'entries.plan').read_bytes()).hexdigest())
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(directory / 'receipt.json')


if __name__ == '__main__':
    main()
