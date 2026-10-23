#!/usr/bin/env python3
"""Test immutable native build-source reuse without changing live source/cache."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
import time
import provenance


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    args = parser.parse_args()
    original_root, original_project = provenance.ROOT, provenance.PROJECT
    capture = args.capture.resolve()
    records = json.loads((capture / 'source-snapshot.json').read_text())
    prefix = str(original_project.relative_to(original_root)) + '/'
    owned = [name for name in records if name.startswith(prefix)]
    tests = ['valid', 'different-elf', 'different-candidate', 'changed-owned-source',
             'missing-archived-content', 'corrupted-archived-content']
    started = time.perf_counter()
    for test in tests:
        with tempfile.TemporaryDirectory(prefix='source-reuse-', dir=Path.home() / '.ai-tmp/msvc2/vc5.10/c1') as temporary:
            scratch = Path(temporary)
            root = scratch / 'repo'
            project = root / original_project.relative_to(original_root)
            origin = scratch / 'capture'
            shutil.copytree(capture, origin)
            for name in owned:
                destination = root / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(records[name]['content'], destination)
            for relative in ['build/native/c1-host', 'build/c1.dll', 'cases/minimal/candidate.json', 'cases/minimal/trial.json']:
                destination = project / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(original_project / relative, destination)
            if test == 'different-elf':
                with (project / 'build/native/c1-host').open('ab') as stream:
                    stream.write(b'!')
            elif test == 'different-candidate':
                with (project / 'cases/minimal/candidate.json').open('a') as stream:
                    stream.write(' ')
            elif test == 'changed-owned-source':
                name = next(name for name in owned if name.endswith('.c'))
                with (root / name).open('a') as stream:
                    stream.write('\n/* changed */\n')
            elif test in ('missing-archived-content', 'corrupted-archived-content'):
                manifest = json.loads((origin / 'source-snapshot.json').read_text())
                name = next(iter(manifest))
                archive = scratch / 'private-cached-content'
                archive.write_bytes(b'corrupted') if test.startswith('corrupted') else None
                manifest[name]['content'] = str(archive)
                (origin / 'source-snapshot.json').write_text(json.dumps(manifest))
                receipt = json.loads((origin / 'receipt.json').read_text())
                file = origin / 'source-snapshot.json'
                receipt['artifacts'][file.name] = {'sha256': provenance.sha(file), 'size': file.stat().st_size}
                (origin / 'receipt.json').write_text(json.dumps(receipt))
            provenance.ROOT, provenance.PROJECT = root, project
            try:
                provenance.reuse_native_sources(origin, scratch / 'result.json', project / 'build/native/c1-host',
                                                project / 'cases/minimal/candidate.json', project / 'build/c1.dll',
                                                project / 'cases/minimal/trial.json')
            except (AssertionError, FileNotFoundError):
                assert test != 'valid', 'valid immutable reuse was rejected'
            else:
                assert test == 'valid', 'accepted invalid reuse: ' + test
            finally:
                provenance.ROOT, provenance.PROJECT = original_root, original_project
    print(json.dumps({'status': 'PASS', 'valid_reuse': True, 'rejected': tests[1:],
                      'seconds': time.perf_counter() - started}))


if __name__ == '__main__':
    main()
