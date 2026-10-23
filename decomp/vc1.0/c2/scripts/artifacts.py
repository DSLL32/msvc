"""Strict C2 artifact normalization: only the proven COFF timestamp is a clock."""
from pathlib import Path
import hashlib


def digest(data):return hashlib.sha256(data).hexdigest()


def normalize(data, record):
    name=record.get('output_name',Path(record['path']).name)
    fields=record['ignored_fields']
    if fields:
        assert name.lower().endswith('.obj'), 'Clock exemption is only proven for COFF objects'
        assert len(fields)==1 and (fields[0]['offset'],fields[0]['bytes'])==(4,4), 'Unknown clock exemption'
        assert len(data)>=8
    value=bytearray(data)
    for field in fields:value[field['offset']:field['offset']+field['bytes']]=b'\0'*field['bytes']
    return bytes(value)


def compare(data, expected, record):
    assert digest(expected)==record['sha256'], 'Captured artifact changed: '+record['path']
    assert len(expected)==record['bytes'], 'Captured artifact length changed'
    actual=normalize(data,record);reference=normalize(expected,record)
    assert actual==reference, 'Artifact differs: '+record['path']
    assert digest(actual)==record['normalized_sha256'], 'Artifact normalized hash differs'
    return dict(name=record.get('output_name',Path(record['path']).name),bytes=len(data),
                normalized_sha256=digest(actual),ignored_timestamp=[4,4] if record['ignored_fields'] else [])
