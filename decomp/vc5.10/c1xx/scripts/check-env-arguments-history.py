#!/usr/bin/env python3
"""Revalidate frozen environment-parser pairs with current static audit; never execute."""
import argparse
import importlib.util
import json
from pathlib import Path
import time


def load(name, filename):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).with_name(filename))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--role', choices=('normal', 'restore', 'all'), default='all')
    args = parser.parse_args()
    packet = json.loads(Path('cases/minimal/env-arguments-trial.json').read_text())
    results = []
    for role in ('normal', 'restore') if args.role == 'all' else (args.role,):
        start = time.perf_counter()
        record = packet['proofs'][role]
        decoder = load('env_frozen_' + role, 'check-callback.py')
        history = load('env_history_' + role, 'check-history.py')
        original, native = Path(record['original']), Path(record['native'])
        audit = history.configure(decoder, original, native, saved_record=record)
        results.append(dict(role=role, result=decoder.check(original, native, audit_path=audit), current_audit=str(audit), seconds=time.perf_counter() - start))
    print(json.dumps(results, indent=2))
