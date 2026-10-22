"""Count accepted original code extents, including shared data, without overlap."""
import hashlib
import json
import tempfile
from pathlib import Path
from compare import PE, imports, mask, wt

def union(ranges):
    result = []
    for start, end in sorted(ranges):
        if start >= end:
            continue
        if result and start <= result[-1][1]:
            result[-1][1] = max(end, result[-1][1])
        else:
            result.append([start, end])
    return result

cfg = json.loads(Path('functions.json').read_text())
pe = PE(wt.TC / 'bin/C23232.EXE')
assert hashlib.sha256(pe.data).hexdigest() == cfg['target']['sha256']
sections = union((pe.base+s['rva'], pe.base+s['rva']+(s['virtual_size'] or s['size']))
                 for s in pe.sections if s['flags'] & 0x20)
records = json.loads(Path('runtime.json').read_text()) + imports(pe) + cfg['driver']
accepted = set(cfg['accepted'])
assert accepted <= {r['name'] for r in records}
ranges = []
for record in records:
    if record['name'] in accepted:
        for start, end in [[record['start'], record['end']]] + record.get('shared', []):
            for code_start, code_end in sections:
                if max(start, code_start) < min(end, code_end):
                    ranges.append((max(start, code_start), min(end, code_end)))
covered = union(ranges)
accepted_bytes = sum(end-start for start, end in covered)
total_bytes = sum(end-start for start, end in sections)
# Independently check the computed union against the actual trap masker.
# Counting ranges, rather than equal bytes, avoids treating native INT3/padding
# bytes as accepted merely because they equal a masked byte.
wt.PREFIX.parent.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='coverage-', dir=wt.PREFIX.parent) as temporary:
    masked_path = Path(temporary) / 'original.exe'
    mask(pe, records, accepted, masked_path)
    masked = PE(masked_path)
    for code_start, code_end in sections:
        expected = bytearray(b'\xcc' * (code_end-code_start))
        for start, end in covered:
            a, b = max(start, code_start), min(end, code_end)
            if a < b:
                expected[a-code_start:b-code_start] = pe.read(a, b-a)
        assert masked.read(code_start, code_end-code_start) == expected, 'coverage/mask disagreement'
result = dict(mask_verified=True, metric='accepted original code-extent union; not branch coverage',
    target_sha256=cfg['target']['sha256'], accepted_frontiers=len(cfg['accepted']),
    accepted_code_bytes=accepted_bytes, original_code_bytes=total_bytes,
    percentage=100.0*accepted_bytes/total_bytes,
    method='union accepted source, shared text data, CRT and import-thunk ranges; clip to original code sections using VirtualSize, falling back to raw size when zero',
    code_sections=[[hex(a),hex(b)] for a,b in sections], union_ranges=len(covered))
print(json.dumps(result, indent=2))
