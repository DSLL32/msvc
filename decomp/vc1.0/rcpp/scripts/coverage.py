#!/usr/bin/env python3
"""Accepted original code extents, not executed-instruction/branch coverage."""
import json
from pathlib import Path
from compare import indexes

def union(ranges):
    merged=[]
    for start,end in sorted(ranges):
        if start>=end: continue
        if merged and start<=merged[-1][1]: merged[-1][1]=max(end,merged[-1][1])
        else: merged.append([start,end])
    return merged

cfg,original,rec,oi,ri=indexes()
accepted=set(cfg['accepted'])
ranges=[]
for function in oi:
    if function['name'] in accepted:
        ranges += [(function['start'],function['end'])] + function.get('shared',[])
sections=[]
all_code=[]
clipped=[]
for section in original.sections:
    if not section['flags'] & 0x20:continue
    start=original.base+section['rva']
    end=start+section['virtual_size']
    all_code.append((start,end))
    covered=union((max(start,a),min(end,b))for a,b in ranges)
    clipped+=covered
    sections.append(dict(name=section['name'],start=start,end=end,total_bytes=end-start,
                         accepted_union_bytes=sum(b-a for a,b in covered)))
covered=union(clipped)
size=sum(b-a for a,b in union(all_code))
count=sum(b-a for a,b in covered)
report=dict(metric='accepted original code-extent union; not branch or executed-instruction coverage',
            target_sha256=cfg['target']['sha256'],includes=['accepted full functions','shared .text data','identified CRT functions','import thunks'],
            extent_rule='VirtualSize, or backed raw size when VirtualSize is zero',
            accepted_union_bytes=count,total_original_code_bytes=size,percentage=100.0*count/size,
            sections=sections,accepted_clipped_union=covered)
Path('coverage.json').write_text(json.dumps(report,indent=2)+'\n')
print('%d / %d original code bytes accepted (%.2f%%); not branch coverage.'%(count,size,100.0*count/size))
