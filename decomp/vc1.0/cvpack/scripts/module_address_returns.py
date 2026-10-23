"""Actual typed AX result and read-only caller arguments for the module test."""
import hashlib,json,re
from state import BINARY
from recursive_arena import word

def validate(config,pe):
    ref=config.get('module_address_observations');assert ref or not any(r['name']=='address_in_module'and r['status']!='stub'for r in config['functions']),'missing module-address return proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale module-address return proof'
    p=json.loads(path.read_text());assert p['schema']==1 and p['calls']==1 and p['returned']==0x257c5 and p['return_bits']==16 and p['result']==1
    assert p['module']==3 and p['segment']==1 and p['offset']==0x120 and p['input_offset']==4283 and p['record_size']==53 and p['caller']=='convert_procedure32'
    assert p['scope_link_frontier']==dict(original=0x1bd60,calls=[dict(input_offset=4283,record_size=53,offset=0x120),dict(input_offset=4453,record_size=47,offset=0),dict(input_offset=4642,record_size=45,offset=0x1b0)]),'unreviewed additional procedure calls'
    for filename,digest in p['sources'].items():assert hashlib.sha256((BINARY/filename).read_bytes()).hexdigest()==digest,'module-address source changed'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'module-address body/callsite changed'
    assert pe.read(0x257c0,8)==bytes.fromhex('e85b01000083c40c'),'module-address call/three-slot caller cleanup differs'
    # Complete original body only reads explicit memory operands. PUSH writes
    # below ESP; its preserved incoming cdecl argument slots remain unchanged.
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM
    decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    for ins in decoder.disasm(pe.read(0x25920,0x5c),0x25920):
        assert all(op.type!=X86_OP_MEM or not(op.access&CS_AC_WRITE)for op in ins.operands),'module-address success body could alter observed argument slots'
    return p

def parse(events):
    result=[]
    for line in events.splitlines():
        f=line.split()
        if f and f[0]=='module-address-return':
            assert len(f)==8;result.append(dict(result=int(f[1],16),esp=int(f[2],16),module=int(f[3],16),segment=int(f[4],16),offset=int(f[5],16),input=int(f[6],16),record=f[7]))
    return result

def compare(config,pe,events,native,old,new,case):
    p=validate(config,pe)
    if not p:return
    original=parse(events)
    profiles=p['scope_link_frontier']['calls']if re.search(r'^exception 80000003 va (?:0001bd60|0001d840|0001d790) ',events,re.M)else[{k:p[k]for k in('input_offset','record_size','offset')}]
    assert case=='hello-debug-dll'and len(original)==len(native)==len(profiles),'missing module-address return/entry inventory'
    assert events.splitlines().count('module-address-rearm 000257c5 000257c8')==len(profiles),'module-address return probe missing rearm'
    results=[]
    for a,b,profile in zip(original,native,profiles):
        assert not b.get('error')and a['result']&0xffff==b['result']==1,'module-address typed AX result differs'
        assert b['caller']==p['caller'],'module-address native caller differs'
        for row,walks in((a,old),(b,new)):
            module=next(r for r in walks['module_list']if word(bytes.fromhex(r['bytes']),0,2)==3)
            assert row['module']==module['address']and row['segment']&0xffff==1 and row['offset']==profile['offset'],'module-address actual typed arguments differ'
            assert row['input']==walks['type_data'][0]['address']+profile['input_offset'],'module-address source owner differs'
            record=bytes.fromhex(row['record']);assert len(record)==profile['record_size']and word(record,0,2)==len(record)-2 and word(record,2,2)==0x205
            assert word(record,0x20,2)==row['segment']&0xffff and word(record,0x1c)==row['offset'],'module-address arguments do not come from actual procedure record'
        assert a['record']==b['record'],'module-address entry source record differs'
        results.append(dict(module=3,segment=1,offset=profile['offset'],result=1,original_raw_eax=a['result'],original_raw_segment=a['segment'],original=a,native=b))
    return results[0]if len(results)==1 else results
