#!/usr/bin/env python3
"""Exercise native entry hooks in real i386 PIE and non-PIE executables."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('executable', nargs='+', type=Path)
    args = ap.parse_args()
    scratch = Path.home() / '.ai-tmp/vc1.0-native-trace'
    scratch.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='checks-', dir=scratch))
    function_filter = directory / 'application.txt'
    function_filter.write_text('main\nalpha\nhidden\nbeta\n')
    prefix = directory / 'seen.txt'
    results = []

    def run(executable, name, expected=None, seen='', code=0, disabled=False,
            bad_filter=False, readonly=False):
        start = time.perf_counter()
        env = {key:value for key,value in os.environ.items()
               if not key.startswith('RECON_TRACE_')}
        env['RECON_TRACE_FILTER'] = str(directory / 'missing' if bad_filter else function_filter)
        env['RECON_TRACE_SEEN'] = str(prefix)
        prefix.write_text(seen)
        trace = directory / (executable.name + '-' + name + '.trace')
        trace.write_bytes(b'')
        with trace.open('rb' if readonly else 'wb') as output:
            if not disabled:
                env['RECON_TRACE_FD'] = str(output.fileno())
            process = subprocess.run([str(executable)], env=env,
                                     pass_fds=(output.fileno(),), capture_output=True)
        actual = trace.read_text().splitlines()
        assert process.returncode == code, (name, process.returncode, process.stderr)
        if code == 0:
            assert process.stdout == b'trace target output\n', (name, process.stdout)
            assert process.stderr == b'', (name, process.stderr)
        else:
            assert not process.stdout, (name, process.stdout)
            assert b'recon trace:' in process.stderr, (name, process.stderr)
        if expected is not None:
            assert actual == expected, (name, expected, actual)
        results.append(dict(executable=executable.name, check=name, status='passed',
                            seconds=time.perf_counter()-start, entries=actual))

    for supplied in args.executable:
        executable = supplied.resolve()
        run(executable, 'first-and-repeated', ['main', 'alpha', 'hidden', 'beta'])
        run(executable, 'accepted-prefix', ['beta'], seen='main\nalpha\nhidden\n')
        run(executable, 'zero-new-entries', [], seen='main\nalpha\nhidden\nbeta\n')
        run(executable, 'disabled', [], disabled=True, bad_filter=True)
        run(executable, 'bad-filter', [], code=126, bad_filter=True)
        run(executable, 'bad-prefix', [], code=126, seen='missing_application\n')
        run(executable, 'readonly-fd', [], code=126, readonly=True)
        stripped = directory / (executable.name + '-stripped')
        shutil.copyfile(executable, stripped)
        stripped.chmod(0o700)
        subprocess.run(['strip', '--strip-all', str(stripped)], check=True)
        run(stripped, 'missing-symbols', [], code=126)
        # Disabled hooks do not read either names or symbols, even if stripped.
        run(stripped, 'disabled-no-symbols', [], disabled=True, bad_filter=True)
    receipt = dict(status='passed', scope='native i386 tracing checks only',
                   cases=results, seconds=sum(item['seconds'] for item in results))
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(dict(status=receipt['status'], checks=len(results),
                          seconds=receipt['seconds'], receipt=str(directory / 'receipt.json'))))


if __name__ == '__main__':
    main()
