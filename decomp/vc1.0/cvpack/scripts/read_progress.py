"""A single reviewed pending public read, after197of209 bytes were copied."""
import hashlib,json,re
from state import BINARY

def validate(config,pe):
    record=config.get('partial_read_observations')
    if not record:return None
    path=BINARY/record['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==record['evidence_sha256'],'stale pending-read proof'
    p=json.loads(path.read_text())
    assert p['site']==0x17e69 and p['frontier']==0x2a5c0 and p['return']==0x2a3dd
    assert p['requested']==209 and p['copied']==197 and p['remaining']==12 and p['module']==3
    assert set(p['sources'])=={'src/files.c','src/modules.c'}
    for name,digest in p['sources'].items():assert hashlib.sha256((BINARY/name).read_bytes()).hexdigest()==digest,'pending-read source changed'
    for start,end,digest in p['instructions']:assert hashlib.sha256(pe.read(start,end-start)).hexdigest()==digest,'pending-read original loop changed'
    assert [(a,b)for a,b,_ in p['instructions']]==[(0x2a350,0x2a3fe),(0x17e20,0x17ea8)]
    assert p['native_line']==next(i for i,line in enumerate((BINARY/'src/files.c').read_text().splitlines(),1)if line.strip()=='advance_file_page(slot);')
    return p

def original(events):
    result=[]
    for line in events.splitlines():
        if not line.startswith('read-progress '):continue
        words=line.split()[1:];assert len(words)==28,'invalid pending-read record'
        names=['site','address','count','copied','remaining','counter','position','slot','caller','readreturn','destination','cursor','module','bytes']
        assert words[::2]==names,'invalid pending-read fields'
        hexkeys={'site','address','slot','caller','readreturn','destination','cursor'}
        r={key:(value if key=='bytes'else int(value,16 if key in hexkeys else 10))for key,value in zip(words[::2],words[1::2])}
        r.update(pending=True,returned=False);result.append(r)
    return result

def compare(config,pe,events,native,frontier,state):
    from reads import original as read_entries
    proof=validate(config,pe);reads=read_entries(events);pending=[r for r in reads if 'returned'not in r]
    old=original(events)
    if not pending:
        assert not old and not native,'extra pending-read proof'
        return []
    assert proof and frontier['original']==proof['frontier'],'unreviewed pending-read frontier'
    assert len(pending)==len(old)==len(native)==1,'missing mandatory actual pending-read proof'
    from objects import original_walks
    walks=original_walks(events,json.loads((BINARY/'walks.json').read_text()))
    a,b,entry=old[0],native[0],pending[0]
    assert not b.get('error'),b
    assert a['address']==walks['symbol_data'][0]['address']and b['address']==state['walks']['symbol_data'][0]['address'],'pending read allocation identity differs'
    assert a['address']==entry['address']and a['site']==entry['site']==proof['site']
    for key,value in dict(count=209,copied=197,remaining=12,counter=209,position=53248,caller=0x2a3dd,readreturn=0x17e6e,module=3,pending=True,returned=False).items():
        assert a[key]==b[key]==value,('pending read state differs',key)
    assert a['position']==entry['entry_position']+197,'pending read page position differs'
    assert b['native_source_line']==proof['native_line'],'pending read native caller differs'
    for r in(a,b):
        assert r['destination']==r['address']and r['address']and r['slot']and r['cursor'],'pending read raw ownership differs'
    original_slot=int(re.search(r'^stack 1 ([0-9a-f]+)$',events,re.M)[1],16)
    assert a['slot']==original_slot and b['slot']==state['parameters']['slot']['address'],'pending read slot owner differs'
    for r,objects in((a,walks),(b,state['walks'])):
        slots=[node for name in('active_files','free_files','closed_files')for node in objects[name]if node['address']==r['slot']]
        assert len(slots)==1,'pending read slot has no unique typed ownership'
        slot=bytes.fromhex(slots[0]['bytes']);page_address=int.from_bytes(slot[32:36],'little')
        pages=[node for name in('used_pages','free_pages')for node in objects[name]if node['address']==page_address]
        assert len(pages)==1,'pending read page has no unique typed ownership'
        page=bytes.fromhex(pages[0]['bytes'])
        assert int.from_bytes(page[12:16],'little')==int.from_bytes(page[8:12],'little')==r['position'],'pending page is not exhausted'
        assert int.from_bytes(page[4120:4124],'little')==r['cursor']==page_address+4120,'pending source cursor differs'
    native_entries=state['pending_read_entries'];assert len(native_entries)==1
    other=native_entries[0]
    assert other['address']==b['address']and other['count']==entry['count']==209 and other['site']==entry['site']
    for r,start in((a,entry),(b,other)):
        before,after=bytes.fromhex(start['prebytes']),bytes.fromhex(r['bytes'])
        assert len(before)==len(after)==209,'pending read buffer extent differs'
        assert before[197:]==after[197:],'pending read changed its own unwritten tail'
    assert bytes.fromhex(a['bytes'])[:197]==bytes.fromhex(b['bytes'])[:197],'pending defined bytes differ'
    for r,node in((a,walks['symbol_data'][0]),(b,state['walks']['symbol_data'][0])):
        assert bytes.fromhex(node['bytes'])[:209]==bytes.fromhex(r['bytes']),'pending observation differs from actual frontier allocation'
    return [dict(site=entry['site'],count=209,copied=197,remaining=12,pending=True,original_address=a['address'],native_address=b['address'],defined_sha256=hashlib.sha256(bytes.fromhex(a['bytes'])[:197]).hexdigest())]
