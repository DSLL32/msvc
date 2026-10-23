"""Offline negative checks for the explicit CRT seam evidence validator."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('projection', root/'scripts/native-traces.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
before = json.loads((root/'cases/analysis/strncmp-before-identity.json').read_text())
record = next(r for r in json.loads((root/'runtime.json').read_text()) if r['name'] == '_strncmp')
evidence = json.loads((root/'cases/analysis/strncmp-observation.json').read_text())
module.check_runtime_evidence(root/'cases/analysis/strncmp-observation.json', '_strncmp', before, record)
changes = [
    ('wrong input string', ('observed_native','left_hex'), '00'),
    ('wrong count', ('observed_native','count'), 5),
    ('different caller predicate', ('observed_native','result'), 0),
    ('different library object', ('original','identity','object_sha256'), '0'*64),
    ('different original extent', ('original','identity','record','end'), record['end']-1),
    ('different checkpoint', ('before_config_sha256',), '0'*64),
]
with tempfile.TemporaryDirectory(prefix='c2-crt-evidence-') as temporary:
    path = Path(temporary)/'evidence.json'
    for label, fields, value in changes:
        changed = copy.deepcopy(evidence)
        slot = changed
        for field in fields[:-1]:
            slot = slot[field]
        slot[fields[-1]] = value
        path.write_text(json.dumps(changed))
        try:
            module.check_runtime_evidence(path, '_strncmp', before, record)
        except AssertionError:
            pass
        else:
            raise AssertionError('accepted ' + label)
print('CRT seam: valid evidence accepted; six inconsistent observations rejected; no execution')
