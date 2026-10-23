"""Separate indexed application/library extents from still unclassified code."""
from pathlib import Path
import hashlib
import json
from binary import PE
from pe_indexes import imports


def union(ranges):
    result=[]
    for start,end in sorted(ranges):
        if start>=end: continue
        if result and start<=result[-1][1]: result[-1][1]=max(end,result[-1][1])
        else: result.append([start,end])
    return result

config=json.loads(Path('functions.json').read_text())
original=PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
assert hashlib.sha256(original.data).hexdigest()==config['target']['sha256']
sections=union((original.base+s['rva'],original.base+s['rva']+(s['virtual_size']or s['size']))
               for s in original.sections if s['flags']&0x20)
accepted=set(config['accepted'])
groups={'application':config['driver'],'crt':json.loads(Path('runtime.json').read_text()),
        'import_thunks':imports(original)}

def extents(rows):
    return union((max(a,lo),min(b,hi)) for r in rows
                 for a,b in [[r['start'],r['end']]]+r.get('shared',[])
                 for lo,hi in sections if max(a,lo)<min(b,hi))

def size(ranges): return sum(b-a for a,b in ranges)

report=dict(checkpoint=len(accepted),target_sha256=config['target']['sha256'],
    metric='Index inventory, not recovered function/branch coverage. Group unions may overlap and are not additive. Provisional entries do not measure bodies.')
for name,rows in groups.items():
    covered=[r for r in rows if r['name']in accepted]
    pending=[r for r in rows if r['name']not in accepted]
    report[name]=dict(indexed_rows=len(rows),accepted_rows=len(covered),
        accepted_union_bytes=size(extents(covered)),unaccepted_rows=len(pending),
        unaccepted_provisional_rows=sum(bool(r.get('provisional'))for r in pending),
        unaccepted_indexed_extent_bytes=size(extents(pending)),
        unaccepted_nonprovisional_indexed_extent_bytes=size(extents([r for r in pending if not r.get('provisional')])))
all_indexed=extents([r for rows in groups.values()for r in rows])
unknown=[]
for lo,hi in sections:
    cursor=lo
    for a,b in all_indexed:
        if b<=lo or a>=hi:continue
        if cursor<a:unknown.append([cursor,a])
        cursor=max(cursor,b)
    if cursor<hi:unknown.append([cursor,hi])
report.update(original_code_bytes=size(sections),unclassified_code_union_bytes=size(unknown),
    unclassified_ranges=len(unknown),
    largest_unclassified_ranges=[dict(start=hex(a),end=hex(b),bytes=b-a)for a,b in sorted(unknown,key=lambda r:r[1]-r[0],reverse=True)[:20]],
    limit='Unclassified ranges can include application/support functions, code data and alignment; no CRT/stdlib attribution without identity evidence.',
    pending_application=[dict(**r,extent_status='entry placeholder'if r.get('provisional')else'indexed extent')for r in config['driver']if r['name']not in accepted])
Path('cases/remaining-code-inventory.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k not in['pending_application','largest_unclassified_ranges']},indent=2))
