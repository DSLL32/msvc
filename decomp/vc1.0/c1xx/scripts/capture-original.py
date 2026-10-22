"""Explicit masked-original C1XX capture, never called by native fixtures.

Produces new scratch evidence only. It does not update cached references and
never executes the reconstructed Win32 binary.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time
from binary import PE
from win32_indexes import imports, mask, logical

BINARY = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--accepted-count', type=int)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('c1xx_original_tool', BINARY / 'scripts/wine-tool.py')
    tool = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tool)
    config = json.loads((BINARY / 'functions.json').read_text())
    count = len(config['accepted']) if args.accepted_count is None else args.accepted_count
    assert 0 <= count <= len(config['accepted'])
    image = PE(tool.TC / 'bin/C1XX3232.EXE')
    assert hashlib.sha256(image.data).hexdigest() == config['target']['sha256']
    drivers = {r['start'] for r in config['driver']}
    records = [r for r in json.loads((BINARY / 'runtime.json').read_text()) if r['start'] not in drivers]
    records += imports(image) + config['driver']
    directory = Path(tempfile.mkdtemp(prefix='original-capture-', dir=tool.PREFIX.parent))
    work = directory / 'case'
    work.mkdir()
    fixture = BINARY.parent / 'fixtures/cpp-basic'
    phase = json.loads((fixture / 'manifest.json').read_text())['phases']['c1']
    for item in phase['inputs']:
        data = (fixture / item['path']).read_bytes()
        assert len(data) == item['bytes'] and hashlib.sha256(data).hexdigest() == item['sha256']
        (work / Path(item['path']).name).write_bytes(data)
    executable = directory / 'masked-original.exe'
    mask(image, records, config['accepted'][:count], executable)
    flags = phase['environment']['MSC_CMD_FLAGS'].format(bin=tool.windows(tool.TC / 'bin'), include=tool.windows(tool.TC / 'include'))
    command = ['wine', str(BINARY / 'build/debug-run.exe'), tool.windows(executable), '',
               tool.windows(directory / 'stdout.bin'), tool.windows(directory / 'stderr.bin'),
               tool.windows(directory / 'events.txt'), tool.windows(work), 'MSC_CMD_FLAGS=' + flags,
               phase['command_line_template'].format(bin=tool.windows(tool.TC / 'bin')),
               ','.join('%x' % (s['original'] - image.base) for s in config['buffered_streams']),
               ','.join('%x:%s' % (s['original'] - image.base, 'p' if s.get('pointer') else 'v') for s in config['checkpoint_state'])]
    started = time.perf_counter()
    subprocess.run(command, env=tool.environment(), check=True, timeout=90,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    receipt = dict(scope='explicit original-only reference diagnostic; no cache update',
        accepted_count=count, terminal=logical((directory / 'events.txt').read_text(), image, records),
        capture_seconds=time.perf_counter()-started, original_sha256=config['target']['sha256'],
        outputs={p.name: dict(bytes=p.stat().st_size, sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in work.iterdir() if p.is_file()})
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(directory / 'receipt.json')


if __name__ == '__main__':
    main()
