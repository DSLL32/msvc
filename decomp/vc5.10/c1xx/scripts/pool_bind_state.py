"""The ten-call binding wrapper and actual first member's complete input ABI."""
import copy,json,re,struct
from scaler_probe import symbols
from scaler_state import fp,semantic


def check(s,d,original,native,trial,text,state,final,traps):
    def require(value,message):s.require(value,'pool binding wrapper: '+message)
    names=symbols(native/'native-executable.elf');start,size=names['initialize_pool_arenas'];method,method_size=names['_ZN12CompilerPool9BindArenaEj']
    entries=json.loads((native/'pool-bind-entries.json').read_text());require(len(entries)==1,'not one actual first member frontier');entry=entries[0]
    pool=json.loads((native/'arena-pool-entries.json').read_text());require(len(pool)==1,'missing actual noarg wrapper entry');pool=pool[0]
    expected_receiver=state['frontend']['storage']['pool_10.cursor'][0]
    require(entry['pc']==method and entry['ecx']==expected_receiver and entry['args']==[0] and entry['formal_inventory']==[{'name':'this','type':'CompilerPool *'},{'name':'arena_index','type':'unsigned int'}],'actual BindArena ECX/stack/formal inventory differs')
    pending='CompilerPool::BindArena' not in trial['accepted']
    require((not pending or state['symbol']=='CompilerPool::BindArena' and state['args']=={'this':expected_receiver,'arena_index':0}) and entry['guard']==pool['guard'] and entry['last_error']==pool['last_error']==203 and semantic(entry['fp'])==semantic(pool['fp']),'named stub formals or complete modeled state changed')
    require(start<entry['return_pc']<start+size,'wrong actual first member caller')
    raw=bytes(d.elf_byte(native,entry['return_pc']-5+i)for i in range(5));require(raw[0]==0xe8 and entry['return_pc']+struct.unpack_from('<i',raw,1)[0]==method,'not actual wrapper-to-member CALL')
    old=re.findall(r'^trap seq=(\d+) rva=00038f94 esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M)
    require(len(old)==1 and int(old[0][3],16)==0x104c0810 and [int(v,16)for v in old[0][5].split(',')][:2]==[0x10438f27,0],'actual original member receiver/arena index/caller differs')
    wrapper=next(row for row in traps if row[1]=='00038f1b');require(semantic(fp((original/('x87-context-%03d.bin'%int(wrapper[0]))).read_bytes()))==semantic(fp((original/('x87-context-%03d.bin'%int(old[0][0]))).read_bytes()))==semantic(entry['fp']),'wrapper changed actual FP state')
    # Native compact pools retain the encountered original storage identities;
    # no new pool or arena fields or region payload are represented here.
    model=json.loads((native/'state-model.json').read_text());original_pools=model['pool_projection']['original_bases'];order=[9,8,0,1,3,4,5,2,6,7]
    require([original_pools[i]for i in order]==[0x104c0810,0x104c0b00,0x104c0ac8,0x104bf5b0,0x104bf210,0x104c06e8,0x104c0560,0x104c0498,0x104c04a8,0x104bf498],'existing pool identity map does not cover wrapper receivers')

    if not pending:
        refill_pc,refill_size=names['_ZN12CompilerPool6RefillEj'];refills=json.loads((native/'pool-refill-entries.json').read_text());require(len(refills)==1,'wrong actual first refill inventory');refill=refills[0]
        require(refill['pc']==refill_pc and refill['ecx']==expected_receiver and refill['args']==[1] and refill['formal_inventory']==[{'name':'this','type':'CompilerPool *'},{'name':'request_bytes','type':'unsigned int'}] and ('CompilerPool::Refill' in trial['accepted'] or state['symbol']=='CompilerPool::Refill' and state['args']=={'this':expected_receiver,'request_bytes':1}),'actual pending Refill receiver/request/formals differ')
        require(method<refill['return_pc']<method+method_size,'wrong BindArena-to-Refill caller')
        raw=bytes(d.elf_byte(native,refill['return_pc']-5+i)for i in range(5));require(raw[0]==0xe8 and refill['return_pc']+struct.unpack_from('<i',raw,1)[0]==refill_pc,'not actual member-to-refill CALL')
        original_refill=re.findall(r'^trap seq=(\d+) rva=0001ee29 esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M)
        require(len(original_refill)==1 and int(original_refill[0][3],16)==0x104c0810 and [int(v,16)for v in original_refill[0][5].split(',')][:2]==[0x10438faa,1],'actual original refill receiver/request/caller differs')
        arena=state['frontend']['storage']['arenas'][0];offset=state['frontend']['storage']['pool_10.arena'][0]-state['frontend']['storage']['saved_state'][0]
        expected=copy.deepcopy(entry['guard']);bss=bytearray.fromhex(expected['bss']);struct.pack_into('<I',bss,offset,arena);expected['bss']=bss.hex()
        require(refill['guard']==expected and state['frontend']['pools'][9]['arena']==arena and refill['last_error']==203 and semantic(refill['fp'])==semantic(entry['fp']),'member did not perform exactly its one native arena-pointer store')
        baseline=(original/'pool-bind-bss-entry.bin').read_bytes();require(len(baseline)==0xf69c and baseline==(original/'reset-return-bss-008.bin').read_bytes(),'wrapper changed original whole BSS before binding')
        original_bss=bytearray(baseline);struct.pack_into('<I',original_bss,0xc0810-0xb9000+12,0x104c6500)
        require((original/'pool-bind-bss-frontier.bin').read_bytes()==bytes(original_bss),'member changed original whole BSS beyond one pointer store')
        before='%03d'%int(old[0][0])
        for prefix in ('frontend-data-','frontend-snapshot-','state-'):
            require((original/(prefix+before+'.bin')).read_bytes()==(original/(prefix+final+'.bin')).read_bytes(),'member changed original initialized/saved/support storage')
        require(struct.unpack('<4I',(original/('pool-10-'+'%03d'%int(original_refill[0][0])+'.bin')).read_bytes())==(0,0,1,0x104c6500),'original pool arena identity differs')
        require(semantic(fp((original/('x87-context-%03d.bin'%int(original_refill[0][0]))).read_bytes()))==semantic(refill['fp']),'member changed original FP state')
    if 'CompilerPool::Refill' in trial['accepted']:
        page_pc,page_size=names['_ZN13CompilerArena13AllocatePagesEjPj'];entries=json.loads((native/'arena-page-entries.json').read_text());require(len(entries)==1,'wrong actual first page allocation frontier');page=entries[0]
        arena=state['frontend']['storage']['arenas'][0];output,width=state['frontend']['storage']['pool_10.remaining']
        require(width==4 and page['pc']==page_pc and page['ecx']==arena and page['args']==[1,output] and page['output_value']==0 and page['formal_inventory']==[{'name':'this','type':'CompilerArena *'},{'name':'pages','type':'unsigned int'},{'name':'remaining_bytes','type':'unsigned int *'}],'actual AllocatePages ECX/pages/output pointer/value/formals differ')
        require(('CompilerArena::AllocatePages' in trial['accepted'] or state['symbol']=='CompilerArena::AllocatePages' and state['args']=={'this':arena,'pages':1,'remaining_bytes':output}),'named page stub input ABI differs')
        require(refill_pc<page['return_pc']<refill_pc+refill_size,'wrong actual refill-to-page caller')
        raw=bytes(d.elf_byte(native,page['return_pc']-5+i)for i in range(5));require(raw[0]==0xe8 and page['return_pc']+struct.unpack_from('<i',raw,1)[0]==page_pc,'not actual refill-to-page CALL')
        require(page['guard']==refill['guard'] and page['last_error']==203 and semantic(page['fp'])==semantic(refill['fp']),'refill changed target state/FP before page helper frontier')
        original_page=re.findall(r'^trap seq=(\d+) rva=0001ee58 esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M)
        require(len(original_page)==1 and int(original_page[0][3],16)==0x104c6500 and [int(v,16)for v in original_page[0][5].split(',')][:3]==[0x1041ee52,1,0x104c0814],'actual original page this/count/out-pointer/caller differs')
        request=refill['args'][0];info=(original/'system-info-after.bin').read_bytes();page_bytes=struct.unpack_from('<I',info,4)[0];minimum=state['frontend']['pools'][9]['minimum_pages'];pages=max(((request-1)&0xffffffff)//page_bytes+1,minimum)
        require(page_bytes==4096 and minimum==1 and request==1 and pages==page['args'][0],'actual unsigned ceil/max page inputs/result differ')
        require((original/'pool-refill-bss-frontier.bin').read_bytes()==(original/'pool-bind-bss-frontier.bin').read_bytes(),'refill changed original whole BSS before allocator frontier')
        before='%03d'%int(original_refill[0][0])
        for prefix in ('frontend-data-','frontend-snapshot-','state-'):
            require((original/(prefix+before+'.bin')).read_bytes()==(original/(prefix+final+'.bin')).read_bytes(),'refill changed original initialized/saved/support storage')
        require(struct.unpack('<4I',(original/('pool-10-'+'%03d'%int(original_refill[0][0])+'.bin')).read_bytes())==(0,0,1,0x104c6500),'original output pointer does not still address actual remaining DWORD')
        require(semantic(fp((original/('x87-context-%03d.bin'%int(original_page[0][0]))).read_bytes()))==semantic(page['fp']),'original/native page frontier FP context differs')
    if 'CompilerArena::AllocatePages' in trial['accepted']:
        commit_pc,commit_size=names['_ZN13CompilerArena12CommitExtentEv'];entries=json.loads((native/'arena-commit-entries.json').read_text());require(len(entries)==1,'wrong actual CommitExtent frontier inventory');commit=entries[0]
        require(commit['pc']==commit_pc and commit['ecx']==arena and commit['args']==[] and commit['formal_inventory']==[{'name':'this','type':'CompilerArena *'}] and state['symbol']=='CompilerArena::CommitExtent' and state['args']=={'this':arena},'actual pending CommitExtent ECX/no-stackargs/formals differ')
        require(page_pc<commit['return_pc']<page_pc+page_size,'wrong allocator-to-commit caller')
        raw=bytes(d.elf_byte(native,commit['return_pc']-5+i)for i in range(5));require(raw[0]==0xe8 and commit['return_pc']+struct.unpack_from('<i',raw,1)[0]==commit_pc,'not actual allocator-to-commit CALL')
        old_commit=re.findall(r'^trap seq=(\d+) rva=0001d738 esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M)
        require(len(old_commit)==1 and int(old_commit[0][3],16)==0x104c6500 and int(old_commit[0][5].split(',')[0],16)==0x1041ee9f,'actual original CommitExtent receiver/caller differs')
        native_bss_start=state['frontend']['storage']['saved_state'][0];arena_offset=arena-native_bss_start;out_offset=output-native_bss_start
        before=bytearray.fromhex(page['guard']['bss']);native_fields=list(struct.unpack_from('<8I',before,arena_offset));cursor,limit,base=native_fields[:3]
        offset=(cursor-base)&0xffffffff;available=((((page['args'][0]+1)*page_bytes+offset-1)&(~(page_bytes-1)&0xffffffff))-offset)&0xffffffff
        require(available==4096 and cursor==limit==base and base and cursor+available>limit,'actual page arithmetic/prestate differs')
        expected=copy.deepcopy(page['guard']);struct.pack_into('<I',before,out_offset,available);struct.pack_into('<I',before,arena_offset,cursor+available);expected['bss']=before.hex()
        require(commit['guard']==expected and commit['last_error']==203 and semantic(commit['fp'])==semantic(page['fp']),'allocator changed native state beyond count/cursor writes')
        expected_pools=copy.deepcopy(json.loads((native/'arena-size-entry.json').read_text())['frontend']['pools']);expected_pools[9].update(arena=arena,remaining=available)
        require(state['frontend']['arenas']==list(struct.unpack_from('<72I',before,arena_offset)) and state['frontend']['pools']==expected_pools,'serialized final arena/pool state differs from exact actual writes')
        original_bss=bytearray((original/'pool-refill-bss-frontier.bin').read_bytes());old_cursor,old_limit,old_base=struct.unpack_from('<3I',original_bss,0xc6500-0xb9000)
        require(old_cursor==old_limit==old_base and old_base,'original page input pointer identities differ')
        struct.pack_into('<I',original_bss,0xc0814-0xb9000,available);struct.pack_into('<I',original_bss,0xc6500-0xb9000,old_cursor+available)
        require((original/'arena-page-bss-frontier.bin').read_bytes()==bytes(original_bss),'allocator changed original whole BSS beyond count/cursor writes')
        require((original/('arenas-'+final+'.bin')).read_bytes()==bytes(original_bss[0xc6500-0xb9000:0xc6500-0xb9000+288]) and struct.unpack('<4I',(original/('pool-10-'+final+'.bin')).read_bytes())==(0,available,1,0x104c6500),'original final typed field/value aliases differ')
        require(semantic(fp((original/('x87-context-%03d.bin'%int(old_commit[0][0]))).read_bytes()))==semantic(commit['fp']),'CommitExtent FP context differs')
