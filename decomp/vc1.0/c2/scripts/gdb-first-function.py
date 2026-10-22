"""Explicit native GDB diagnostic; set $c2_capture_path before sourcing.

Stops at the next function's is_function_reference call after HelloWindowProc.
Reads typed bounded COFF writer objects, never an original process/image.
"""
import gdb,json
from pathlib import Path

gdb.Breakpoint('is_function_reference', internal=True)
gdb.execute('run')
assert 'is_function_reference' in gdb.execute('bt 2',to_string=True)
uint=gdb.lookup_type('unsigned int');voidpp=gdb.lookup_type('void').pointer().pointer();inferior=gdb.selected_inferior()
def vector(v):
    cap=int(v['capacity']);count=int(v['count']);block=v['first'];out=[]
    while len(out)<count:
        items=block['items'].address.cast(voidpp)
        for i in range(min(cap,count-len(out))):out.append(items[i])
        block=block['next']
    return out
rows=[]
for pointer in vector(gdb.parse_and_eval('coff_contributions')):
    c=pointer.cast(gdb.lookup_type('CoffContribution').pointer()).dereference();scope=c['scope'];block=scope['block'];last=int(block);first=block['next'];cursor=first;data=b'';seen=set()
    while int(cursor) not in seen:
        seen.add(int(cursor));n=int(scope['buffered_bytes']) if int(cursor)==last else 500
        data+=bytes(inferior.read_memory(int(cursor['bytes'].address),n));cursor=cursor['next']
    assert len(data)==int(scope['completed_bytes'])+int(scope['buffered_bytes'])
    relocs=[];block=c['relocations'];last=int(block);cursor=block['next'];seen=set()
    while int(cursor) not in seen:
        seen.add(int(cursor));count=int(cursor['count']);assert count<=50
        for i in range(count):
            r=cursor['entries'][i];relocs.append([int(r['address']),int(r['symbol']),int(r['type'])])
        cursor=cursor['next']
    section=c['section'];rows.append(dict(number=int(c['number']),name=section['name'].string(),data=data.hex(),relocations=relocs))
Path(gdb.parse_and_eval('$c2_capture_path').string()).write_text(json.dumps(rows,indent=2)+'\n')
print('C2_COFF_FIRST_CAPTURED',len(rows))
