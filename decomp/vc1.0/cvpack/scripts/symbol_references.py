"""Reviewed97 contained constructor writes and actual invocation-bound returns."""
import hashlib,json,re
from state import BINARY
from recursive_arena import word

def validate(config,pe,rebuilt=None,linked=None):
    ref=config.get('symbol_reference_observations');assert ref,'missing reference completion proof'
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale reference completion proof'
    p=json.loads(path.read_text());assert p['schema']==1 and p['function']=='create_symbol_reference'and p['case']=='hello-debug-dll'
    assert p['classification']=='reviewed97-constructor-completion-plus-independent-entry-progression'
    assert (p['size'],p['slab'],p['slab_extent'],p['initial_used'])==(36,1,8100,1500)
    assert p['defined']==[[0,6],[8,32]]and p['reserved']==[[6,8],[32,36]]
    assert p['return_site']==0x1d943 and p['reservation_site']==0x1d7d3
    assert p['profiles']==[dict(module=3,offset=108,record_size=56,source_address=0x120,bucket=0xdcf,hash=0xcb386dcf),dict(module=3,offset=296,record_size=48,source_address=0,bucket=0x2b8,hash=0xd851d2b8),dict(module=3,offset=496,record_size=48,source_address=0x1b0,bucket=0x544,hash=0x9a8d8544)],'unreviewed reservation inputs'
    for name,digest in p['sources'].items():assert hashlib.sha256((BINARY/name).read_bytes()).hexdigest()==digest,'reference source changed'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'reference original caller/body changed'
    lines=(BINARY/'src/types.c').read_text().splitlines();line=p['native_allocation_line'];assert lines[line-2].strip()=='node = allocate_symbol(36);'and lines[line-1].strip()=='node->hash = hash;','reference native reservation site changed'
    if rebuilt is not None:
        from capstone import Cs,CS_ARCH_X86,CS_MODE_32
        start=linked['_create_symbol_reference'];end=min(v for v in linked.values()if v>start);code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(rebuilt.read(start,end-start),start));calls=[(i,r)for i,r in enumerate(code)if r.mnemonic=='call']
        assert len(calls)==2 and [r.op_str for i,r in calls]==['0x%x'%linked['_hash_public'],'0x%x'%linked['_allocate_symbol']],'reference constructor callees differ'
        assert code[calls[1][0]-1].bytes==bytes.fromhex('6a24'),'reference reservation is not exactly36'
        assert code[calls[0][0]-7].bytes==bytes.fromhex('6800100000')or any(r.bytes==bytes.fromhex('6800100000')for r in code[:calls[0][0]]),'reference hash modulus differs'
    return p

def parse(events):
    rows=[]
    for line in events.splitlines():
        f=line.split()
        if not f:continue
        if f[0]=='symbol-reference-reservation':
            assert len(f)==16,'reference reservation raw fields omitted'
            keys=('sequence','base','esp','raw_bucket','record','buckets','module','offset','kind','hash','name_offset','segment','address','head_before')
            row={k:int(v,10 if k=='sequence'else 16)for k,v in zip(keys,f[1:-1])};assert row['sequence']==len(rows);row['bucket']=row['raw_bucket']&0xffff;row['source']=f[-1];rows.append(row)
        elif f[0]=='symbol-reference-bytes':
            assert len(f)==5;row=rows[int(f[2])];assert row['base']==int(f[3],16)
            key={'birth':'before','return':'after','frontier':'frontier_bytes'}[f[1]];assert key not in row;row[key]=f[4]
        elif f[0]=='symbol-reference-reserved-state':
            assert len(f)==4;row=rows[int(f[1])];assert 'available'not in row;row.update(available=int(f[2]),cursor=int(f[3],16))
        elif f[0]=='symbol-reference-published':
            assert len(f)==7;row=rows[int(f[1])];assert 'published'not in row and row['base']==int(f[3],16);row.update(return_esp=int(f[2],16),published=int(f[4],16),return_available=int(f[5]),return_cursor=int(f[6],16))
    return rows

def compare(config,pe,events,native,old,new,state,native_blocks,symbols):
    p=validate(config,pe);original=parse(events)
    assert len(original)==len(native)==3 and events.splitlines().count('observed-reference-completion 0001d943 calls 3')==1,'missing/extra actual constructor completion inventory'
    assert events.splitlines().count('symbol-reference-rearm 0001d7d3 0001d7d7')==3 and events.splitlines().count('symbol-reference-rearm 0001d943 0001d946')==2,'reference permanent rearm inventory differs'
    from symbol_blocks import parse as block_parse
    blocks=block_parse(events);assert len(blocks['allocations'])==2
    assert state['frames'][0]=='record_symbol_offsets'and state['parameters']['module']==3 and state['parameters']['size']==608,'native completion caller/formals differ'
    native_blocks=native_blocks['allocations'];assert len(native_blocks)==2
    result=[]
    for i,(a,b,profile)in enumerate(zip(original,native,p['profiles'])):
        assert not b.get('error')and a['sequence']==b['sequence']==i,'reference observation error/order'
        assert b['return_pc']==b['caller_pc']and b['return_fp']==b['caller_fp']and b['thread']>0,'native return belongs to stale invocation/frame'
        assert b['source_line']==p['native_allocation_line'],'native reservation callsite changed'
        call=bytes.fromhex(b['caller_call']);assert len(call)==5 and call[0]==0xe8 and b['caller_pc']+int.from_bytes(call[1:],'little',signed=True)==b['constructor_address'],'native completion is not the actual constructor CALL return'
        caller_line=next(i for i,line in enumerate((BINARY/'src/types.c').read_text().splitlines(),1)if line.strip()=='create_symbol_reference(record, global_symbol_hash, module, offset, 0x400);')
        assert b['caller_line']==b['return_line']==caller_line,'native completion caller source location differs'
        assert a['return_esp']==a['esp']+24,'original constructor ABI return/argument frame differs'
        for row,walks,block in((a,old,blocks['allocations'][1]),(b,new,native_blocks[1])):
            base=block['base'];assert row['base']==base+1500+36*i and row['cursor']==row['base']+36 and row['available']==6600-36*(i+1),'reference reservation lies outside actual slab/cursor'
            assert row['module']==3 and row['offset']==profile['offset']and row['kind']==0x400 and row['buckets']==walks['global_symbol_hash'][0]['address'],'reference five ABI inputs/owner differ'
            assert row['bucket']==profile['bucket']and row['hash']==profile['hash']and row['name_offset']==37 and row['segment']==1 and row['address']==profile['source_address'],'reference actual defined hash outputs differ'
            source=bytes.fromhex(row['source']);assert len(source)==profile['record_size']and word(source,0,2)+2==len(source)and word(source,2,2)==0x205 and word(source,28)==profile['source_address']and word(source,32,2)==1,'reference source record shape/inputs differ'
            assert row['head_before']==0 and row['published']==row['base'],'reference exact old/new bucket publication differs'
            before=bytes.fromhex(row['before']);after=bytes.fromhex(row['after']);assert len(before)==len(after)==36
            begin=1500+36*i;assert before==bytes.fromhex(block['before'])[begin:begin+36],'reference reservation prebytes disagree with actual unallocated slab tail'
            final=blocks['frontier'][1]if row is a else block['after'];assert after==bytes.fromhex(final)[begin:begin+36],'reference return bytes disagree with actual slab contents'
            heads=bytes.fromhex(walks['global_symbol_hash'][0]['bytes']);assert word(heads,row['bucket']*4)==row['base'],'reference publication does not own actual global bucket'
            for lo,hi in p['reserved']:assert after[lo:hi]==before[lo:hi],'reference reserved bytes changed'
            expected=bytearray(before);expected[0:4]=row['head_before'].to_bytes(4,'little');expected[4:6]=(37).to_bytes(2,'little');expected[8:12]=row['hash'].to_bytes(4,'little');expected[12:16]=row['address'].to_bytes(4,'little');expected[16:18]=(1).to_bytes(2,'little')
            expected[18:20]=(14).to_bytes(2,'little');expected[20:22]=(0x400).to_bytes(2,'little');expected[22:26]=row['hash'].to_bytes(4,'little');expected[26:30]=row['offset'].to_bytes(4,'little');expected[30:32]=(3).to_bytes(2,'little')
            # Module ends at node32: payload padding32..34 and extra allocator
            # tail34..36 retain their OWN before bytes, without equality credit.
            assert after==expected,'reference whole constructor write footprint differs'
        for row,owner in((a,symbols['original']),(b,symbols['native'])):
            payload=symbols['original_output']['bytes']if row is a else symbols['native']['postbytes']
            assert row['record']==owner['base']+profile['offset']and row['source']==bytes.fromhex(payload)[profile['offset']:profile['offset']+profile['record_size']].hex(),'reference source lacks actual packed allocation/record ownership'
        assert a['source']==b['source'],'reference full actual input differs'
        assert a['frontier_bytes']==a['after'],'reference changed after observed return'
        assert a['return_available']==a['available']and a['return_cursor']==a['cursor'],'original allocation state changed inside constructor'
        assert b['after']==a['after']or all(bytes.fromhex(a['after'])[lo:hi]==bytes.fromhex(b['after'])[lo:hi]for lo,hi in p['defined']),'reference defined fields differ'
        result.append(dict(sequence=i,original_base=a['base'],native_base=b['base'],module=3,offset=profile['offset'],bucket=profile['bucket'],size=36,original=a,native=b))
    return dict(records=result,original_bases=[r['base']for r in original],native_bases=[r['base']for r in native],classification=p['classification'],unobserved_later_heap=True)
