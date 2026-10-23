"""Own C1XX ten-pool execution, successful commit and complete state guards."""
import copy,json,re,struct
from scaler_probe import symbols
from scaler_state import fp,semantic
from allocation_imports import export_rva

ORDER=[9,8,0,1,3,4,5,2,6,7]
METHODS={
 'bind':('pool-bind-entries','_ZN12CompilerPool9BindArenaEj',0x38f94,8,[('this','CompilerPool *'),('arena_index','unsigned int')]),
 'refill':('pool-refill-entries','_ZN12CompilerPool6RefillEj',0x1ee29,8,[('this','CompilerPool *'),('request_bytes','unsigned int')]),
 'allocate-pages':('arena-page-entries','_ZN13CompilerArena13AllocatePagesEjPj',0x1ee58,12,[('this','CompilerArena *'),('pages','unsigned int'),('remaining_bytes','unsigned int *')]),
 'commit':('arena-commit-entries','_ZN13CompilerArena12CommitExtentEv',0x1d738,4,[('this','CompilerArena *')])}


def check(s,d,original,native,trial,text,state,final,traps):
    def require(value,message):s.require(value,'ten-pool closure: '+message)
    def read(name):return json.loads((native/(name+'.json')).read_text())
    def call(ret,target):
        code=bytes(d.elf_byte(native,ret-5+i)for i in range(5))
        require(code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==target,'not actual native CALL')
    names=symbols(native/'native-executable.elf');model=read('state-model');storage=state['frontend']['storage']
    points=[0x1d76e,0x1d774,0x1ee9f,0x1ee52,0x38faa]+[0x38f27+i*12 for i in range(10)]
    observer=trial['pool_cycle_observers']
    require(observer['rvas']==points and observer['pool_order']==ORDER and observer['calls']==10 and observer['commit_API_calls']==1 and observer['committed_bytes']==65536,'wrong declared observation contract')
    require('pool-cycle-observers points=15 calls=10 after-mask-readback=1 no-code-mock=1\n' in text,'interior observations not after verified full mask')
    require(all(any(row['start_rva']<=point<row['end_rva'] and row.get('kind')!='text_data'for row in trial['ranges'])for point in points),'observer outside genuine restored code')
    tables={key:read(item[0])for key,item in METHODS.items()};returns=read('pool-cycle-returns')
    require(len(returns)==31 and [(v['kind'],v['index'])for v in returns]==[('commit',0)]+[(key,i)for i in range(10)for key in ('allocate-pages','refill','bind')],'wrong actual method return inventory')
    events=[]
    for i in range(10):
        events += [dict(kind='enter',function=key,index=i)for key in ('bind','refill','allocate-pages')]
        if i==0:events += [dict(kind=kind,function=key,index=0)for kind,key in [('enter','commit'),('enter','commit-api'),('return','commit-api'),('return','commit')]]
        events += [dict(kind='return',function=key,index=i)for key in ('allocate-pages','refill','bind')]
    require(read('pool-cycle-events')==events and len(events)==64,'not actual complete ten-call chronology')
    # Generic snapshots are indexed before save_state increments sequence;
    # the additional full-BSS/nonvolatile observations explicitly use seq+1.
    originals={}
    for key,item in METHODS.items():
        rows=re.findall(r'^trap seq=(\d+) rva=%08x esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$'%item[2],text,re.M)
        originals[key]=[dict(seq=int(v[0]),esp=int(v[1],16),eax=int(v[2],16),ecx=int(v[3],16),edi=int(v[4],16),stack=[int(w,16)for w in v[5].split(',')])for v in rows]
        require(len(rows)==len(tables[key])==(1 if key=='commit'else 10),'wrong '+key+' entry count')
    samples=[]
    pattern=r'^pool-cycle point=(\d+) sample=(\d+) seq=(\d+) rva=(\w+) esp=(\w+) eax=(\w+) ecx=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+) error=(\d+) stack=([0-9a-f,]+)$'
    for row in re.findall(pattern,text,re.M):
        point,sample,seq=map(int,row[:3]);value=dict(zip(('rva','esp','eax','ecx','ebx','esi','edi','ebp'),[int(v,16)for v in row[3:11]]));value.update(point=point,sample=sample,seq=seq,error=int(row[11]),stack=[int(v,16)for v in row[12].split(',')]);samples.append(value)
    require(len(samples)==33 and [v['sample']for v in samples]==list(range(33)) and [v['point']for v in samples]==[0,1,2,3,4,5]+[v for i in range(1,10)for v in (3,4,5+i)],'wrong original interior chronology')
    pool=read('arena-pool-entries');require(len(pool)==1,'missing actual wrapper entry');pool=pool[0]
    wrapper_seq=int(next(v[0]for v in traps if v[1]=='00038f1b'))
    raw=bytearray((original/('pool-bss-%03d.bin'%(wrapper_seq+1))).read_bytes());require(len(raw)==63132 and raw==(original/'reset-return-bss-008.bin').read_bytes(),'wrapper initial full BSS differs from proven nine Reset prefix')
    expected=copy.deepcopy(pool['guard']);native_bss=bytearray.fromhex(expected['bss']);native_start=storage['saved_state'][0];arena=storage['arenas'][0];arena_offset=arena-native_start;old_arena_offset=0xc6500-0xb9000
    base=struct.unpack_from('<I',native_bss,arena_offset+8)[0];old_base=struct.unpack_from('<I',raw,old_arena_offset+8)[0];reserve=struct.unpack_from('<I',native_bss,arena_offset+20)[0]
    require(base and old_base and reserve==struct.unpack_from('<I',raw,old_arena_offset+20)[0] and reserve>=65536,'bad independently reserved arena identity/size')
    baseline={key:(original/(prefix+'-%03d.bin'%wrapper_seq)).read_bytes()for key,prefix in [('data','frontend-data'),('saved','frontend-snapshot'),('support','state')]}
    last=None
    def same(value,old,is_sample=False):
        nonlocal last
        if is_sample:
            index=old['sample'];now=(old['seq'],index);prefix='pool-cycle-';suffix='%03d.bin'%index
            originals_bss=(original/(prefix+'bss-'+suffix)).read_bytes();oldfp=fp((original/(prefix+'fp-'+suffix)).read_bytes())
            for key in baseline:require((original/(prefix+key+'-'+suffix)).read_bytes()==baseline[key],'original '+key+' changed at interior point')
            require(old['error']==203,'original interior LastError changed')
        else:
            index=old['seq'];now=(index,-1);suffix='%03d.bin'%index
            originals_bss=(original/('pool-bss-%03d.bin'%(index+1))).read_bytes();oldfp=fp((original/('x87-context-'+suffix)).read_bytes())
            for key,prefix in [('data','frontend-data'),('saved','frontend-snapshot'),('support','state')]:require((original/(prefix+'-'+suffix)).read_bytes()==baseline[key],'original '+key+' changed at entry')
            require(re.findall(r'^trap seq=%d .*\nlast-error=(\d+)$'%index,text,re.M)==['203'],'original entry LastError changed')
        # An interior return uses the next generic sequence value, before that
        # generic boundary, hence interiors sort before its entry snapshot.
        ordered=(now[0],0 if is_sample else 1,now[1]);require(last is None or ordered>last,'observation order regressed');last=ordered
        expected['bss']=native_bss.hex()
        require(originals_bss==bytes(raw),'original full BSS changed beyond exact modeled writes')
        require(value['guard']==expected and value['last_error']==203,'native full modeled/padding/saved/external state or error differs')
        require(semantic(value['fp'])==semantic(oldfp)==semantic(pool['fp']),'actual FP state differs')
    def preserved(row):return {key:row[key]for key in ('ebx','esi','edi','ebp')}
    def old_preserved(row):
        if 'ebx'in row:return preserved(row)
        values=re.findall(r'^pool-registers seq=%d ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$'%(row['seq']+1),text,re.M);require(len(values)==1,'missing original entry nonvolatiles')
        return dict(zip(('ebx','esi','edi','ebp'),[int(v,16)for v in values[0]]))
    def entered(key,index,receiver,args,old_receiver,old_args,return_rva,caller_name):
        value=tables[key][index];old=originals[key][index];item=METHODS[key]
        require(value['pc']==names[item[1]][0] and value['ecx']==receiver and value['args']==args and value['formal_inventory']==[dict(name=n,type=t)for n,t in item[4]],'actual '+key+' receiver/formals/arguments differ')
        require(old['ecx']==old_receiver and old['stack'][:len(old_args)+1]==[0x10400000+return_rva]+old_args,'original '+key+' receiver/arguments/caller differ')
        start,size=names[caller_name];require(start<value['return_pc']<start+size,'native caller ownership differs');call(value['return_pc'],value['pc']);same(value,old)
        return value,old
    def returned(key,index,before,old_before,sample,result=None):
        value=next(v for v in returns if v['kind']==key and v['index']==index);cleanup=METHODS[key][3]
        require(sample['rva']==old_before['stack'][0]-0x10400000 and value['pc']==before['return_pc'] and value['entry_esp']==before['esp'] and value['esp']==before['esp']+cleanup and sample['esp']==old_before['esp']+cleanup,'actual method return PC/stack cleanup differs')
        require(preserved(value)==preserved(before) and old_preserved(sample)==old_preserved(old_before),'method nonvolatile registers changed');same(value,sample,True)
        if result is not None:require(value['eax']==result[0] and sample['eax']==result[1],'actual returned page pointer differs')
    def commit_api():
        before=read('pool-commit-api-entries');after=read('pool-commit-api-returns');require(len(before)==len(after)==1,'wrong real MEM_COMMIT inventory');before,after=before[0],after[0]
        ob,oa=samples[:2];provider=re.findall(r'^pool-commit-before address=(\w+) bytes=(\w+) kind=(\w+) protection=(\w+) provider=(\w+) esp=(\w+) error=(\d+)$',text,re.M);require(len(provider)==1,'missing actual commit provider')
        kernel=int(re.search(r'^module name=KERNEL32.dll actual=(\w+) ',text,re.M)[1],16)
        require([int(v,16)for v in provider[0][:5]]==[old_base,65536,0x1000,4,kernel+export_rva(original/'loaded-kernel32.bin','VirtualAlloc')] and int(provider[0][5],16)==ob['esp'] and int(provider[0][6])==203,'actual original API/provider inputs differ')
        require(ob['stack'][:4]==[old_base,65536,0x1000,4] and oa['eax']==old_base and oa['esp']==ob['esp']+16 and preserved(oa)==preserved(ob),'original API result/RET16/nonvolatiles differ')
        require(before['pc']==names['recon_VirtualAlloc'][0] and before['args']==[base,65536,0x1000,4] and before['caller']=='CompilerArena::CommitExtent' and before['caller_this']==arena,'native actual API/caller differs')
        start,size=names[METHODS['commit'][1]];require(start<before['return_pc']<start+size,'API call outside real reconstructed CommitExtent')
        require(before['formal_inventory']==[dict(name=n,type=t)for n,t in [('address','LPVOID'),('bytes','DWORD'),('kind','DWORD'),('protection','DWORD')]],'native API input inventory differs')
        call(before['return_pc'],before['pc']);require(after['pc']==before['return_pc'] and after['esp']==before['esp']+20 and after['eax']==base and preserved(after)==preserved(before),'native API response/RET16/nonvolatile differs')
        same(before,ob,True);same(after,oa,True)
        regions=[struct.unpack('<7I',(original/('pool-commit-region-%d.bin'%i)).read_bytes())for i in range(2)]
        require(regions==[(old_base,old_base,4,65536,0x1000,4,0x20000),(old_base+65536,old_base,4,reserve-65536,0x2000,0,0x20000)],'actual original committed and opaque reserved regions differ')
        owned=after['owned_reservations'];require(len(owned)==11 and sum(row==dict(address=base,bytes=reserve)for row in owned)==1,'commit not within unique existing owned reservation')
        mapped=[]
        for line in after['maps'].splitlines():
            columns=line.split();lo,hi=[int(v,16)for v in columns[0].split('-')]
            if lo<base+reserve and hi>base:mapped.append((max(lo,base),min(hi,base+reserve),columns[1]))
        require(mapped==[(base,base+65536,'rw-p'),(base+65536,base+reserve,'---p')],'native commit protection/opaque-tail closure differs')
    for index,pool_index in enumerate(ORDER):
        pool_address=storage['pool_%02d.cursor'%(pool_index+1)][0];offset=pool_address-native_start;old_pool=model['pool_projection']['original_bases'][pool_index];old_offset=old_pool-0x104b9000
        require(struct.unpack_from('<4I',native_bss,offset)==(0,0,1,0) and struct.unpack_from('<4I',raw,old_offset)==(0,0,1,0),'pool initial existing fields differ')
        bound,old_bound=entered('bind',index,pool_address,[0],old_pool,[0],0x38f27+index*12,'initialize_pool_arenas')
        struct.pack_into('<I',native_bss,offset+12,arena);struct.pack_into('<I',raw,old_offset+12,0x104c6500)
        refill,old_refill=entered('refill',index,pool_address,[1],old_pool,[1],0x38faa,METHODS['bind'][1])
        allocated,old_allocated=entered('allocate-pages',index,arena,[1,pool_address+4],0x104c6500,[1,old_pool+4],0x1ee52,METHODS['refill'][1]);require(allocated['output_value']==0,'actual caller output prestate differs')
        cursor=base+index*4096;old_cursor=old_base+index*4096
        require(struct.unpack_from('<I',native_bss,arena_offset)[0]==cursor and struct.unpack_from('<I',raw,old_arena_offset)[0]==old_cursor,'prior arena cursor differs')
        struct.pack_into('<I',native_bss,offset+4,4096);struct.pack_into('<I',raw,old_offset+4,4096);struct.pack_into('<I',native_bss,arena_offset,cursor+4096);struct.pack_into('<I',raw,old_arena_offset,old_cursor+4096)
        if index==0:
            committed,old_committed=entered('commit',0,arena,[],0x104c6500,[],0x1ee9f,METHODS['allocate-pages'][1]);commit_api()
            struct.pack_into('<I',native_bss,arena_offset+4,base+65536);struct.pack_into('<I',native_bss,arena_offset+12,65536);struct.pack_into('<I',raw,old_arena_offset+4,old_base+65536);struct.pack_into('<I',raw,old_arena_offset+12,65536)
            returned('commit',0,committed,old_committed,samples[2])
        at=3+index*3
        returned('allocate-pages',index,allocated,old_allocated,samples[at],(cursor,old_cursor))
        struct.pack_into('<I',native_bss,offset,cursor);struct.pack_into('<I',raw,old_offset,old_cursor)
        returned('refill',index,refill,old_refill,samples[at+1],(cursor,old_cursor));returned('bind',index,bound,old_bound,samples[at+2])
    cli=read('pool-cli-entries');require(len(cli)==1,'missing next named stub input observation');cli=cli[0]
    old_cli=re.findall(r'^trap seq=(\d+) rva=00038fad esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M);require(len(old_cli)==1,'missing actual original command-line frontier');row=old_cli[0]
    old=dict(seq=int(row[0]),esp=int(row[1],16),eax=int(row[2],16),ecx=int(row[3],16),edi=int(row[4],16),stack=[int(v,16)for v in row[5].split(',')]);old_vector,argv=d.b.vector(original/'invoke-entry-argv.bin');native_vector,new_argv=d.b.vector(native/'native-argv.bin')
    require(old['stack'][:3]==[0x10438c54,26,old_vector] and cli['pc']==names['process_compiler_options'][0] and cli['args']==[26,native_vector] and argv==new_argv,'next actual argc/Invoke-vector/caller differs')
    start,size=names['c1xx_main'];require(start<cli['return_pc']<start+size,'wrong main-to-options caller');call(cli['return_pc'],cli['pc']);require(cli['formal_inventory']==[dict(name='argc',type='int'),dict(name='argv',type='char **')] and state['symbol']=='process_compiler_options' and state['args']==dict(argc=26,argv=native_vector),'next named stub formals/observed arguments differ');same(cli,old)
    require(state['frontend']['arenas']==list(struct.unpack_from('<72I',native_bss,arena_offset)),'final serialized arenas differ from exact writes')
    for i in range(10):
        cursor,remaining,minimum,receiver=struct.unpack_from('<4I',native_bss,storage['pool_%02d.cursor'%(i+1)][0]-native_start)
        require(state['frontend']['pools'][i]==dict(cursor=cursor,remaining=remaining,minimum_pages=minimum,arena=receiver),'final serialized pool differs from exact writes')
    for name in ('pool-committed','pool-final-committed'):require((original/(name+'.bin')).read_bytes()==(native/(name+'.bin')).read_bytes()==bytes(65536),'actual committed bytes not complete zero page range')
    return dict(calls=10,method_returns=31,ordered_events=64,commit_API_calls=1,committed_bytes_checked=65536,opaque_reserved_tail_bytes=reserve-65536,unmodeled_original_state_not_claimed_equivalent=True)
