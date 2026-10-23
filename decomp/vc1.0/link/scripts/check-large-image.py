#!/usr/bin/env python3
"""Host-only checks of original large-image artifacts and narrow clock masking."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import time

start = time.perf_counter()
spec = importlib.util.spec_from_file_location('large_artifacts', Path(__file__).with_name('artifacts-large.py'))
parser = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parser)
base = Path('cases/large-image-fixups')
validation = json.loads((base/'original-validation.json').read_text())
positive, negative = [], []
for case, record in validation.items():
    for output in record['outputs']:
        paths = [Path(run['path'])/output['output_name'] for run in record['runs']]
        left, right = [p.read_bytes() for p in paths]
        fields = parser.timestamp_fields(left, output['artifact_kind'])
        assert fields == output['timestamp_fields'] == parser.timestamp_fields(right, output['artifact_kind'])
        assert parser.normalized(left, fields) == parser.normalized(right, fields)
        positive.append(case+':'+output['output_name'])

image = (base/'reference/large-image-mapped/1/large.dll').read_bytes()
layout = parser.image(image)
def denied(label, data):
    try:
        fields = parser.timestamp_fields(data, 'pe-coff-fixup')
        assert parser.normalized(data, fields) == parser.normalized(image, layout['fields'])
    except (AssertionError, struct.error):
        negative.append(label)
    else:
        raise AssertionError('Unjustified artifact mutation accepted: '+label)

for label, offset in [('entry',0xa8),('stack-reserve',0xe0),('heap-commit',0xec),
                      ('data-first-byte',2048),('relocated-pointer',2048+0x300108),
                      ('fixup-target',layout['debug'][1]['offset']+8),
                      ('debug-timestamp-inconsistent',layout['fields'][1]['offset']),
                      ('physical-tail',len(image)-1)]:
    changed=bytearray(image);changed[offset]^=1;denied(label,bytes(changed))
denied('physical-length',image[:-1])
changed=bytearray(image)
for field in layout['fields']:
    struct.pack_into('<I',changed,field['offset'],layout['timestamp']+17)
assert parser.normalized(changed,parser.timestamp_fields(changed,'pe-coff-fixup')) == parser.normalized(image,layout['fields'])
positive.append('exactly-three-consistent-clock-fields')
# Fresh decoder-entry proofs must be nonempty and match the independent original
# samples, including every scalar field before the new callbacks run.
from decoder_state import compare
progress=json.loads((base/'progress.json').read_text())
for entry in progress['accepted']:
    receipt=json.loads(Path(entry['frontier_receipt']).read_text())
    if receipt.get('decoder_state'):
        observations=[json.loads(line) for line in Path(entry['original_events']).read_text().splitlines() if json.loads(line)['event']=='decoder_state']
        compare(observations,receipt['decoder_state'])
        positive.append(entry['name']+':224-byte-decoder-state')
assert len([p for p in positive if ':224-byte-decoder-state' in p])==3
print(json.dumps(dict(status='pass',positive=positive,rejected=negative,seconds=time.perf_counter()-start),indent=2))
