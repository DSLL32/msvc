"""CL case descriptions and frozen original oracles; no executable discovery."""
import hashlib
import json
from pathlib import Path

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
TC = ROOT/'toolchains/1.0-nt'
CASES = BINARY/'cases'

def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def specifications():
    result = {}
    for name in ('catalog', 'internal', 'extra'):
        for case in json.loads((CASES/'remaining'/(name+'.json')).read_text())['cases']:
            result[case['id']] = dict(case)
    result['help'] = dict(id='help', arguments='/?', expected_exit=0)
    result['compile'] = dict(id='compile', arguments='test.c', expected_exit=0, files={'test.c':'../test.c'})
    result['hello'] = dict(id='hello', arguments='/O2 /G5 hello.c', expected_exit=0, files={'hello.c':'../hello.c'})
    return result

def substitutions(case_id):
    return dict(bin=windows(TC/'bin'), cwd='Q:\\'+case_id,
                proxy=windows(BINARY/'build/pass-proxy.exe'),
                fail=windows(BINARY/'build/fail-pass.exe'),
                wait=windows(BINARY/'build/wait-pass.exe'))

def prepare(case, work):
    work.mkdir(parents=True, exist_ok=True)
    for destination, source in case.get('files', {}).items():
        target = work/destination
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((CASES/'remaining'/source).read_bytes())
    for destination, text in case.get('text_files', {}).items():
        target = work/destination
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(text.encode('latin1'))

def oracles():
    rows = {r['id']:r for r in json.loads((CASES/'remaining/validation.json').read_text())['cases']}
    for name in ('help','compile','hello'):
        row = json.loads((CASES/(name+'.json')).read_text())
        rows[name] = row
    return rows
