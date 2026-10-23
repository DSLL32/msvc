"""Replay the recorded final cache extension and reject corrupted exit evidence.

References, native objects and captured bytes are read only. Never invoke Wine.
"""
from pathlib import Path
import copy
import hashlib
import importlib.util
import json
import tempfile
import time

started = time.perf_counter()
spec = importlib.util.spec_from_file_location('traces', 'scripts/native-traces.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
expected = json.loads(Path('cases/native-traces.json').read_text())
before = Path('cases/floating-original-1060.json')
after = Path('cases/floating-original-1061.json')
before_record = json.loads(before.read_text())
# Reverse only the documented final transition, rather than keeping another
# copy of the full multi-case projection. Hash against the saved pre-exit run.
old = copy.deepcopy(expected)
transition = old['provenance']['floating']['extensions'].pop()
assert transition['accepted_function'] == '@coff_section_symbol@4'
old['metadata_sha256']['functions.json'] = before_record['config_sha256']
old['application_definitions']['coff_section_symbol']['source'] = 'src/stubs.c'
old['cases']['floating'].update(last=1060, terminal=before_record['logical'], terminal_kind='application', returncode=-5)
old_bytes = (json.dumps(old,indent=2)+'\n').encode()
old_sha = hashlib.sha256(old_bytes).hexdigest()
assert old_sha == json.loads(Path('cases/floating-original-1061-native.json').read_text())['precompletion_cache_sha256']
rejected = []
with tempfile.TemporaryDirectory(prefix='floating-completion-proof-',dir=Path.home()/'.ai-tmp/vc1.0-c2') as d:
    d = Path(d)
    m.CACHE = d/'cache.json'
    m.CACHE.write_bytes(old_bytes)
    assert m.extend(Path('build/native-trace'),before,after) == expected
    for label in ['nonzero terminal','extra terminal event','corrupt completed object','wrong normalized digest','wrong previous logical entry']:
        evidence = json.loads(after.read_text())
        prior = before
        if label == 'nonzero terminal': evidence['events'] = 'exit 1\n'
        elif label == 'extra terminal event': evidence['events'] += 'exit 0\n'
        elif label == 'wrong normalized digest': evidence['outputs'][0]['normalized_sha256'] = '0'*64
        elif label == 'wrong previous logical entry':
            v = copy.deepcopy(before_record)
            v['logical'] = '@scheduler_call_float_arguments@4'
            prior = d/'before.json'
            prior.write_text(json.dumps(v))
        else:
            artifact = evidence['outputs'][0]
            data = bytearray(Path(artifact['path']).read_bytes())
            data[-1] ^= 1
            bad = d/'bad.obj'
            bad.write_bytes(data)
            artifact.update(path=str(bad),sha256=hashlib.sha256(data).hexdigest())
        altered = d/'after.json'
        altered.write_text(json.dumps(evidence))
        try: m.extend(Path('build/native-trace'),prior,altered)
        except AssertionError: rejected.append(label)
        else: raise AssertionError('accepted '+label)
receipt = dict(positive='same completed cache reproduced from reversed final transition',
    before=str(before),after=str(after),precompletion_cache_sha256=old_sha,
    rejected=rejected,executes_wine=False,seconds=time.perf_counter()-started)
Path('cases/floating-completion-extension-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(receipt)
