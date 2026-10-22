#!/usr/bin/env python3
"""Trace candidate cases in original CL without accepting reconstruction code."""
import argparse
import hashlib
from importlib.machinery import SourceFileLoader
import json
from pathlib import Path
import shutil
import subprocess


wt = SourceFileLoader('wine_tool', str(Path(__file__).resolve().with_name('wine-tool.py'))).load_module()
HERE = wt.PROJECT
FIXTURES = HERE / 'cases/remaining'
SCRATCH = Path.home() / '.ai-tmp/vc1.0-cl/probes'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cases', nargs='*', help='case IDs; default: all candidates')
    parser.add_argument('--update', action='store_true', help='retain results for unselected cases')
    args = parser.parse_args()
    catalog = json.loads((FIXTURES / 'catalog.json').read_text())
    unknown = set(args.cases) - {case['id'] for case in catalog['cases']}
    if unknown:
        parser.error('unknown case IDs: ' + ', '.join(sorted(unknown)))
    config = json.loads((HERE / 'functions.json').read_text())
    targets = [r for r in config['driver'] if r['name'] not in config['accepted']]
    requested = {name for case in catalog['cases'] for name in case['targets']}
    requested.update(r['name'] for r in targets)
    sites = [r for r in config['driver'] if r['name'] in requested]
    names = {r['start'] - 0x10000: r['name'] for r in sites}
    SCRATCH.mkdir(parents=True, exist_ok=True)
    addresses = SCRATCH / 'trace-addresses.txt'
    addresses.write_text(''.join('%x\n' % rva for rva in names))
    original = wt.TC / 'bin/CL.EXE'
    assert hashlib.sha256(original.read_bytes()).hexdigest() == config['target']['sha256']
    report = dict(target_sha256=config['target']['sha256'],
                  method='Original-only one-shot function-entry breakpoints; no reconstruction acceptance',
                  remaining_functions_at_start=[r['name'] for r in targets], cases=[])
    if args.update:
        previous = json.loads((SCRATCH / 'coverage.json').read_text())
        assert previous['remaining_functions_at_start'] == report['remaining_functions_at_start']
        report['cases'] = previous['cases']
    for case in catalog['cases']:
        if args.cases and case['id'] not in args.cases:
            continue
        work = SCRATCH / case['id']
        if work.exists():
            shutil.rmtree(work)
        work.mkdir()
        for destination, source in case.get('files', {}).items():
            path = work / destination
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(FIXTURES / source, path)
        for destination, content in case.get('text_files', {}).items():
            path = work / destination
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content.encode('latin1'))
        substitutions = dict(bin=wt.windows(wt.TC/'bin'), cwd=wt.windows(work),
                             proxy=wt.windows(HERE/'build/pass-proxy.exe'),
                             fail=wt.windows(HERE/'build/fail-pass.exe'),
                             wait=wt.windows(HERE/'build/wait-pass.exe'))
        command = case['arguments'].format(**substitutions)
        env = wt.environment()
        for key in ('CL', '_CL_', '_MSC_IDE_FLAGS', 'LINK', '_LINK_', 'MSC_CMD_FLAGS'):
            env.pop(key, None)
        env.update({key: value.format(**substitutions) for key, value in case.get('environment', {}).items()})
        stdout = SCRATCH / (case['id'] + '.stdout')
        stderr = SCRATCH / (case['id'] + '.stderr')
        events = SCRATCH / (case['id'] + '.events')
        events.unlink(missing_ok=True)
        invoke = ['wine', wt.windows(HERE/'build/trace-run.exe'), wt.windows(original), command,
                  wt.windows(stdout), wt.windows(stderr), wt.windows(events), wt.windows(work), wt.windows(addresses)]
        if case.get('mode'):
            invoke.append(case['mode'])
        result = subprocess.run(invoke, env=env, stdin=subprocess.DEVNULL,
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=80)
        log = events.read_text().splitlines() if events.exists() else []
        hits = [names[int(line.split()[1], 16)] for line in log if line.startswith('hit ')]
        new_hits = [name for name in hits if name in report['remaining_functions_at_start']]
        exits = [int(line.split()[1], 16) for line in log if line.startswith('exit ')]
        entry = dict(id=case['id'], arguments=case['arguments'], launcher_status=result.returncode,
                     exit_status=exits[-1] if exits else None, reached=new_hits,
                     intended_targets=case['targets'],
                     missed_targets=sorted(set(case['targets']) - set(hits)),
                     stdout=stdout.read_bytes().decode('latin1') if stdout.exists() else '',
                     stderr=stderr.read_bytes().decode('latin1') if stderr.exists() else '',
                     events=log)
        entry['verified'] = (result.returncode == 0 and bool(exits) and
                             ('expected_exit' not in case or exits[-1] == case['expected_exit']) and
                             set(case.get('expected_events', [])) <= set(log) and
                             not any(line.startswith(('exception ', 'timeout ', 'dispatch-error ')) for line in log) and
                             (not entry['missed_targets'] or case.get('investigation', False)))
        report['cases'] = [r for r in report['cases'] if r['id'] != case['id']]
        report['cases'].append(entry)
        print('%-29s exit=%-4s %s' % (case['id'], entry['exit_status'], ', '.join(new_hits) or '(no new source function)'), flush=True)
        (SCRATCH / 'coverage.json').write_text(json.dumps(report, indent=2)+'\n')
    reached = {name for c in report['cases'] for name in c['reached']}
    order = {c['id']: i for i, c in enumerate(catalog['cases'])}
    report['cases'].sort(key=lambda c: order[c['id']])
    report['reached_functions'] = sorted(reached)
    report['unreached_functions'] = sorted(set(report['remaining_functions_at_start']) - reached)
    (SCRATCH / 'coverage.json').write_text(json.dumps(report, indent=2)+'\n')
    print('%d/%d remaining named functions reached; report: %s' %
          (len(reached), len(targets), SCRATCH / 'coverage.json'))
    if any(not c.get('verified', False) for c in report['cases']):
        raise SystemExit('A probe did not reach its expected targets or exit status; inspect coverage.json')


if __name__ == '__main__':
    main()
