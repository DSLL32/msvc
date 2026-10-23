"""Actual default-pool wrapper entry, call order, and first member frontier."""
import copy,json,re,struct
from allocation_state import symbols,call
from allocation_imports import pe_read
from provenance import captured_native_binary
ORDER=[8,7,0,1,2,3,4,5,6]

def boundary(original,native,state,sequence,module):
    rows=re.findall(r'^trap seq=(\d+) rva=00012ac3 .*stack=([0-9a-f,]+)$',(original/'events.txt').read_text(),re.M);assert len(rows)==1
    at=int(rows[0][0]);assert at<sequence and int(rows[0][1].split(',')[0],16)==module+0x129d7
    entries=json.loads((native/'default-pools-entry.json').read_text());assert len(entries)==1;entry=copy.deepcopy(entries[0]);entry.update(entry['crt_state'])
    names=symbols(captured_native_binary(native))
    assert entry['symbol']=='initialize_default_pools()' and entry['pc']==names['initialize_default_pools'][0] and entry['boundary_rva']==0x12ac3
    assert entry['args']=={} and entry['formal_inventory']==[] and entry['invocation']==state['invocation']
    assert entry['caller']==dict(symbol='initialize_compiler_arenas',pc=entry['return'],args={})
    call(captured_native_binary(native),entry['return'],entry['pc'])
    return entry,at

def validate(original,native,fields,entry,state,entry_sequence,sequence,module,candidate):
    assert candidate['default_pool_order']==ORDER
    assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])in [('default-pool-arena',0x12b30),('pool-refill',0xaa9c),('arena-page-allocation',0xaacb)]
    binary=captured_native_binary(native);names=symbols(binary)
    first=fields['pools'][8];native_this=names['c1_bss'][0]+first['native_offset'];original_this=module+first['original_object_rva']
    assert first['index']==8 and first['native_size']==16 and first['original_object_rva']==0x54dc0
    assert state['symbol']=='CompilerPool::SetArena' and state['args']==dict(this=native_this,arena_index=0)
    assert state['formal_inventory']==[dict(name='this',type='CompilerPool *'),dict(name='arena_index',type='unsigned int')]
    assert state['caller']['symbol']=='initialize_default_pools' and state['caller']['args']=={}
    entries=json.loads((native/'pool-arena-entries.json').read_text());assert len(entries)==1;actual=entries[0]
    assert actual['pc']==names['_ZN12CompilerPool8SetArenaEj'][0] and actual['ecx']==native_this and actual['arena_index']==0
    assert actual['formal_inventory']==state['formal_inventory'] and actual['return']==state['caller']['pc']
    call(binary,actual['return'],actual['pc'])
    assert actual['compiler_state']==state['compiler_state']==entry['compiler_state']
    for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
        assert(original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
    for key,value in entry['crt_state'].items():assert state[key]==value
    events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=%d rva=00012b30 esp=([0-9a-f]+) eax=[0-9a-f]+ ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(rows)==1
    assert int(rows[0][1],16)==original_this and [int(value,16)for value in rows[0][2].split(',')][:2]==[module+0x12acf,0]
    snapshot=(original/'relocated-text.bin').read_bytes()
    # All nine actual immediate receiver/call blocks establish source order;
    # only the first member call executes before the genuine named trap.
    for index,pool in enumerate(ORDER):
        start=0x12ac3+index*12;receiver=module+fields['pools'][pool]['original_object_rva']
        expected=b'\x6a\0\xb9'+struct.pack('<I',receiver)+b'\xe8'+struct.pack('<i',0x12b30-(start+12))
        assert snapshot[start-0x1000:start+12-0x1000]==expected
    assert snapshot[0x12b2f-0x1000]==0xc3
    # The pending member's real ABI is evidence, not accepted body/extent credit.
    assert pe_read(original/'loaded-target.bin',0x12b46,3)==b'\xc2\x04\0'


def validate_binding(original,native,fields,entry,state,entry_sequence,sequence,module,candidate):
    """Real SetArena entry and its sole pointer store before genuine Refill."""
    assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])in [('pool-refill',0xaa9c),('arena-page-allocation',0xaacb)]
    rows=re.findall(r'^trap seq=(\d+) rva=00012b30 .*stack=([0-9a-f,]+)$',(original/'events.txt').read_text(),re.M);assert len(rows)==1
    at=int(rows[0][0]);assert entry_sequence<at<sequence
    observed=json.loads((native/'pool-arena-entries.json').read_text());assert len(observed)==1
    before=copy.deepcopy(observed[0]);before.update(before['crt_state'])
    assert before['boundary_rva']==0x12b30 and before['invocation']==entry['invocation']==state['invocation']
    validate(original,native,fields,entry,before,entry_sequence,at,module,candidate)
    binary=captured_native_binary(native);names=symbols(binary)
    pool=fields['pools'][8];native_this=names['c1_bss'][0]+pool['native_offset'];original_this=module+pool['original_object_rva']
    assert state['symbol']=='CompilerPool::Refill' and state['args']==dict(this=native_this,minimum_bytes=1)
    assert state['formal_inventory']==[dict(name='this',type='CompilerPool *'),dict(name='minimum_bytes',type='unsigned int')]
    assert state['caller']['symbol']=='CompilerPool::SetArena' and state['caller']['args']==dict(this=native_this,arena_index=0)
    call(binary,state['caller']['pc'],names['_ZN12CompilerPool6RefillEj'][0])
    expected=copy.deepcopy(before['compiler_state']);expected['bss']['pools'][8]['arena']=names['c1_bss'][0]+fields['arenas']['native_offset']
    assert state['compiler_state']==expected
    raw=bytearray((original/('compiler-bss-%03d.bin'%at)).read_bytes())
    offset=pool['original_object_rva']-fields['original_bss_region_rva'][0]+12
    assert struct.unpack_from('<I',raw,offset)[0]==0
    struct.pack_into('<I',raw,offset,module+fields['arenas']['original_rva'])
    assert (original/('compiler-bss-%03d.bin'%sequence)).read_bytes()==bytes(raw)
    for region in ('compiler-data','compiler-snapshot','state'):
        assert(original/('%s-%03d.bin'%(region,at))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
    for key,value in before['crt_state'].items():assert state[key]==value
    rows=re.findall(r'^trap seq=%d rva=0000aa9c esp=([0-9a-f]+) eax=[0-9a-f]+ ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%sequence,(original/'events.txt').read_text(),re.M);assert len(rows)==1
    assert int(rows[0][1],16)==original_this and [int(x,16)for x in rows[0][2].split(',')][:2]==[module+0x12b46,1]
    snapshot=(original/'relocated-text.bin').read_bytes()
    expected_code=b'\x8b\x44\x24\x04\x6a\x01\xc1\xe0\x05\x05'+struct.pack('<I',module+fields['arenas']['original_rva'])+b'\x89\x41\x0c\xe8'+struct.pack('<i',0xaa9c-0x12b46)+b'\xc2\x04\0'
    assert snapshot[0x12b30-0x1000:0x12b49-0x1000]==expected_code


def validate_refill(original,native,fields,entry,state,entry_sequence,sequence,module,candidate):
    """Unsigned page calculation and real owned remaining-count output address."""
    assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])==('arena-page-allocation',0xaacb)
    rows=re.findall(r'^trap seq=(\d+) rva=0000aa9c .*stack=([0-9a-f,]+)$',(original/'events.txt').read_text(),re.M);assert len(rows)==1
    at=int(rows[0][0]);assert entry_sequence<at<sequence
    observed=json.loads((native/'pool-refill-entries.json').read_text());assert len(observed)==1
    before=copy.deepcopy(observed[0]);before.update(before['crt_state'])
    assert before['boundary_rva']==0xaa9c and before['invocation']==entry['invocation']==state['invocation']
    names=symbols(captured_native_binary(native));assert before['pc']==names['_ZN12CompilerPool6RefillEj'][0]
    assert before['ecx']==before['args']['this'] and before['minimum_bytes']==before['args']['minimum_bytes']
    validate_binding(original,native,fields,entry,before,entry_sequence,at,module,candidate)
    binary=captured_native_binary(native);pool=fields['pools'][8]
    native_pool=names['c1_bss'][0]+pool['native_offset'];native_arena=names['c1_bss'][0]+fields['arenas']['native_offset']
    original_pool=module+pool['original_object_rva'];original_arena=module+fields['arenas']['original_rva']
    assert state['symbol']=='CompilerArena::AllocatePages'
    assert pe_read(original/'loaded-target.bin',0xab0a,3)==b'\xc2\x08\0'
    assert state['formal_inventory']==[dict(name='this',type='CompilerArena *'),dict(name='pages',type='unsigned int'),dict(name='available_bytes',type='unsigned int *')]
    page_size=before['compiler_state']['initialized']['system_info']['dwPageSize'];minimum_pages=before['compiler_state']['bss']['pools'][8]['minimum_pages']
    assert page_size>0 and minimum_pages==1 and before['args']['minimum_bytes']==1
    pages=max(((before['args']['minimum_bytes']-1)&0xffffffff)//page_size+1,minimum_pages)
    assert state['args']==dict(this=native_arena,pages=pages,available_bytes=native_pool+4)
    assert state['caller']['symbol']=='CompilerPool::Refill' and state['caller']['args']==dict(this=native_pool,minimum_bytes=1)
    call(binary,state['caller']['pc'],names['_ZN13CompilerArena13AllocatePagesEjPj'][0])
    assert state['compiler_state']==before['compiler_state']
    for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
        assert(original/('%s-%03d.bin'%(region,at))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
    for key,value in before['crt_state'].items():assert state[key]==value
    rows=re.findall(r'^trap seq=%d rva=0000aacb esp=([0-9a-f]+) eax=[0-9a-f]+ ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%sequence,(original/'events.txt').read_text(),re.M);assert len(rows)==1
    assert int(rows[0][1],16)==original_arena and [int(x,16)for x in rows[0][2].split(',')][:3]==[module+0xaac5,pages,original_pool+4]
    source=next(row for row in candidate['functions']if row['name']=='CompilerPool::Refill');assert source['chunks']==[[0xaa9c,0xaacb],[0x2c3f7,0x2c3fe]]
    snapshot=(original/'relocated-text.bin').read_bytes()
    assert snapshot[0xaaa4-0x1000:0xaaaa-0x1000]==b'\xf7\x35'+struct.pack('<I',module+0x5f50c)
    assert snapshot[0xaac0-0x1000:0xaac5-0x1000]==b'\xe8'+struct.pack('<i',0xaacb-0xaac5)
    assert snapshot[0xaac5-0x1000:0xaacb-0x1000]==b'\x89\x06\x5e\xc2\x04\0'
    assert snapshot[0x2c3f7-0x1000:0x2c3fe-0x1000]==b'\x8b\xc1\xe9'+struct.pack('<i',0xaab8-0x2c3fe)
