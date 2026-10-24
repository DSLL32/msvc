"""Explicit final and encountered transient filenames at a case checkpoint."""
import hashlib
import json
from pathlib import Path

BINARY = Path(__file__).resolve().parents[1]


def declared_outputs(phase, case):
    transient = case.get('transient_outputs', [])
    for item in transient:
        evidence = BINARY/item['evidence']
        raw = evidence.read_bytes()
        assert hashlib.sha256(raw).hexdigest() == item['evidence_sha256'], 'transient discovery evidence changed'
        assert item['output_name'] in {row['path'] for row in json.loads(raw)['visible_files']}, 'transient name absent from actual discovery'
    rows = phase['outputs'] + transient
    names = [item['output_name'] for item in rows]
    assert len(names) == len(set(names)), 'duplicate declared output'
    assert all(name and name not in ('.','..') and Path(name).name == name and '\\' not in name for name in names), 'output name escapes case directory'
    assert set(names).isdisjoint({Path(item['path']).name for item in phase['inputs']}), 'output/input names collide'
    return names
