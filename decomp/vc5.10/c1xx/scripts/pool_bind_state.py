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
        require(refill['pc']==refill_pc and refill['ecx']==expected_receiver and refill['args']==[1] and refill['formal_inventory']==[{'name':'this','type':'CompilerPool *'},{'name':'request_bytes','type':'unsigned int'}] and state['symbol']=='CompilerPool::Refill' and state['args']=={'this':expected_receiver,'request_bytes':1},'actual pending Refill receiver/request/formals differ')
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
        require(struct.unpack('<4I',(original/('pool-10-'+final+'.bin')).read_bytes())==(0,0,1,0x104c6500),'original pool arena identity differs')
        require(semantic(fp((original/('x87-context-%03d.bin'%int(original_refill[0][0]))).read_bytes()))==semantic(refill['fp']),'member changed original FP state')
