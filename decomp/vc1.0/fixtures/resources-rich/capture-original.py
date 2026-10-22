#!/usr/bin/env python3
"""Replay RC and its captured RCPP child independently with exact byte checks."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
TC = ROOT / 'toolchains/1.0-nt'
SCRATCH = Path.home() / '.ai-tmp/vc1.0-resource-capture'


def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def environment(prefix):
    env = os.environ.copy()
    env.update(WINEPREFIX=str(prefix), WINEARCH='win32', WINEDEBUG='-all',
               WINEPATH='T:\\bin', INCLUDE='T:\\include', LIB='T:\\lib')
    for key in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS', 'RC', 'RCPP'):
        env.pop(key, None)
    return env


def initialize(prefix):
    env = environment(prefix)
    if not (prefix / 'system.reg').exists():
        prefix.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['wineboot', '-u'], env=env, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, check=True, timeout=60)
    link = prefix / 'dosdevices/t:'
    if link.is_symlink():
        assert link.resolve() == TC.resolve(), 'T: belongs to a different toolchain'
    else:
        link.symlink_to(TC, target_is_directory=True)
    return env


def resource_records(data):
    """Read Win32 .res headers without normalizing or changing any bytes."""
    result = []
    pos = 0
    while pos < len(data):
        size, header = struct.unpack_from('<II', data, pos)
        cursor = pos + 8

        def identifier():
            nonlocal cursor
            value, = struct.unpack_from('<H', data, cursor)
            cursor += 2
            if value == 0xffff:
                value, = struct.unpack_from('<H', data, cursor)
                cursor += 2
                return value
            chars = []
            while value:
                chars.append(value)
                value, = struct.unpack_from('<H', data, cursor)
                cursor += 2
            return ''.join(map(chr, chars))

        kind, name = identifier(), identifier()
        cursor = (cursor + 3) & ~3
        version, flags, language, resource_version, characteristics = struct.unpack_from('<IHHII', data, cursor)
        assert cursor + 16 <= pos + header and pos + header + size <= len(data)
        if size:
            payload = data[pos + header:pos + header + size]
            result.append(dict(type=kind, name=name, language=language, flags=flags,
                               bytes=size, sha256=hashlib.sha256(payload).hexdigest()))
        pos = (pos + header + size + 3) & ~3
    assert pos == len(data)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', nargs='?', choices=['rc', 'rcpp', 'all'], default='all')
    parser.add_argument('--prefix', type=Path, default=SCRATCH / 'wine')
    args = parser.parse_args()
    manifest = json.loads((HERE / 'manifest.json').read_text())
    env = initialize(args.prefix.resolve())
    for item in manifest['toolchain_dependencies']:
        assert hashlib.sha256((ROOT / item['path']).read_bytes()).hexdigest() == item['sha256'], item['path']
    for item in manifest['source_files']:
        assert hashlib.sha256((HERE / item['path']).read_bytes()).hexdigest() == item['sha256'], item['path']
    report = dict(exact_bytes=True, ignored_fields=[], phases={})
    for name, case in manifest['phases'].items():
        if args.phase not in ('all', name):
            continue
        work = SCRATCH / ('verify-' + name)
        if work.exists():
            shutil.rmtree(work)
        shutil.copytree(HERE / 'source', work)
        for item in case.get('additional_inputs', []):
            source = HERE / item['path']
            assert hashlib.sha256(source.read_bytes()).hexdigest() == item['sha256']
            shutil.copyfile(source, work / item['name'])
        phase_env = env.copy()
        phase_env.update(TMP=windows(work), TEMP=windows(work))
        before = {p.relative_to(work).as_posix() for p in work.rglob('*') if p.is_file()}
        result = subprocess.run(['wine', 'T:\\bin\\' + case['executable']] + case['arguments'],
                                cwd=work, env=phase_env, stdin=subprocess.DEVNULL,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
        assert result.returncode == case['expected_exit'], (name, result.returncode, result.stdout, result.stderr)
        assert result.stdout == (HERE / case['stdout']).read_bytes(), (name, 'stdout', result.stdout)
        assert result.stderr == (HERE / case['stderr']).read_bytes(), (name, 'stderr', result.stderr)
        expected = HERE / case['output']['path']
        actual = (work / case['output']['name']).read_bytes()
        assert hashlib.sha256(expected.read_bytes()).hexdigest() == case['output']['sha256']
        assert actual == expected.read_bytes(), (name, 'product differs')
        after = {p.relative_to(work).as_posix() for p in work.rglob('*') if p.is_file()}
        assert after == before | {case['output']['name']}, (name, 'unexpected files', after - before)
        entry = dict(exit=0, stdout_bytes=len(result.stdout), stderr_bytes=len(result.stderr),
                     output_bytes=len(actual), sha256=hashlib.sha256(actual).hexdigest())
        if name == 'rc':
            entry['resources'] = resource_records(actual)
            assert entry['resources'] == manifest['resource_records']
        report['phases'][name] = entry
        print(name + ': exit 0, raw streams and product match byte for byte')
    (SCRATCH / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
