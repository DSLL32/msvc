#!/usr/bin/env python3
"""Independently reproduce frozen LINK fixtures with the untouched toolchain."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
sys.dont_write_bytecode = True

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
TC = ROOT / 'toolchains/1.0-nt'


def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, default=HERE)
    parser.add_argument('--prefix', type=Path, default=Path.home() / '.ai-tmp/vc1.0-link/wine')
    parser.add_argument('--scratch', type=Path, default=Path.home() / '.ai-tmp/vc1.0-link/fixture-verify')
    parser.add_argument('--phase', default='all')
    args = parser.parse_args()
    fixture = args.fixture.resolve()
    manifest = json.loads((fixture / 'manifest.json').read_text())
    spec = importlib.util.spec_from_file_location('fixture_artifacts', fixture / 'artifacts.py')
    artifacts = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(artifacts)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(args.prefix), WINEARCH='win32', WINEDEBUG='-all',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'), WINEPATH=windows(TC / 'bin'))
    receipt = {}
    for name, phase in manifest['phases'].items():
        if args.phase not in ('all', name):
            continue
        work = args.scratch / fixture.name / name
        if work.exists():
            shutil.rmtree(work)
        work.mkdir(parents=True)
        for entry in phase['inputs']:
            data = (fixture / entry['path']).read_bytes()
            assert len(data) == entry['bytes'] and hashlib.sha256(data).hexdigest() == entry['sha256']
            (work / Path(entry['path']).name).write_bytes(data)
        target = ROOT / phase['target']
        assert hashlib.sha256(target.read_bytes()).hexdigest() == phase['sha256']
        phase_env = env.copy()
        for key in phase['unset_environment']:
            phase_env.pop(key, None)
        phase_env.update({key: value.format(lib=windows(TC / 'lib')) for key, value in phase['environment'].items()})
        result = subprocess.run(['wine', str(target), *phase['arguments']], cwd=work, env=phase_env,
                                stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
        (work / 'stdout').write_bytes(result.stdout)
        (work / 'stderr').write_bytes(result.stderr)
        assert result.returncode == phase['expected_exit'], (name, result.returncode)
        for stream in ('stdout', 'stderr'):
            actual = getattr(result, stream)
            assert actual == phase['expected_' + stream].encode('latin1'), (name, stream)
        outputs = []
        for entry in phase['outputs']:
            data = (work / entry['output_name']).read_bytes()
            expected = (fixture / entry['path']).read_bytes()
            assert len(data) == entry['bytes']
            kind = entry['artifact_kind']
            assert artifacts.timestamp_fields(data, kind) == entry['ignored_fields']
            normalized = artifacts.normalize_artifact(data, kind)
            assert normalized == artifacts.normalize_artifact(expected, kind), (name, entry['output_name'])
            assert hashlib.sha256(normalized).hexdigest() == entry['normalized_sha256']
            outputs.append({'name': entry['output_name'], 'bytes': len(data), 'normalized_sha256': entry['normalized_sha256']})
        receipt[name] = {'exit': result.returncode, 'streams_exact': True, 'outputs': outputs}
        print(name + ': exact streams and normalized artifacts match', flush=True)
    (args.scratch / (fixture.name + '-receipt.json')).write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    main()
