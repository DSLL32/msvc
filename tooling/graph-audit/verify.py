#!/usr/bin/env python3
"""Explicit read-only snapshot/parity/benchmark driver; never called by Rust.

All baseline reports are written within a scratch copy, never a worker project.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import time

TOOLS = ('cl', 'c1', 'c1xx', 'c2', 'link', 'rc', 'rcpp', 'cvpack')

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def snapshot(repo, destination):
    destination.mkdir(parents=True, exist_ok=True)
    for name in ('lib', 'tooling'):
        path = destination/name
        if not path.exists(): path.symlink_to(repo/name, target_is_directory=True)
    toolchains=destination/'toolchains'
    if not toolchains.exists():
        shutil.copytree(repo/'toolchains/1.0-nt',toolchains/'1.0-nt')
    base = destination/'decomp/vc1.0'
    base.mkdir(parents=True, exist_ok=True)
    fixtures = base/'fixtures'
    if not fixtures.exists(): shutil.copytree(repo/'decomp/vc1.0/fixtures',fixtures)
    receipts = {}
    for tool in TOOLS:
        source = repo/'decomp/vc1.0'/tool
        target = base/tool
        for attempt in range(10):
            if target.exists(): shutil.rmtree(target)
            target.mkdir()
            files = [p for p in source.glob('*') if p.is_file()]
            files += list((source/'scripts').rglob('*.py'))
            files += list((source/'src').rglob('*')) if tool == 'cvpack' else []
            files += [p for p in (source/'build').glob('*') if p.is_file() and
                      (p.suffix.lower() in ('.exe','.map') or p.name == 'runtime.json')]
            # Evidence existence is checked by CVPACK ownership/source policy.
            if tool == 'cvpack':
                cfg = json.loads((source/'functions.json').read_text())
                for record in cfg['functions'] + cfg.get('runtime_aliases', []) + cfg.get('support_unmasked', []):
                    if record.get('evidence'): files.append(source/record['evidence'])
                for record in json.loads((source/'runtime-anchors.json').read_text()).get('anchors', []):
                    if record.get('evidence'): files.append(source/record['evidence'])
                for record in json.loads((source/'runtime-support.json').read_text()):
                    if record.get('evidence'): files.append(source/record['evidence'])
                    if record.get('input'):
                        dependency=repo/record['input']['path']
                        assert sha(dependency)==record['input']['sha256']
                        if dependency.is_relative_to(source): files.append(dependency)
                        else:
                            output=destination/record['input']['path']
                            output.parent.mkdir(parents=True,exist_ok=True)
                            shutil.copyfile(dependency,output)
            files = sorted(set(p for p in files if p.is_file()))
            hashes = {str(p.relative_to(source)): sha(p) for p in files}
            for p in files:
                out = target/p.relative_to(source)
                out.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(p, out)
            if all(sha(source/p) == h == sha(target/p) for p,h in hashes.items()):
                receipts[tool] = hashes
                break
        else: raise RuntimeError(f'{tool}: inputs kept mutating during snapshot')
    (destination/'snapshot.json').write_text(json.dumps(receipts, indent=2)+'\n')
    return receipts

def run(command, cwd):
    start = time.perf_counter()
    process = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    return dict(seconds=time.perf_counter()-start, status=process.returncode,
                stdout=process.stdout, stderr=process.stderr)

def benchmark(binary, root, repetitions, tools=TOOLS):
    results = {}
    for tool in tools:
        started=__import__('datetime').datetime.now(__import__('datetime').timezone.utc).isoformat()
        project = root/'decomp/vc1.0'/tool
        py = [sys.executable, 'scripts/audit-graph.py'] if tool != 'cvpack' else [sys.executable, str(Path(__file__).with_name('cvpack-reference.py')), '--project', str(project), '--cache', str(project/'graph-identities.json')]
        rust = [str(binary), '--profile', tool, '--repo', str(root), '--project', str(project), '--discovery', 'legacy']
        if tool == 'cvpack': rust += ['--rebuilt-runtime', str(project/'graph-identities.json')]
        positive_py = run(py, project)
        positive_rs = run(rust, project)
        if positive_py['status'] or positive_rs['status']:
            results[tool] = dict(python=positive_py, rust=positive_rs)
            print(tool, 'FAILED', json.dumps(results[tool]), flush=True)
            continue
        if tool == 'cvpack':
            canonical=json.loads(positive_py['stdout'])
        else:
            extracted=run([sys.executable,str(Path(__file__).with_name('parity-reference.py')),tool],project)
            assert extracted['status']==0,extracted
            canonical=json.loads(extracted['stdout'])
        rust_report=json.loads(positive_rs['stdout'])
        assert canonical['checked_functions']==rust_report['checked_functions'],tool
        assert canonical['functions']==rust_report['functions'],tool
        py_runs, rs_runs = [], []
        for _ in range(repetitions):
            # Alternate order to limit simple thermal/cache ordering bias.
            order = [(py,py_runs),(rust,rs_runs)] if _ % 2 == 0 else [(rust,rs_runs),(py,py_runs)]
            for command, values in order:
                result=run(command,project)
                assert result['status']==0, result
                values.append(result['seconds'])
        summary = lambda values: dict(median_seconds=statistics.median(values), min_seconds=min(values), max_seconds=max(values), samples_seconds=values)
        results[tool] = dict(python=summary(py_runs),rust=summary(rs_runs), python_stdout=positive_py['stdout'], rust_report=json.loads(positive_rs['stdout']))
        p=results[tool]['python']['median_seconds']; r=results[tool]['rust']['median_seconds']
        results[tool].update(speedup=p/r,saved_seconds=p-r,started_utc=started,finished_utc=__import__('datetime').datetime.now(__import__('datetime').timezone.utc).isoformat(),binary_sha256=sha(binary))
        print(tool, f'{p:.6f}s -> {r:.6f}s ({p/r:.2f}x)', flush=True)
    return results

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[2])
    parser.add_argument('--snapshot',type=Path,required=True)
    parser.add_argument('--reuse',action='store_true')
    parser.add_argument('--snapshot-only',action='store_true')
    parser.add_argument('--binary',type=Path,default=Path(__file__).parent/'target/release/graph-audit')
    parser.add_argument('--repetitions',type=int,default=7)
    parser.add_argument('--tools',nargs='+',choices=TOOLS,default=TOOLS)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    if not args.reuse: snapshot(args.repo.resolve(),args.snapshot.resolve())
    if args.snapshot_only:
        print('Snapshot complete:',args.snapshot.resolve())
        return
    result=benchmark(args.binary.resolve(),args.snapshot.resolve(),args.repetitions,args.tools)
    args.report.write_text(json.dumps(dict(uname=list(os.uname()),load_average=os.getloadavg(),results=result),indent=2)+'\n')
    assert all('speedup' in r for r in result.values())
if __name__=='__main__': main()
