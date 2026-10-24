"""Actual nine pool call sequences and the sole successful owned arena commit."""
import copy,json,re,struct
from allocation_state import symbols,call
from allocation_imports import export_rva
from provenance import captured_native_binary
from scaler_state import semantic,original_fp
import default_pool_state

ORDER=default_pool_state.ORDER
METHODS={
 'set-arena':('pool-arena-entries','CompilerPool::SetArena','_ZN12CompilerPool8SetArenaEj',0x12b30,8),
 'refill':('pool-refill-entries','CompilerPool::Refill','_ZN12CompilerPool6RefillEj',0xaa9c,8),
 'allocate-pages':('arena-page-entries','CompilerArena::AllocatePages','_ZN13CompilerArena13AllocatePagesEjPj',0xaacb,12),
 'commit':('pool-commit-entries','CompilerArena::CommitExtent','_ZN13CompilerArena12CommitExtentEv',0x9883,4)}

def validate(original,native,fields,entry,state,entry_sequence,sequence,module,candidate):
    assert candidate['expected_frontier']['kind']=='command-line-entry'
    points=[0x98b9,0x98bf,0xab12,0xaac5,0x12b46]+[0x12acf+i*12 for i in range(9)]
    assert candidate['pool_cycle_observers']['rvas']==points
    assert (original/'pool-cycle-observers.txt').read_text()==''.join('%x\n'%rva for rva in points)
    assert all(any(row['start_rva']<=rva<row['end_rva']for row in candidate['ranges'])for rva in points)
    events=(original/'events.txt').read_text();binary=captured_native_binary(native);names=symbols(binary)
    tables={key:json.loads((native/(value[0]+'.json')).read_text())for key,value in METHODS.items()}
    returns=json.loads((native/'pool-cycle-returns.json').read_text());assert len(returns)==28
    expected_events=[]
    for index in range(9):
        expected_events.extend(dict(kind='enter',function=key,index=index)for key in ('set-arena','refill','allocate-pages'))
        if index==0:
            expected_events.extend([dict(kind='enter',function='commit',index=0),dict(kind='enter',function='commit-api',index=0),dict(kind='return',function='commit-api',index=0),dict(kind='return',function='commit',index=0)])
        expected_events.extend(dict(kind='return',function=key,index=index)for key in ('allocate-pages','refill','set-arena'))
    assert json.loads((native/'pool-cycle-events.json').read_text())==expected_events
    assert [(row['kind'],row['index'])for row in returns]==[('commit',0)]+[(key,index)for index in range(9)for key in ('allocate-pages','refill','set-arena')]
    old={};old_returns={};new_returns={}
    for key,(file,symbol,linkage,rva,cleanup)in METHODS.items():
        old[key]=re.findall(r'^trap seq=(\d+) rva=%08x esp=([0-9a-f]+) eax=([0-9a-f]+) ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%rva,events,re.M)
        old[key]=[row for row in old[key]if int(row[0])<sequence]
        count=1 if key=='commit'else 9;assert len(old[key])==len(tables[key])==count
        if key=='set-arena':
            old_returns[key]=[re.findall(r'^trap seq=(\d+) rva=%08x esp=([0-9a-f]+) eax=([0-9a-f]+) ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%(0x12acf+i*12),events,re.M)for i in range(9)]
            assert all(len(row)==1 for row in old_returns[key]);old_returns[key]=[row[0]for row in old_returns[key]]
        else:
            returned={'refill':0x12b46,'allocate-pages':0xaac5,'commit':0xab12}[key]
            old_returns[key]=re.findall(r'^trap seq=(\d+) rva=%08x esp=([0-9a-f]+) eax=([0-9a-f]+) ecx=([0-9a-f]+) stack=([0-9a-f,]+)$'%returned,events,re.M);assert len(old_returns[key])==count
        new_returns[key]=[row for row in returns if row['kind']==key];assert len(new_returns[key])==count
    commit_entry=copy.deepcopy(tables['commit'][0]);commit_entry.update(commit_entry['crt_state'])
    commit_at=int(old['commit'][0][0]);assert commit_entry['boundary_rva']==0x9883 and commit_entry['pc']==names[METHODS['commit'][2]][0]
    # Retain the earlier independently checked first-write boundary before
    # validating the new commit and all following repeated calls.
    default_pool_state.validate_allocation(original,native,fields,entry,commit_entry,entry_sequence,commit_at,module,candidate)
    expected=copy.deepcopy(entry['compiler_state']);raw=bytearray((original/('compiler-bss-%03d.bin'%entry_sequence)).read_bytes())
    arena=fields['arenas']['native_offset']+names['c1_bss'][0];old_arena=module+fields['arenas']['original_rva'];arena_offset=fields['arenas']['original_rva']-0x50000
    native_base=expected['bss']['arenas'][0]['base'];old_base=struct.unpack_from('<I',raw,arena_offset+8)[0]
    baseline={region:(original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()for region in ('compiler-data','compiler-snapshot','state')}
    def registers(at):
        row=re.findall(r'^pool-registers seq=%d ebx=([0-9a-f]+) esi=([0-9a-f]+) edi=([0-9a-f]+) ebp=([0-9a-f]+)$'%at,events,re.M);assert len(row)==1
        return dict(zip(('ebx','esi','edi','ebp'),(int(value,16)for value in row[0])))
    def same(at,value):
        assert value['compiler_state']==expected
        error=re.findall(r'^trap seq=%d .*\nlast-error=(\d+)$'%at,events,re.M);assert error==['0']
        actual_error=value.get('last_error',value.get('crt_state',{}).get('win32_last_error',value.get('win32_last_error')));assert actual_error==0
        assert(original/('compiler-bss-%03d.bin'%at)).read_bytes()==bytes(raw)
        for region,data in baseline.items():assert(original/('%s-%03d.bin'%(region,at))).read_bytes()==data
    last=entry_sequence
    def entered(key,index,args,old_this,old_args,caller,caller_args,return_rva):
        nonlocal last
        value=tables[key][index];row=old[key][index];at=int(row[0]);words=[int(x,16)for x in row[4].split(',')]
        assert last<at<sequence;last=at
        assert value['symbol']==METHODS[key][1] and value['pc']==names[METHODS[key][2]][0] and value['boundary_rva']==METHODS[key][3]
        assert value['args']==args and value['ecx']==args['this'] and int(row[3],16)==old_this
        assert all(value[key]==wanted for key,wanted in args.items()if key!='this')
        assert words[:len(old_args)+1]==[module+return_rva]+old_args
        assert value['caller']['symbol']==caller and value['caller']['args']==caller_args and value['caller']['pc']==value['return']
        assert value['invocation']==entry['invocation'] and value['crt_state']==entry['crt_state']
        assert value['formal_inventory']==[dict(name=name,type=kind)for name,kind in {
          'set-arena':[('this','CompilerPool *'),('arena_index','unsigned int')],
          'refill':[('this','CompilerPool *'),('minimum_bytes','unsigned int')],
          'allocate-pages':[('this','CompilerArena *'),('pages','unsigned int'),('available_bytes','unsigned int *')],
          'commit':[('this','CompilerArena *')]}[key]]
        call(binary,value['return'],value['pc']);same(at,value)
        assert semantic(value['fp'])==semantic(original_fp(original,at))
        return value,row
    def returned(key,index,before,old_before,result=None):
        nonlocal last
        value=new_returns[key][index];row=old_returns[key][index];at=int(row[0]);assert last<at<sequence;last=at
        assert value['index']==index and value['pc']==before['return'] and value['entry_esp']==before['esp']
        assert value['esp']==before['esp']+METHODS[key][4] and int(row[1],16)==int(old_before[1],16)+METHODS[key][4]
        assert value['preserved']==before['preserved'] and registers(at)==registers(int(old_before[0]))
        assert value['last_error']==entry['crt_state']['win32_last_error']==0
        assert semantic(value['fp'])==semantic(before['fp'])==semantic(original_fp(original,at));same(at,value)
        if result is not None:assert value['eax']==result[0] and int(row[2],16)==result[1]
    for index,pool_index in enumerate(ORDER):
        model=fields['pools'][pool_index];pool=names['c1_bss'][0]+model['native_offset'];old_pool=module+model['original_object_rva'];pool_offset=model['original_object_rva']-0x50000
        bound,old_bound=entered('set-arena',index,dict(this=pool,arena_index=0),old_pool,[0],'initialize_default_pools',{},0x12acf+index*12)
        expected['bss']['pools'][pool_index]['arena']=arena;struct.pack_into('<I',raw,pool_offset+12,old_arena)
        refill,old_refill=entered('refill',index,dict(this=pool,minimum_bytes=1),old_pool,[1],'CompilerPool::SetArena',dict(this=pool,arena_index=0),0x12b46)
        allocated,old_allocated=entered('allocate-pages',index,dict(this=arena,pages=1,available_bytes=pool+4),old_arena,[1,old_pool+4],'CompilerPool::Refill',dict(this=pool,minimum_bytes=1),0xaac5)
        previous=native_base+index*4096;old_previous=old_base+index*4096
        assert expected['bss']['arenas'][0]['cursor']==previous
        expected['bss']['pools'][pool_index]['remaining']=4096;expected['bss']['arenas'][0]['cursor']=previous+4096
        struct.pack_into('<I',raw,pool_offset+4,4096);struct.pack_into('<I',raw,arena_offset,old_previous+4096)
        if index==0:
            committed,old_committed=entered('commit',0,dict(this=arena),old_arena,[],'CompilerArena::AllocatePages',allocated['args'],0xab12)
            last=validate_commit_api(original,native,expected,raw,baseline,last,sequence,module,names,old_base,native_base,registers)
            expected['bss']['arenas'][0]['committed_limit']=native_base+0x10000;expected['bss']['arenas'][0]['committed_bytes']=0x10000
            struct.pack_into('<I',raw,arena_offset+4,old_base+0x10000);struct.pack_into('<I',raw,arena_offset+12,0x10000)
            returned('commit',0,committed,old_committed)
        returned('allocate-pages',index,allocated,old_allocated,(previous,old_previous))
        expected['bss']['pools'][pool_index]['cursor']=previous;struct.pack_into('<I',raw,pool_offset,old_previous)
        returned('refill',index,refill,old_refill,(previous,old_previous))
        returned('set-arena',index,bound,old_bound)
    assert last<sequence;assert state['symbol']=='parse_command_line' and state['args']=={key:entry['invocation']['args'][key]for key in ('argc','argv')}
    assert state['formal_inventory']==[dict(name='argc',type='int'),dict(name='argv',type='char **')]
    assert state['caller']['symbol']=='c1_main' and state['caller']['args']==state['args'];call(binary,state['caller']['pc'],names['parse_command_line'][0]);same(sequence,state)
    for key,value in entry['crt_state'].items():assert state[key]==value
    final=re.findall(r'^trap seq=%d rva=00012b49 .*stack=([0-9a-f,]+)$'%sequence,events,re.M);invoke=re.findall(r'^trap seq=\d+ rva=00012229 .*stack=([0-9a-f,]+)$',events,re.M);assert len(final)==len(invoke)==1
    old_args=[int(x,16)for x in invoke[0].split(',')][1:3];assert [int(x,16)for x in final[0].split(',')][:3]==[module+0x127fe]+old_args
    for prefix in ('pool-committed','pool-final-committed'):
        assert(original/(prefix+'-000.bin')).read_bytes()==(native/(prefix+'-000.bin')).read_bytes()==bytes(0x10000)

def validate_commit_api(original,native,expected,raw,baseline,last,sequence,module,names,old_base,new_base,registers):
    events=(original/'events.txt').read_text();before=re.findall(r'^pool-commit-before seq=(\d+) esp=([0-9a-f]+) address=([0-9a-f]+) bytes=([0-9a-f]+) kind=([0-9a-f]+) protection=([0-9a-f]+) provider=([0-9a-f]+) error=(\d+)$',events,re.M)
    after=re.findall(r'^pool-commit-after seq=(\d+) esp=([0-9a-f]+) before-esp=([0-9a-f]+) result=([0-9a-f]+) error=(\d+) before-error=(\d+)$',events,re.M);assert len(before)==len(after)==1;b=before[0];a=after[0]
    at,bt=int(b[0]),int(a[0]);assert last<at<bt<sequence
    kernel=int(re.findall(r'^module name=KERNEL32.dll actual=([0-9a-f]+)',events,re.M)[0],16);provider=kernel+export_rva(original/'loaded-kernel32.bin','VirtualAlloc')
    assert [int(x,16)for x in b[2:7]]==[old_base,0x10000,0x1000,4,provider] and int(b[7])==int(a[4])==int(a[5])==0
    assert int(a[1],16)==int(b[1],16)+16 and int(a[2],16)==int(b[1],16) and int(a[3],16)==old_base and registers(at)==registers(bt)
    nb=json.loads((native/'pool-commit-api-entries.json').read_text());na=json.loads((native/'pool-commit-api-returns.json').read_text());assert len(nb)==len(na)==1;nb=nb[0];na=na[0]
    assert nb['args']==[new_base,0x10000,0x1000,4] and nb['pc']==names['recon_VirtualAlloc'][0]
    assert nb['formal_inventory']==[dict(name=name,type=kind)for name,kind in [('address','LPVOID'),('bytes','DWORD'),('kind','DWORD'),('protection','DWORD')]]
    assert nb['caller']['symbol']=='CompilerArena::CommitExtent' and nb['caller']['args']==dict(this=expected['live_addresses']['bss']+804)
    assert na['index']==0 and na['pc']==nb['return'] and na['esp']==nb['esp']+20 and na['eax']==new_base
    call(captured_native_binary(native),na['pc'],nb['pc']);assert nb['preserved']==na['preserved'] and nb['last_error']==na['last_error']==0
    assert semantic(nb['fp'])==semantic(na['fp'])==semantic(original_fp(original,at))==semantic(original_fp(original,bt))
    for old_at,value in [(at,nb),(bt,na)]:
        assert value['compiler_state']==expected and(original/('compiler-bss-%03d.bin'%old_at)).read_bytes()==bytes(raw)
        for region,data in baseline.items():assert(original/('%s-%03d.bin'%(region,old_at))).read_bytes()==data
    reserve=expected['bss']['arenas'][0]['reserve_bytes'];regions=re.findall(r'^pool-commit-region part=(\d+) base=([0-9a-f]+) allocation=([0-9a-f]+) allocation-protect=([0-9a-f]+) bytes=([0-9a-f]+) state=([0-9a-f]+) protect=([0-9a-f]+) type=([0-9a-f]+)$',events,re.M);assert len(regions)==2
    assert [tuple(int(x,16 if i else 10)for i,x in enumerate(row))for row in regions]==[(0,old_base,old_base,4,0x10000,0x1000,4,0x20000),(1,old_base+0x10000,old_base,4,reserve-0x10000,0x2000,0,0x20000)]
    assert len(na['allocations'])==8 and sum(view==dict(address=new_base,length=reserve)for view in na['allocations'])==1
    mapped=[]
    for line in na['maps'].splitlines():
        m=re.match(r'^([0-9a-f]+)-([0-9a-f]+) ([-rwxps]+)\s',line);assert m
        start,end=int(m[1],16),int(m[2],16)
        if start<new_base+reserve and end>new_base:mapped.append((max(start,new_base),min(end,new_base+reserve),m[3]))
    assert mapped==[(new_base,new_base+0x10000,'rw-p'),(new_base+0x10000,new_base+reserve,'---p')]
    assert(original/'pool-committed-000.bin').read_bytes()==(native/'pool-committed-000.bin').read_bytes()==bytes(0x10000)
    return bt
