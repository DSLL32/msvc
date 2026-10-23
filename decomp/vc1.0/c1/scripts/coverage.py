#!/usr/bin/env python3
"""Accepted original code extents, including shared text data and runtime.

This is a clipped byte-extent union, not instruction or branch coverage.
"""
from images import indexes
import json
import re
from pathlib import Path

original, _, config, records, _ = indexes()
code = [(original.base+s['rva'], original.base+s['rva']+s['virtual_size'])
        for s in original.sections if s['flags'] & 0x20]
accepted = set(config['accepted'])
ranges = []
public_ranges, direct_ranges = [], []
direct_names = {name for case in config.get('direct_cases',{}).values() if not case['public_reachability'] for name in config['accepted'][case['accepted_from']:case['accepted_count']]}
by_origin = {'frontend_source': [], 'CRT': [], 'import_thunks': []}
owners = {}
for line in Path('build/c1.map').read_text().splitlines():
    match = re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
    if match:
        owners[match[1]] = line.split()[-1]

for record in records:
    if record['name'] not in accepted:
        continue
    if record['name'].startswith('import:'):
        origin = 'import_thunks'
    elif 'library' in record or owners.get(record['name']) == 'runtime.obj':
        origin = 'CRT'
    else:
        origin = 'frontend_source'
    for start, end in [(record['start'], record['end'])] + record.get('shared', []):
        for a, b in code:
            left, right = max(a, start), min(b, end)
            if left < right:
                ranges.append((left, right))
                (direct_ranges if record['name'] in direct_names else public_ranges).append((left,right))
                by_origin[origin].append((left, right))
merged = []
for start, end in sorted(ranges):
    if merged and start <= merged[-1][1]:
        merged[-1][1] = max(merged[-1][1], end)
    else:
        merged.append([start, end])
size = sum(end-start for start, end in merged)
total = sum(end-start for start, end in code)
origin_sizes = {}
for origin, rows in by_origin.items():
    intervals = []
    for start, end in sorted(rows):
        if intervals and start <= intervals[-1][1]:
            intervals[-1][1] = max(intervals[-1][1], end)
        else:
            intervals.append([start, end])
    origin_sizes[origin] = sum(end-start for start, end in intervals)
assert sum(origin_sizes.values()) == size, 'origin classifications overlap; audit the boundary'
def union_size(rows):
    merged=[]
    for a,b in sorted(rows):
        if merged and a<=merged[-1][1]: merged[-1][1]=max(b,merged[-1][1])
        else: merged.append([a,b])
    return sum(b-a for a,b in merged)

public_size=union_size(public_ranges)
direct_size=union_size(direct_ranges)
assert public_size+direct_size==size, 'public/direct extents overlap; classify shared ownership explicitly'
print(json.dumps(dict(public_case_accepted_union_bytes=public_size,
                     direct_only_accepted_union_bytes=direct_size,
                     direct_only_functions=sorted(direct_names),
                     accepted_union_bytes=size, original_code_bytes=total,
                     accepted_by_origin_bytes=origin_sizes,
                     origin_classification='Frontend: source driver functions outside runtime.c. CRT: runtime.c overrides plus matched LIBC.LIB/SETARGV code. Imports: import thunks. Includes shared text data; categories are disjoint. This describes the reconstruction origins, not a complete original linker map.',
                     percent=100*size/total,
                     metric='Union of accepted original code extents, shared text data, CRT and import thunks, clipped to code-section VirtualSize; not branch coverage.'), indent=2))
