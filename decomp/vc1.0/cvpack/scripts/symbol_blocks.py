"""Two reached8100-byte symbol blocks, retained by public/global buckets."""
import hashlib,json
from state import BINARY
from objects import namespace
from recursive_arena import word

def validate(config,pe,rebuilt=None,linked=None):
    ref=config.get('symbol_block_observations')
    assert ref or not any(r['name']=='publish_packed_type'and r['status']!='stub'for r in config['functions']),'missing retained symbol block lifetime proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale retained symbol block evidence'
    p=json.loads(path.read_text());assert p['schema']==1 and p['capacity']==8100 and p['maximum_blocks']==2 and p['frontier']==0x257a0
    assert p['allocation_return']==0x1954f and p['families']==['intern_public','intern_global']and p['first_requests']==[52,40]and p['previous_available']==[0,12]
    assert p['reserved']==[[6,8]]and p['record_tail']==2 and p['ownership']=='whole actual blocks; nodes are contained fields'
    for filename,digest in p['sources'].items():assert hashlib.sha256((BINARY/filename).read_bytes()).hexdigest()==digest,'symbol block source changed'
    for filename in('data.json','walks.json'):assert hashlib.sha256((BINARY/filename).read_bytes()).hexdigest()==p[filename+'_sha256'],'symbol block registry changed'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'symbol block constructor bytes changed'
    lines=(BINARY/'src/modules.c').read_text().splitlines();line=p['native_allocation_line'];assert lines[line-2].strip()=='result = checked_alloc(0x1fa4);'and lines[line-1].strip()=='public_symbol_available = 0x1fa4;','symbol block allocation site differs'
    if 'address_frontier'in p:assert p['address_frontier']==0x25920,'unreviewed retained-symbol frontier'
    if rebuilt is not None:p['compiled']=compiled_provenance(rebuilt,linked)
    return p

def compiled_provenance(pe,linked):
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    start=linked['_allocate_symbol'];end=min(v for v in linked.values()if v>start);raw=pe.read(start,end-start);code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(raw,start));calls=[(i,r)for i,r in enumerate(code)if r.mnemonic=='call'];assert len(calls)==1 and calls[0][1].op_str=='0x%x'%linked['_checked_alloc']
    i,call=calls[0];assert code[i-1].bytes==bytes.fromhex('68a41f0000')and bytes.fromhex('816508ffff0000')in raw,'symbol block extent/truncatedrequest differs'
    tail=pe.read(call.address+5,19);slot=tail[5:6];available=linked['_public_symbol_available'].to_bytes(4,'little');cursor=linked['_public_symbol_cursor'].to_bytes(4,'little')
    assert tail[:16]==b'\x83\xc4\x04\x89\x45'+slot+b'\xc7\x05'+available+b'\xa4\x1f\x00\x00','symbol block malloc result/available reset differs'
    pattern=b'\x8b\x45\x08\x03\x45'+slot+b'\xa3'+cursor+b'\x8b\x45'+slot
    assert raw.count(pattern)==1,'symbol block size+same allocation->cursor/return differs'
    assert b'\x29\x05'+available in raw,'symbol block available decrement differs'
    return dict(call=call.address,size=8100,result_slot=int.from_bytes(slot,'little',signed=True),publication=pattern.hex(),rebuilt_sha256=hashlib.sha256(pe.data).hexdigest())

def parse(events):
    rows=[];frontier={}
    for line in events.splitlines():
        f=line.split()
        if f and f[0]=='symbol-block-allocation':
            assert len(f)==8 and int(f[1])==len(rows);rows.append(dict(base=int(f[2],16),size=int(f[3]),family={0x1b3e5:'intern_public',0x1b5cf:'intern_global'}[int(f[4],16)],request=int(f[5]),previous_cursor=int(f[6],16),previous_available=int(f[7])))
        elif f and f[0]=='symbol-block-bytes':
            assert len(f)==5;index=int(f[2]);assert index<len(rows)and rows[index]['base']==int(f[3],16)
            if f[1]=='birth':assert 'before'not in rows[index];rows[index]['before']=f[4]
            else:assert f[1]=='frontier'and index not in frontier;frontier[index]=f[4]
    return dict(allocations=rows,frontier=frontier)

def layout(blocks,groups):
    """Each node belongs to one actual retained block and one bucket family."""
    seen={};intervals=[[]for r in blocks]
    for group in groups:
        heads=group['heads'];assert len(heads)==16384;count=0;total=0
        for bucket in range(4096):
            pointer=word(heads,bucket*4)
            while pointer:
                assert pointer not in seen,'duplicate/cyclic symbol family node'
                matches=[(i,pointer-r['base'])for i,r in enumerate(blocks)if r['base']<=pointer<r['base']+r['used']];assert len(matches)==1,'symbol family node lacks live actual block'
                index,at=matches[0];block=blocks[index];raw=bytes.fromhex(block['bytes']);assert at%4==0 and at+22<=block['used']
                length=word(raw,at+18,2);size=length+22;name=word(raw,at+4,2);hash=word(raw,at+8)
                assert size%4==0 and at+size<=block['used']and 4<=name<length+2 and name+1+raw[at+18+name]<=length+2,'symbol node exceeds exact used allocation'
                assert hash%4096==bucket,'symbol node family/hash bucket differs'
                seen[pointer]=dict(block=index,offset=at,size=size,bucket=bucket,family=group['name']);intervals[index].append((at,at+size));count+=1;total+=length+2;pointer=word(raw,at)
        assert count==group['count']and total==group['total'],'symbol family count/size differs'
    for block,spans in zip(blocks,intervals):
        cursor=0
        for lo,hi in sorted(spans):assert lo==cursor,'unowned/overlapping retained symbol block prefix';cursor=hi
        assert cursor==block['used'],'incomplete retained symbol block lifetime'
    return seen

def context(config,pe,events,native,walks,state,arena):
    p=validate(config,pe)
    if not native:
        assert state['frames'][0]not in('convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block'),'missing mandatory retained symbol block observations'
        return
    assert p and arena and not native.get('error')and native['frontier']in('convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block'),'unsupported symbol block observation'
    if native['frontier']=='address_in_module':assert p['address_frontier']==0x25920,'missing retained-symbol module-address scope'
    if native['frontier']=='convert_bp_relative32':assert p['bp_frontier']==0x254f0,'missing BP-relative observed scope'
    original=parse(events);assert len(original['allocations'])==len(native['allocations'])==len(original['frontier'])==2,'missing retained symbol block lifetimes'
    assert events.splitlines().count('symbol-block-rearm 0001954f 00019552')==2,'symbol block birth probe not rearmed each time'
    old_blocks=[];new_blocks=[]
    for side,output in((original,old_blocks),(native,new_blocks)):
        for i,a in enumerate(side['allocations']):
            before=bytes.fromhex(a['before']);after=original['frontier'][i]if side is original else a['after'];assert a['base']and a['base']%4==0 and a['size']==len(before)==len(bytes.fromhex(after))==8100
            assert a['family']==p['families'][i]and a['request']==p['first_requests'][i]and a['previous_available']==p['previous_available'][i]and a['previous_available']<a['request'],'symbol block birth request/family differs'
            assert a['previous_cursor']==(0 if i==0 else side['allocations'][i-1]['base']+8100-a['previous_available']),'retained symbol block predecessor/cursor differs'
            available=side['allocations'][i+1]['previous_available']if i==0 else arena['original_available'if side is original else 'native_available']
            used=8100-available;assert 0<used<=8100
            assert bytes.fromhex(after)[used:]==before[used:],'unallocated retained symbol tail changed'
            output.append(dict(base=a['base'],used=used,bytes=after,before=a['before']))
        assert output[0]['base']+8100<=output[1]['base']or output[1]['base']+8100<=output[0]['base'],'retained symbol block allocations overlap'
    assert old_blocks[1]['base']==walks['public_symbol_cursor'][0]['address']and new_blocks[1]['base']==state['walks']['public_symbol_cursor'][0]['address'],'current symbol block identity differs'
    old_groups=[dict(name='public',heads=arena['original_heads'],count=arena['count'],total=arena['total']),*arena['original_families']];new_groups=[dict(name='public',heads=arena['native_heads'],count=arena['count'],total=arena['total']),*arena['native_families']]
    left,right=layout(old_blocks,old_groups),layout(new_blocks,new_groups);assert sorted(left.values(),key=lambda r:(r['block'],r['offset']))==sorted(right.values(),key=lambda r:(r['block'],r['offset'])),'retained symbol block topology differs'
    def logical(pointer,blocks):
        if not pointer:return None
        matches=[(i,pointer-r['base'])for i,r in enumerate(blocks)if r['base']<=pointer<r['base']+r['used']];assert len(matches)==1;return matches[0]
    defined=[[],[]];pointers=[[],[]]
    for node in left.values():
        i=node['block'];lo=node['offset'];hi=lo+node['size'];defined[i].extend([[lo,lo+6],[lo+8,hi-2]]);pointers[i].append(lo)
        for block in(old_blocks[i],new_blocks[i]):
            raw=bytes.fromhex(block['bytes']);birth=bytes.fromhex(block['before'])
            assert raw[lo+6:lo+8]==birth[lo+6:lo+8]and raw[hi-2:hi]==birth[hi-2:hi],'symbol node reserved/tail bytes changed without definition'
    for i,(a,b)in enumerate(zip(old_blocks,new_blocks)):
        x=bytearray.fromhex(a['bytes']);y=bytearray.fromhex(b['bytes'])
        for lo in pointers[i]:assert logical(word(x,lo),old_blocks)==logical(word(y,lo),new_blocks),'retained symbol next pointer differs';x[lo:lo+4]=y[lo:lo+4]=b'\0'*4
        for lo,hi in defined[i]:assert x[lo:hi]==y[lo:hi],'retained symbol defined bytes differ'
        for source,block,owners in((a,old_blocks,walks),(b,new_blocks,state['walks'])):
            assert all(not(source['base']<address+size and address<source['base']+8100)for address,size,identity in namespace(owners)if identity!=('public_symbol_cursor',0)),'retained block overlaps another actual owner'
    arena['retained_blocks']=dict(original=old_blocks,native=new_blocks,defined=defined,pointers=pointers,nodes=len(left))
    return dict(original_namespace={'previous_symbol_block_0':[dict(address=old_blocks[0]['base'],bytes=old_blocks[0]['bytes'])]},native_namespace={'previous_symbol_block_0':[dict(address=new_blocks[0]['base'],bytes=new_blocks[0]['bytes'])]},blocks=2,nodes=len(left),defined=defined,original=original,native=native)

def check_disjoint(proof,old_extra,new_extra):
    """Retained blocks cannot alias any newly observed allocation family."""
    for side,extra in(('original',old_extra),('native',new_extra)):
        for i,a in enumerate(proof[side]['allocations']):
            for address,size,identity in namespace(extra):
                if i==0 and identity==('previous_symbol_block_0',0):continue
                assert not(a['base']<address+size and address<a['base']+8100),'retained symbol block overlaps another observed allocation'
