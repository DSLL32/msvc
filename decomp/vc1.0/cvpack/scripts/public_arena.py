"""One reviewed live symbol block shared by public and global bucket families."""
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
    expected=[(0x19530,0x19573),(0x1b310,0x1b500)]
    if any(r['name']=='intern_global'and r['status']!='stub'for r in config['functions']):expected.append((0x1b500,0x1b6c2))
    assert [(r['start'],r['end'])for r in proof['instructions']]==expected,'symbol arena creation/write proof missing'
    for source,digest in proof['sources'].items():
        assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'public arena source changed'
    for r in proof['instructions']:
        assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'public arena instruction proof changed'
    return proof

def layout(raw,base,available,heads,count,total,families=()):
    """Every used allocation has exactly one family/bucket owner."""
    capacity=8100
    assert len(raw)==capacity and 0<=available<=capacity,'invalid public block extent'
    used=capacity-available
    assert 0<used<=capacity and base and base%4==0,'invalid live public block'
    assert len(families)<=1 and all(g['name']=='global'for g in families),'unreviewed symbol family'
    groups=[dict(name='public',heads=heads,count=count,total=total),*families]
    assert len({r['name']for r in groups})==len(groups),'duplicate symbol hash family'
    nodes={};intervals=[]
    for group in groups:
        heads=group['heads'];assert len(heads)==4096*4,'invalid symbol hash extent'
        owned=0;record_total=0
        for bucket in range(4096):
            pointer=int.from_bytes(heads[bucket*4:bucket*4+4],'little')
            while pointer:
                assert pointer not in nodes,'duplicate or cyclic symbol family/node ownership'
                offset=pointer-base
                assert offset>=0 and offset%4==0 and offset+22<=used,'symbol node outside live block'
                length=int.from_bytes(raw[offset+18:offset+20],'little')
                size=length+22
                assert size%4==0 and offset+size<=used,'symbol node allocation exceeds used block'
                name=int.from_bytes(raw[offset+4:offset+6],'little')
                assert 4<=name<length+2 and name+1+raw[offset+18+name]<=length+2,'symbol name exceeds record'
                value=int.from_bytes(raw[offset+8:offset+12],'little')
                assert value%4096==bucket,'symbol node hash/bucket differs'
                intervals.append((offset,offset+size));nodes[pointer]=dict(offset=offset,size=size,bucket=bucket,family=group['name'])
                owned+=1;record_total+=length+2
                pointer=int.from_bytes(raw[offset:offset+4],'little')
        assert owned==group['count']and record_total==group['total'],'symbol family count/size does not describe owned nodes'
    cursor=0
    for lo,hi in sorted(intervals):
        assert lo==cursor,'unowned or overlapping symbol allocation'
        cursor=hi
    assert cursor==used,'incomplete symbol allocation lifetime'
    return nodes

def comparison(left,right,old,new,context):
    assert context['original_available']==context['native_available'],'public available bytes differ'
    retained=context.get('retained_blocks')
    if retained:
        assert old['address']==retained['original'][-1]['base']and new['address']==retained['native'][-1]['base']
        assert old['bytes']==retained['original'][-1]['bytes']and new['bytes']==retained['native'][-1]['bytes'],'retained/current block dump differs'
        defined=retained['defined'][-1];pointers=retained['pointers'][-1];x=bytearray(len(left));y=bytearray(len(right))
        for lo,hi in defined:x[lo:hi]=left[lo:hi];y[lo:hi]=right[lo:hi]
        left[:]=x;right[:]=y
        return pointers,dict(nodes=retained['nodes'],retained_blocks=2,defined_ranges=defined,original_sha256=hashlib.sha256(bytes.fromhex(old['bytes'])).hexdigest(),native_sha256=hashlib.sha256(bytes.fromhex(new['bytes'])).hexdigest())
    a=layout(left,old['address'],context['original_available'],context['original_heads'],context['count'],context['total'],context.get('original_families',()))
    b=layout(right,new['address'],context['native_available'],context['native_heads'],context['count'],context['total'],context.get('native_families',()))
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
    old_groups=[];new_groups=[]
    if 'global_symbols_count'in data:
        count=word('global_symbols_count');total=word('global_symbols_size')
        assert int.from_bytes(bytes.fromhex(state['globals']['global_symbols_count']),'little')==count and int.from_bytes(bytes.fromhex(state['globals']['global_symbols_size']),'little')==total,'global symbol family scalar state differs'
        assert len(walks['global_symbol_hash'])==len(state['walks']['global_symbol_hash'])==1,'missing global hash family'
        old_groups=[dict(name='global',heads=bytes.fromhex(walks['global_symbol_hash'][0]['bytes']),count=count,total=total)]
        new_groups=[dict(name='global',heads=bytes.fromhex(state['walks']['global_symbol_hash'][0]['bytes']),count=count,total=total)]
    return dict(original_available=available,native_available=actual,original_heads=bytes.fromhex(walks['public_symbol_hash'][0]['bytes']),native_heads=bytes.fromhex(state['walks']['public_symbol_hash'][0]['bytes']),count=word('public_symbols_count'),total=word('public_symbols_size'),original_families=old_groups,native_families=new_groups)
