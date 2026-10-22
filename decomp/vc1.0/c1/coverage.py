#!/usr/bin/env python3
"""Accepted original code extents, including shared text data and runtime.

This is a clipped byte-extent union, not instruction or branch coverage.
"""
from compare import indexes
import json

original, _, config, records, _ = indexes()
code = [(original.base+s['rva'], original.base+s['rva']+s['virtual_size'])
        for s in original.sections if s['flags'] & 0x20]
accepted = set(config['accepted'])
ranges = []
for record in records:
    if record['name'] not in accepted:
        continue
    for start, end in [(record['start'], record['end'])] + record.get('shared', []):
        for a, b in code:
            left, right = max(a, start), min(b, end)
            if left < right:
                ranges.append((left, right))
merged = []
for start, end in sorted(ranges):
    if merged and start <= merged[-1][1]:
        merged[-1][1] = max(merged[-1][1], end)
    else:
        merged.append([start, end])
size = sum(end-start for start, end in merged)
total = sum(end-start for start, end in code)
print(json.dumps(dict(accepted_union_bytes=size, original_code_bytes=total,
                     percent=100*size/total,
                     metric='Union of accepted original code extents, shared text data, CRT and import thunks, clipped to code-section VirtualSize; not branch coverage.'), indent=2))
