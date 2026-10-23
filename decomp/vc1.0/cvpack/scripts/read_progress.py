"""Two separately reviewed pending reads, with exact owned write prefixes."""
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
    assert [(a,b)for a,b,_ in p['instructions']]==[(0x2a350,0x2a3fe),(0x17e20,0x17ea8),(0x2a5c0,0x2a606),(0x17d80,0x17e12)]
    assert p['native_line']==next(i for i,line in enumerate((BINARY/'src/files.c').read_text().splitlines(),1)if line.strip()=='advance_file_page(slot);')
    t=p['type_read'];assert {k:t[k]for k in('site','owner','requested','copied','remaining','position','frontier','inner_return')}==dict(site=0x17dc9,owner='type_data',requested=4746,copied=3076,remaining=1670,position=45056,frontier=0x2a850,inner_return=0x2a5ef)
    return p

def original(events):
    result=[]
    for line in events.splitlines():
        if not line.startswith('read-progress '):continue
        words=line.split()[1:];assert len(words)in(28,34),'invalid pending-read record'
        names=['site','address','count','copied','remaining','counter','position','slot','caller','readreturn','destination','cursor','module','bytes']
        if len(words)==34:names=names[:-1]+['frontier','innerreturn','shift','bytes']
        assert words[::2]==names,'invalid pending-read fields'
        hexkeys={'site','address','slot','caller','readreturn','destination','cursor','frontier','innerreturn'}
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
    assert proof,'unreviewed pending-read policy'
    assert len(pending)==len(old)==len(native)==1,'missing mandatory actual pending-read proof'
    from objects import original_walks
    walks=original_walks(events,json.loads((BINARY/'walks.json').read_text()))
    a,b,entry=old[0],native[0],pending[0]
    typed=entry['site']==0x17dc9
    assert entry['site']in(0x17e69,0x17dc9),'unreviewed pending read site'
    count,copied,remaining,position,owner,readreturn=(4746,3076,1670,45056,'type_data',0x17dce)if typed else(209,197,12,53248,'symbol_data',0x17e6e)
    assert frontier['original']in((0x2a850,)if typed else(0x2a5c0,0x2a120)),'unreviewed pending-read frontier'
    assert not b.get('error'),b
    assert a['address']==walks[owner][0]['address']and b['address']==state['walks'][owner][0]['address'],'pending read allocation identity differs'
    assert a['address']==entry['address']and a['site']==b['site']==entry['site']==(0x17dc9 if typed else proof['site'])
    for key,value in dict(count=count,copied=copied,remaining=remaining,counter=count,position=position,caller=0x2a3dd,readreturn=readreturn,module=3,pending=True,returned=False).items():
        assert a[key]==b[key]==value,('pending read state differs',key)
    assert a['position']==entry['entry_position']+copied,'pending read page position differs'
    assert b['native_source_line']==proof['native_line'],'pending read native caller differs'
    for r in(a,b):
        assert r['destination']==r['address']and r['address']and r['slot']and r['cursor'],'pending read raw ownership differs'
    nested=frontier['original']in(0x2a120,0x2a850)
    for item in(a,b):
        assert item.get('frontier',0x2a5c0)==frontier['original']and item.get('innerreturn',0)==(0x2a5ef if typed else 0x2a606 if nested else 0)and item.get('shift',0)==(5 if nested else 0),'pending nested frame differs'
    expected=[('release_page'if typed else 'flush_file_page'),'advance_file_page','read_buffered_file','read_file',('read_types'if typed else 'read_publics')]if nested else ['advance_file_page','read_buffered_file','read_file','read_publics']
    assert state['frames'][:len(expected)]==expected,'pending read owning frame chain differs'
    if nested:assert b['nested_source_line']==(proof['type_read']['native_nested_line']if typed else proof['nested_native_line']),'pending nested native caller differs'
    stack={int(index):int(value,16)for index,value in re.findall(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
    registers=dict((name,int(value,16))for name,value in re.findall(r'(eax|ebx|ecx|edx|esi|edi|ebp|esp|eip) ([0-9a-f]+)',events))
    shift=5 if nested else 0
    assert stack[shift]==a['caller']and stack[shift+3]==a['counter']and stack[shift+9]==a['destination']and stack[shift+10]==a['count'],'pending original outer loop frame differs'
    assert stack[shift+11]==a['readreturn']and stack[shift+12]==entry['handle']and stack[shift+13]==a['address']and stack[shift+14]==a['count'],'pending original read caller frame differs'
    assert registers['ebp']==a['copied'],'pending original actual copied register differs'
    if nested:
        assert stack[0]==a['innerreturn']and stack[2]==a['address']+copied and stack[3]==a['cursor']and stack[4]==a['slot'],'pending original saved registers/nested frame differ'
    else:assert registers['edi']==a['address']+copied and registers['esi']==a['cursor']and registers['ebx']==a['slot'],'pending original actual copy registers differ'
    original_slot=stack[shift+1]
    assert a['slot']==original_slot,'pending original slot owner differs'
    if not nested:assert b['slot']==state['parameters']['slot']['address'],'pending native slot owner differs'
    transitions=[]
    for r,objects in((a,walks),(b,state['walks'])):
        slots=[node for name in('active_files','free_files','closed_files')for node in objects[name]if node['address']==r['slot']]
        assert len(slots)==1,'pending read slot has no unique typed ownership'
        slot=bytes.fromhex(slots[0]['bytes']);page_address=int.from_bytes(slot[32:36],'little')
        if nested:
            actual=int(re.search(r'^stack 1 ([0-9a-f]+)$',events,re.M)[1],16)if r is a else state['parameters']['page']['address']
            assert actual==page_address,'pending nested page owner differs'
        pages=[node for name in('used_pages','free_pages')for node in objects[name]if node['address']==page_address]
        assert len(pages)==1,'pending read page has no unique typed ownership'
        page=bytes.fromhex(pages[0]['bytes'])
        assert int.from_bytes(page[12:16],'little')==int.from_bytes(page[8:12],'little')==r['position'],'pending page is not exhausted'
        assert int.from_bytes(page[4120:4124],'little')==r['cursor']==page_address+4120,'pending source cursor differs'
        if typed:
            start=entry['page_entry']if r is a else state['pending_read_entries'][0]['page_entry']
            assert start['slot']==r['slot']and start['page']==page_address and start['address']==r['address'],'Page entry does not own pending read'
            before,after=start['flags'],int.from_bytes(page[20:24],'little')
            assert before&16 and not(before&8)and after==before&~16,'actual Page flags16 clear differs'
            assert start['position']+copied==r['position'],'Page flags observation has different read extent'
            assert page_address+4136<=r['address']or r['address']+count<=page_address,'Page flags alias the scratch allocation'
            transitions.append(dict(original=r is a,before=before,after=after,page=page_address))
    if typed:assert (transitions[0]['before'],transitions[0]['after'])==(transitions[1]['before'],transitions[1]['after']),'actual full Page flag states differ'
    native_entries=state['pending_read_entries'];assert len(native_entries)==1
    other=native_entries[0]
    assert other['address']==b['address']and other['count']==entry['count']==count and other['site']==entry['site']
    for r,start in((a,entry),(b,other)):
        before,after=bytes.fromhex(start['prebytes']),bytes.fromhex(r['bytes'])
        assert len(before)==len(after)==count,'pending read buffer extent differs'
        assert before[copied:]==after[copied:],'pending read changed its own unwritten tail'
    assert bytes.fromhex(a['bytes'])[:copied]==bytes.fromhex(b['bytes'])[:copied],'pending defined bytes differ'
    for r,node in((a,walks[owner][0]),(b,state['walks'][owner][0])):
        assert bytes.fromhex(node['bytes'])[:count]==bytes.fromhex(r['bytes']),'pending observation differs from actual frontier allocation'
    return [dict(site=entry['site'],count=count,copied=copied,remaining=remaining,counter_before_subtraction=count,logical_remaining_after_copy=remaining,page_flag_transitions=transitions,pending=True,original_address=a['address'],native_address=b['address'],defined_sha256=hashlib.sha256(bytes.fromhex(a['bytes'])[:copied]).hexdigest())]
