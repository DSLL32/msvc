"""The ten-call binding wrapper and actual first member's complete input ABI."""
import json,re,struct
from scaler_probe import symbols
from scaler_state import fp,semantic


def check(s,d,original,native,trial,text,state,final,traps):
    def require(value,message):s.require(value,'pool binding wrapper: '+message)
    names=symbols(native/'native-executable.elf');start,size=names['initialize_pool_arenas'];method,method_size=names['_ZN12CompilerPool9BindArenaEj']
    entries=json.loads((native/'pool-bind-entries.json').read_text());require(len(entries)==1,'not one actual first member frontier');entry=entries[0]
    pool=json.loads((native/'arena-pool-entries.json').read_text());require(len(pool)==1,'missing actual noarg wrapper entry');pool=pool[0]
    expected_receiver=state['frontend']['storage']['pool_10.cursor'][0]
    require(entry['pc']==method and entry['ecx']==expected_receiver and entry['args']==[0] and entry['formal_inventory']==[{'name':'this','type':'CompilerPool *'},{'name':'arena_index','type':'unsigned int'}],'actual BindArena ECX/stack/formal inventory differs')
    require(state['symbol']=='CompilerPool::BindArena' and state['args']=={'this':expected_receiver,'arena_index':0} and entry['guard']==pool['guard'] and entry['last_error']==pool['last_error']==203 and semantic(entry['fp'])==semantic(pool['fp']),'named stub formals or complete modeled state changed')
    require(start<entry['return_pc']<start+size,'wrong actual first member caller')
    raw=bytes(d.elf_byte(native,entry['return_pc']-5+i)for i in range(5));require(raw[0]==0xe8 and entry['return_pc']+struct.unpack_from('<i',raw,1)[0]==method,'not actual wrapper-to-member CALL')
    old=re.findall(r'^trap seq=(\d+) rva=00038f94 esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M)
    require(len(old)==1 and int(old[0][3],16)==0x104c0810 and [int(v,16)for v in old[0][5].split(',')][:2]==[0x10438f27,0],'actual original member receiver/arena index/caller differs')
    wrapper=next(row for row in traps if row[1]=='00038f1b');require(semantic(fp((original/('x87-context-%03d.bin'%int(wrapper[0]))).read_bytes()))==semantic(fp((original/('x87-context-%03d.bin'%int(old[0][0]))).read_bytes()))==semantic(entry['fp']),'wrapper changed actual FP state')
    # Native compact pools retain the encountered original storage identities;
    # no new pool or arena fields or region payload are represented here.
    model=json.loads((native/'state-model.json').read_text());original_pools=model['pool_projection']['original_bases'];order=[9,8,0,1,3,4,5,2,6,7]
    require([original_pools[i]for i in order]==[0x104c0810,0x104c0b00,0x104c0ac8,0x104bf5b0,0x104bf210,0x104c06e8,0x104c0560,0x104c0498,0x104c04a8,0x104bf498],'existing pool identity map does not cover wrapper receivers')
