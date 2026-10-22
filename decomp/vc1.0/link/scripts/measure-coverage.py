#!/usr/bin/env python3
"""Union of accepted extents inside original .text VirtualSize, not branch coverage."""
import argparse
import json
from pathlib import Path
from compare import indexes

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path)
args = parser.parse_args()
original, _, cfg, records, _ = indexes()
text = next(section for section in original.sections if section['name'] == '.text')
lo = original.base + text['rva']
hi = lo + text['virtual_size']
ranges = []
for record in records:
    if record['name'] not in cfg['accepted']:
        continue
    for start, end in [(record['start'], record['end'])] + record.get('shared', []):
        start, end = max(start, lo), min(end, hi)
        if start < end:
            ranges.append((start, end))
merged = []
for start, end in sorted(ranges):
    if merged and start <= merged[-1][1]:
        merged[-1][1] = max(merged[-1][1], end)
    else:
        merged.append([start, end])
size = sum(end - start for start, end in merged)
report = {'target': cfg['target'], 'accepted_frontiers': len(cfg['accepted']),
          'accepted_text_bytes': size, 'total_text_virtual_bytes': hi - lo,
          'percent': size / (hi - lo) * 100,
          'measure': __doc__, 'ranges': merged}
if args.output:
    args.output.write_text(json.dumps(report, indent=2) + '\n')
print('%d frontiers, %d / %d original .text bytes = %.4f%%' %
      (len(cfg['accepted']), size, hi - lo, report['percent']))
