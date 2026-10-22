#!/usr/bin/env python3
"""Capture or verify local follow-up case oracles with the untouched RC.EXE."""
import argparse
import hashlib
import importlib.machinery
import json
from pathlib import Path
import shutil
import subprocess
wt = importlib.machinery.SourceFileLoader('wine_tool', 'wine-tool.py').load_module()
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('cases', nargs='*')
parser.add_argument('--record', action='store_true', help='Record deliberately refreshed oracle bytes')
args = parser.parse_args()
cfg = json.loads(Path('functions.json').read_text())
original = wt.TC / 'bin/RC.EXE'
assert hashlib.sha256(original.read_bytes()).hexdigest() == cfg['target']['sha256']
for name in args.cases or [n for n in cfg['cases'] if n != 'resources-rich']:
    settings = cfg['cases'][name]
    fixture = Path('cases') / name
    work = Path.home() / '.ai-tmp/vc1.0-rc' / ('capture-' + name)
    if work.exists():
        shutil.rmtree(work)
    if (fixture / 'source').exists():
        shutil.copytree(fixture / 'source', work)
    else:
        work.mkdir(parents=True)
    source = {str(p.relative_to(work)):p.read_bytes() for p in work.rglob('*') if p.is_file()}
    stem = Path('build') / ('capture-' + name)
    env = wt.environment()
    env.update(TEMP=wt.windows(work), TMP=wt.windows(work))
    subprocess.run(['wine', 'build/debug-run.exe', wt.windows(original), settings['arguments'],
                    wt.windows(str(stem) + '.stdout'), wt.windows(str(stem) + '.stderr'),
                    wt.windows(str(stem) + '.events'), wt.windows(work)], env=env,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, check=True, timeout=90)
    assert Path(str(stem) + '.events').read_text() == 'exit %d\n' % settings.get('exit', 0)
    observed = {str(p.relative_to(work)):p.read_bytes() for p in work.rglob('*') if p.is_file()}
    assert all(observed.get(n) == data for n, data in source.items()), 'original changed fixture input'
    oracle = {n:data for n, data in observed.items() if n not in source}
    for stream in ['stdout', 'stderr']:
        oracle[stream + '.bin'] = Path(str(stem) + '.' + stream).read_bytes()
    manifest = dict(target_sha256=cfg['target']['sha256'], arguments=settings['arguments'],
                    exit=settings.get('exit', 0),
                    inputs={n:dict(bytes=len(d), sha256=hashlib.sha256(d).hexdigest()) for n,d in sorted(source.items())},
                    oracle={n:dict(bytes=len(d), sha256=hashlib.sha256(d).hexdigest()) for n,d in sorted(oracle.items())})
    if args.record:
        (fixture / 'oracle').mkdir(parents=True, exist_ok=True)
        for file, data in oracle.items():
            (fixture / 'oracle' / file).write_bytes(data)
        (fixture / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    else:
        expected = {p.name:p.read_bytes() for p in (fixture / 'oracle').iterdir() if p.is_file()}
        assert expected == oracle, (name, 'captured oracle differs')
        assert json.loads((fixture / 'manifest.json').read_text()) == manifest
    print('%s: original exit %d; %d stdout bytes, %d stderr bytes; %d products' %
          (name, manifest['exit'], len(oracle['stdout.bin']), len(oracle['stderr.bin']), len(oracle)-2), flush=True)
