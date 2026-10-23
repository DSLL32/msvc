#!/usr/bin/env python3
"""Report the clipped union of currently accepted original executable extents.

This follows the masking records, including shared text, matched CRT and imports.
It measures recovered extents, not branch coverage.
"""
import win32_indexes,json
orig,rec,cfg,oi,ri=win32_indexes.indexes()
sections=[(orig.base+s['rva'],orig.base+s['rva']+(s['virtual_size'] or s['size'])) for s in orig.sections if s['flags']&0x20000000]
def union(ranges):
 out=[]
 for a,b in sorted(ranges):
  if out and a<=out[-1][1]:out[-1]=(out[-1][0],max(b,out[-1][1]))
  else:out.append((a,b))
 return out
sections=union(sections);ranges=[]
for r in oi:
 if r['name'] not in cfg['accepted']:continue
 for a,b in [(r['start'],r['end'])]+r.get('shared',[]):
  for x,y in sections:
   if max(a,x)<min(b,y):ranges.append((max(a,x),min(b,y)))
accepted=union(ranges);n=sum(b-a for a,b in accepted);d=sum(b-a for a,b in sections)
print(json.dumps(dict(accepted_code_bytes=n,total_executable_code_bytes=d,accepted_code_percent=100*n/d,accepted_frontiers=len(cfg['accepted']),source_functions=sum(r['name']in cfg['accepted'] for r in cfg['driver']),accepted_source_text_and_switch_bytes=sum(r['end']-r['start']+sum(y-x for x,y in r.get('shared',[])) for r in cfg['driver'] if r['name']in cfg['accepted']),section_ranges=sections,native_limitations=cfg.get("native_limitations", [])),indent=2))
