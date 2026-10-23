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
    assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])==('default-pool-arena',0x12b30)
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
