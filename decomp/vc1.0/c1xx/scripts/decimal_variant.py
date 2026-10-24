"""Retain the reviewed older decimal-library binding; no CRT body or new credit.

The tagged historical proof establishes the seven-word cdecl ABI, logical callee
set and bounded first conversion. These checks bind that decision to unchanged
original/library/member identities. They do not re-prove broader input domains.
"""
import hashlib, json
from pathlib import Path
from binary import PE, archive
BINARY = Path(__file__).resolve().parents[1]
TC = BINARY.parents[2] / 'toolchains/1.0-nt'
def default_record():
    return json.loads((BINARY/'runtime-compatible.json').read_text())['record']
def verify(record=None, original=None):
    p = json.loads((BINARY/'runtime-compatible.json').read_text())
    record = p['record'] if record is None else record
    assert record == p['record'], 'decimal binding record changed'
    original = PE(TC/'bin/C1XX3232.EXE') if original is None else original
    assert hashlib.sha256(original.data).hexdigest() == p['target_sha256']
    assert hashlib.sha256(original.read(record['start'],record['end']-record['start'])).hexdigest() == p['original_body_sha256']
    library = TC/'lib/LIBC.LIB'
    assert hashlib.sha256(library.read_bytes()).hexdigest() == p['library_sha256']
    assert hashlib.sha256(dict(archive(library))[record['object']]).hexdigest() == p['member_sha256']
    assert p['source_credit'] == p['application_credit'] == 0
    return p

def original_records(path):
    if Path(path).resolve() != (TC/'bin/C1XX3232.EXE').resolve(): return []
    verify()
    return [default_record()]
