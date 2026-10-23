"""All six genuine arena call/return inventories and complete reached state."""
import copy,json,re,struct
from allocation_state import symbols,call
from allocation_imports import export_rva,pe_read
from provenance import captured_native_binary,sha
from scaler_state import compare,semantic,original_fp
from arena_state import RECORDS

RESET_PLAN=dict(return_rva=0x12a70,member_return_rvas=[0x12a0f,0x129bc],auxiliary='restore-only typed nonzero Reset inputs for all six arenas; owned reserve interior pointers, never dereferenced',cursor_offset=0x1000,limit_offset=0x2000,committed_bytes_base=117,reset_value_base=37)

def reset_changes(state,index,base,seed):
    result=copy.deepcopy(state);row=result['bss']['arenas'][index]
    row.update(cursor=base+0x1000 if seed else base,committed_limit=base+0x2000 if seed else base,committed_bytes=117+index if seed else 0,reset_value=37+index if seed else 0)
    return result

def reset_proof(before,entry,returned,index,receiver,seed,pc):
    base=before['bss']['arenas'][index]['base'];assert base
    assert entry['index']==returned['index']==index and entry['ecx']==receiver and entry['pc']==pc
    assert entry['formal_inventory']==[dict(name='this',type='CompilerArena *')]
    assert entry['seeded']is seed and entry['pre_seed']==before
    expected=reset_changes(before,index,base,True)if seed else before
    assert entry['compiler_state']==expected
    expected=reset_changes(before,index,base,False)
    assert returned['compiler_state']==expected
    assert returned['pc']==entry['return'] and returned['esp']==entry['esp']+4
    assert semantic(entry['fp'])==semantic(returned['fp'])
    return expected

def validate(original,native,fields,first_member,first_scaler,state,member_sequence,scaler_sequence,sequence,module,candidate,probe):
    assert candidate['reset_observers']==RESET_PLAN
    assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])in [('arenas-complete',0x12ac3),('default-pool-arena',0x12b30),('pool-refill',0xaa9c)]
    binary=captured_native_binary(native);names=symbols(binary);events=(original/'events.txt').read_text()
    def read(name):return json.loads((native/(name+'.json')).read_text())
    tables={name:read(name)for name in ('arena-member-entry-observations','arena-member-returns','scaler-entries','scaler-returns','arena-api-entries','arena-api-returns','arena-reset-entries','arena-reset-returns')}
    assert all(len(rows)==6 for rows in tables.values())
    def traps(rva):
        return [(int(seq),int(esp,16),int(ecx,16),[int(x,16)for x in words.split(',')])for seq,esp,ecx,words in re.findall(r'^trap seq=(\d+) rva=%08x esp=([0-9a-f]+) eax=[0-9a-f]+ ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%rva,events,re.M)]
    members=traps(0x12a18);scalers=traps(0x12a7a);resets=traps(0x12ab2);reset_returns=traps(0x12a70)
    member_returns=traps(0x12a0f)+traps(0x129bc);member_returns.sort()
    assert len(traps(0x12a0f))==1 and len(traps(0x129bc))==5
    assert all(len(rows)==6 for rows in (members,scalers,resets,reset_returns,member_returns))
    before=re.findall(r'^arena-allocation-before seq=(\d+) esp=([0-9a-f]+) address=([0-9a-f]+) bytes=([0-9a-f]+) kind=([0-9a-f]+) protection=([0-9a-f]+) provider=([0-9a-f]+) error=(\d+)$',events,re.M)
    after=re.findall(r'^arena-allocation-after seq=(\d+) esp=([0-9a-f]+) before-esp=([0-9a-f]+) result=([0-9a-f]+) error=(\d+) before-error=(\d+)$',events,re.M)
    regions=re.findall(r'^arena-region base=([0-9a-f]+) allocation=([0-9a-f]+) allocation-protect=([0-9a-f]+) bytes=([0-9a-f]+) state=([0-9a-f]+) protect=([0-9a-f]+) type=([0-9a-f]+)$',events,re.M)
    assert len(before)==len(after)==len(regions)==6
    assert (member_sequence,scaler_sequence)==(members[0][0],scalers[0][0])
    assert tables['arena-member-entry-observations'][0]['compiler_state']==first_member['compiler_state']
    assert tables['scaler-entries'][0]['compiler_state']==first_scaler['compiler_state']
    initial=read('initial-x87');assert len(initial)==1
    reference=sha(original/'receipt.json');binding=read('x87-entry-binding')
    assert binding['original_receipt_sha256']==reference and binding['original_float_save_sha256']==sha(original/'fpu-000.bin') and binding['original_environment_sha256']==sha(original/'invoke-environment.bin')
    assert initial[0]['profile']==dict(reference=reference,applications=1) and initial[0]['args']==[state['module_token'],1,0]
    assert semantic(initial[0]['fp'])==semantic(original_fp(original,0))
    assert semantic(initial[0]['fp'])==dict(cw=0x027f,sw=0x20,tag=0xffff,top=0,live=[])
    kernel=int(re.findall(r'^module name=KERNEL32.dll actual=([0-9a-f]+)',events,re.M)[0],16)
    provider=kernel+export_rva(original/'loaded-kernel32.bin','VirtualAlloc')
    original_bss=bytearray((original/('compiler-bss-%03d.bin'%member_sequence)).read_bytes())
    expected=copy.deepcopy(first_member['compiler_state']);native_spans=[];original_spans=[];last=member_sequence-1;reports=[]
    def unchanged(at):
        for region in ('compiler-data','compiler-snapshot','state'):
            assert(original/('%s-%03d.bin'%(region,at))).read_bytes()==(original/('%s-%03d.bin'%(region,member_sequence))).read_bytes()
    def same_bss(at):assert(original/('compiler-bss-%03d.bin'%at)).read_bytes()==bytes(original_bss)
    for index in range(6):
        entry=tables['arena-member-entry-observations'][index];returned=tables['arena-member-returns'][index]
        helper=tables['scaler-entries'][index];nb=tables['arena-api-entries'][index];na=tables['arena-api-returns'][index]
        reset=tables['arena-reset-entries'][index];reset_return=tables['arena-reset-returns'][index]
        old_member,old_scaler,old_reset,old_reset_return,old_return=[rows[index]for rows in (members,scalers,resets,reset_returns,member_returns)]
        bs,ats=int(before[index][0]),int(after[index][0]);timeline=[old_member[0],old_scaler[0],bs,ats,old_reset[0],old_reset_return[0],old_return[0]]
        assert last<timeline[0] and all(a<b for a,b in zip(timeline,timeline[1:])) and timeline[-1]<sequence;last=timeline[-1]
        native_this=names['c1_bss'][0]+804+index*32;original_this=module+0x58fb0+index*32
        descriptor=names['c1_arena_descriptors'][0]+index*20;original_descriptor=module+0x65780+index*20
        caller='initialize_primary_arenas'if index==0 else 'initialize_compiler_arenas';old_pc=module+(0x12a0f if index==0 else 0x129bc)
        assert entry['pc']==names['_ZN13CompilerArena10InitializeEP23CompilerArenaDescriptorPv'][0] and entry['ecx']==native_this
        assert entry['args']==dict(this=native_this,descriptor=descriptor,address=0) and entry['descriptor']==descriptor and entry['address']==0
        assert entry['formal_inventory']==[dict(name=name,type=kind)for name,kind in [('this','CompilerArena *'),('descriptor','CompilerArenaDescriptor *'),('address','void *')]]
        assert entry['caller']==dict(symbol=caller,pc=entry['return'],args={})
        call(binary,entry['return'],entry['pc'])
        assert entry['invocation']==state['invocation'] and entry['crt_state']==first_member['crt_state'] and entry['compiler_state']==expected
        assert old_member[2]==original_this and old_member[3][:3]==[old_pc,original_descriptor,0]
        same_bss(old_member[0]);unchanged(old_member[0])
        expected['bss']['arenas'][index]['descriptor']=descriptor;struct.pack_into('<I',original_bss,0x8fc0+index*32,original_descriptor)
        assert helper['compiler_state']==expected and helper['args']==dict(descriptor=descriptor)
        assert helper['caller']['args']==entry['args'] and helper['invocation']==entry['invocation'] and helper['crt_state']==entry['crt_state']
        same_bss(old_scaler[0]);unchanged(old_scaler[0])
        report=compare(original,native,index);assert report['initial_state_match'] and report['final_state_match'] and all(row['architectural_state_match']for row in report['comparisons']);reports.append(report)
        size=report['rounded_return'];expected['bss']['arenas'][index]['reserve_bytes']=size;struct.pack_into('<I',original_bss,0x8fc4+index*32,size)
        flags=0x102000 if RECORDS[index]['flags']&1 else 0x2000
        b,a=before[index],after[index]
        assert [int(value,16)for value in b[2:7]]==[0,size,flags,4,provider]
        assert int(a[1],16)==int(b[1],16)+16 and int(a[2],16)==int(b[1],16) and int(b[7])==int(a[4])==int(a[5])==0
        old_base=int(a[3],16);new_base=na['eax'];assert old_base and new_base and old_base%0x10000==0 and new_base%4096==0
        if index==0:assert new_base%0x10000==0
        assert tuple(int(x,16)for x in regions[index])==(old_base,old_base,4,size,0x2000,0,0x20000)
        assert nb['args']==[0,size,flags,4] and nb['pc']==names['recon_VirtualAlloc'][0]
        assert nb['formal_inventory']==[dict(name=name,type=kind)for name,kind in [('address','LPVOID'),('bytes','DWORD'),('kind','DWORD'),('protection','DWORD')]]
        assert nb['last_error']==na['last_error']==0 and nb['preserved']==na['preserved'] and semantic(nb['fp'])==semantic(na['fp'])
        assert na['pc']==nb['return'] and na['esp']==nb['esp']+20;call(binary,na['pc'],nb['pc'])
        assert nb['compiler_state']==na['compiler_state']==expected
        mapped=[]
        for line in na['maps'].splitlines():
            match=re.match(r'^([0-9a-f]+)-([0-9a-f]+) ([-rwxps]+)\s',line);assert match
            start,end=int(match[1],16),int(match[2],16)
            if start<new_base+size and end>new_base:mapped.append((max(start,new_base),min(end,new_base+size),match[3]))
        assert mapped and mapped[0][0]==new_base and mapped[-1][1]==new_base+size and all(row[2]=='---p'for row in mapped)
        assert all(a[1]==b[0]for a,b in zip(mapped,mapped[1:]))
        native_spans.append((new_base,new_base+size));original_spans.append((old_base,old_base+size))
        same_bss(bs);same_bss(ats);unchanged(bs);unchanged(ats)
        expected['bss']['arenas'][index].update(base=new_base,mapped_view=0)
        struct.pack_into('<I',original_bss,0x8fb8+index*32,old_base);struct.pack_into('<I',original_bss,0x8fcc+index*32,0)
        if probe=='restore':
            assert(original/('arena-reset-preseed-%03d.bin'%index)).read_bytes()==bytes(original_bss[0x8fb0:0x8fb0+192])
            for offset,value in [(0,old_base+0x1000),(4,old_base+0x2000),(12,117+index),(24,37+index)]:struct.pack_into('<I',original_bss,0x8fb0+index*32+offset,value)
        same_bss(old_reset[0]);unchanged(old_reset[0])
        assert old_reset[2]==original_this and old_reset[3][0]==module+0x12a70
        expected=reset_proof(expected,reset,reset_return,index,native_this,probe=='restore',names['_ZN13CompilerArena5ResetEv'][0])
        call(binary,reset_return['pc'],reset['pc'])
        for offset,value in [(0,old_base),(4,old_base),(12,0),(24,0)]:struct.pack_into('<I',original_bss,0x8fb0+index*32+offset,value)
        same_bss(old_reset_return[0]);unchanged(old_reset_return[0])
        assert old_reset_return[1]==old_reset[1]+4 and semantic(original_fp(original,old_reset_return[0]))==semantic(reset_return['fp'])
        assert returned['index']==index and returned['pc']==entry['return'] and returned['esp']==entry['esp']+12 and returned['eax']==1 and returned['compiler_state']==expected
        assert old_return[1]==old_member[1]+12 and re.search(r'^fpu seq=%d rva=(?:00012a0f|000129bc) eax=00000001 '%old_return[0],events,re.M)
        same_bss(old_return[0]);unchanged(old_return[0])
    assert state['compiler_state']==expected and state['args']=={} and state['formal_inventory']==[] and state['caller']['symbol']=='initialize_compiler_arenas'
    call(binary,state['caller']['pc'],names['initialize_default_pools'][0]);same_bss(sequence);unchanged(sequence)
    for spans in (native_spans,original_spans):
        assert all(0<a<b<=0x100000000 for a,b in spans)
        ordered=sorted(spans);assert all(a[1]<=b[0]for a,b in zip(ordered,ordered[1:]))
    for index in range(2):
        row=next(row for row in fields['modeled_bss']if row['name']=='work_buffers[%d].data'%index)
        old_work=struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%member_sequence)).read_bytes(),row['original_rva']-0x50000)[0]
        new_work=first_member['compiler_state']['bss']['work_buffers'][index]['data']
        assert all(b<=old_work or a>=old_work+0x500000 for a,b in original_spans)
        assert all(b<=new_work or a>=new_work+0x500000 for a,b in native_spans)
    assert pe_read(original/'loaded-target.bin',0x12ab2,17)==bytes.fromhex('8b410833d289510c8901894104895118c3')
    seeds=re.findall(r'^arena-reset-seed index=(\d+) object=([0-9a-f]+) allocation=([0-9a-f]+) cursor-offset=00001000 limit-offset=00002000 committed=(\d+) reset=(\d+)$',events,re.M)
    assert len(seeds)==(6 if probe=='restore'else 0)
    for index,row in enumerate(seeds):assert tuple(int(value,16 if at in (1,2)else 10)for at,value in enumerate(row))==(index,module+0x58fb0+index*32,original_spans[index][0],117+index,37+index)
    return reports
