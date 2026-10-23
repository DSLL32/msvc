"""Reviewed single live symbol block, with contextual interior node ownership."""
import hashlib,json
from state import BINARY

def validate(config,pe):
    item=config.get('public_arena_observations')
    if not item:return None
    path=BINARY/item['evidence']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==item['evidence_sha256'],'changed public arena proof'
    proof=json.loads(path.read_text())
    assert proof['capacity']==8100 and proof['reserved']==[[6,8]],'unreviewed public arena layout'
    assert set(proof['sources'])=={'src/modules.c','src/image.h'},'public arena source ownership missing'
    assert [(r['start'],r['end'])for r in proof['instructions']]==[(0x19530,0x19573),(0x1b310,0x1b500)],'public arena creation/write proof missing'
    for source,digest in proof['sources'].items():
        assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'public arena source changed'
    for r in proof['instructions']:
        assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'public arena instruction proof changed'
    return proof

def layout(raw,base,available,heads,count,total):
    """Every used allocation must be a complete, uniquely bucket-owned node."""
    capacity=8100
    assert len(raw)==capacity and 0<=available<=capacity,'invalid public block extent'
    used=capacity-available
    assert 0<used<=capacity and base and base%4==0,'invalid live public block'
    assert len(heads)==4096*4,'invalid public hash extent'
    nodes={};intervals=[];record_total=0
    for bucket in range(4096):
        pointer=int.from_bytes(heads[bucket*4:bucket*4+4],'little')
        while pointer:
            assert pointer not in nodes,'duplicate or cyclic public node ownership'
            offset=pointer-base
            assert offset>=0 and offset%4==0 and offset+22<=used,'public node outside live block'
            length=int.from_bytes(raw[offset+18:offset+20],'little')
            size=length+22
            assert size%4==0 and offset+size<=used,'public node allocation exceeds used block'
            name=int.from_bytes(raw[offset+4:offset+6],'little')
            assert 4<=name<length+2 and name+1+raw[offset+18+name]<=length+2,'public name exceeds record'
            value=int.from_bytes(raw[offset+8:offset+12],'little')
            assert value%4096==bucket,'public node hash/bucket differs'
            intervals.append((offset,offset+size));nodes[pointer]=dict(offset=offset,size=size,bucket=bucket)
            record_total+=length+2
            pointer=int.from_bytes(raw[offset:offset+4],'little')
    assert len(nodes)==count and record_total==total,'public count/size does not describe owned nodes'
    cursor=0
    for lo,hi in sorted(intervals):
        assert lo==cursor,'unowned or overlapping public allocation'
        cursor=hi
    assert cursor==used,'incomplete public allocation lifetime'
    return nodes

def comparison(left,right,old,new,context):
    assert context['original_available']==context['native_available'],'public available bytes differ'
    a=layout(left,old['address'],context['original_available'],context['original_heads'],context['count'],context['total'])
    b=layout(right,new['address'],context['native_available'],context['native_heads'],context['count'],context['total'])
    assert sorted(a.values(),key=lambda n:n['offset'])==sorted(b.values(),key=lambda n:n['offset']),'public node ownership differs'
    defined=[];pointers=[]
    for node in sorted(a.values(),key=lambda n:n['offset']):
        lo=node['offset'];hi=lo+node['size']
        defined.extend([[lo,lo+6],[lo+8,hi-2]])
        pointers.append(lo)
    # Preserve raw bytes in both receipts. Only reserved/tail/unallocated bytes
    # lack equality credit; every linked header and complete record is compared.
    mask_left,mask_right=bytearray(len(left)),bytearray(len(right))
    for lo,hi in defined:
        mask_left[lo:hi]=left[lo:hi];mask_right[lo:hi]=right[lo:hi]
    left[:]=mask_left;right[:]=mask_right
    return pointers,dict(nodes=len(a),defined_ranges=defined,original_sha256=hashlib.sha256(bytes.fromhex(old['bytes'])).hexdigest(),native_sha256=hashlib.sha256(bytes.fromhex(new['bytes'])).hexdigest())

def context(config,pe,events,state):
    proof=validate(config,pe)
    if not proof:return None
    # Matching contextual scalar globals are also checked independently by accept.
    data={r['name']:r for r in json.loads((BINARY/'data.json').read_text())}
    memory={}
    for line in events.splitlines():
        if line.startswith('memory '):
            _,address,raw=line.split();memory.update((int(address,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    def word(name):
        p=data[name]['original'];return int.from_bytes(bytes(memory[p+i]for i in range(4)),'little')
    from objects import original_walks
    walks=original_walks(events,json.loads((BINARY/'walks.json').read_text()))
    if not walks['public_symbol_cursor']:
        assert not word('public_symbol_cursor') and not word('public_symbol_available')
        assert not state['walks']['public_symbol_cursor'] and state['globals']['public_symbol_available']=='00000000'
        return None
    old=walks['public_symbol_cursor'][0];new=state['walks']['public_symbol_cursor'][0]
    available=word('public_symbol_available');actual=int.from_bytes(bytes.fromhex(state['globals']['public_symbol_available']),'little')
    assert 0<=available<=8100 and actual==available
    assert word('public_symbol_cursor')==old['address']+8100-available,'original public cursor ownership differs'
    assert state['globals']['public_symbol_cursor']['address']==new['address']+8100-actual,'native public cursor ownership differs'
    return dict(original_available=available,native_available=actual,original_heads=bytes.fromhex(walks['public_symbol_hash'][0]['bytes']),native_heads=bytes.fromhex(state['walks']['public_symbol_hash'][0]['bytes']),count=word('public_symbols_count'),total=word('public_symbols_size'))
