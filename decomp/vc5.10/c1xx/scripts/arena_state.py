"""Encountered arena records and the natural pre-loop helper frontier."""
import json
import re
import struct

FIELDS=('cursor','committed_limit','base','committed_bytes','descriptor','reserve_bytes','reset_value','mapped_view')
DESCRIPTORS=[0,0x3200000,0x10000,0x21,1,1,0xa00000,0x8000,0x10,4,2,0x500000,0x8000,0x18,4,3,0xa00000,0x8000,0x18,4,4,0xa00000,0x8000,0x18,4,5,0x500000,0x4000,0x18,4,6,0x500000,0x8000,0x18,4,7,0x500000,0x8000,0x18,4,8,0x500000,0x8000,0x18,4]


def check(s,d,original,native,trial,text,pre,phase,state,seed,final,symbols,traps):
    def require(value,message):s.require(value,'arena initialization: '+message)
    def raw(suffix,descriptors=False):
        data=(original/(('arena-descriptors-' if descriptors else 'arenas-')+suffix+'.bin')).read_bytes()
        require(len(data)==(180 if descriptors else 288),'wrong original typed record bytes')
        return list(struct.unpack('<%dI'%(len(data)//4),data))
    def seeded(snapshot,descriptor):
        return sum(([snapshot+64+4*i,snapshot+128+4*i,snapshot+192+4*i,17,descriptor+20*i,23,37,29] for i in range(9)),[])
    require(raw('000')==[0]*72,'original BSS defaults not zero')
    before=d.original_fields(original,'006');old=seeded(before['saved_state'],0x104d5af8) if seed else [0]*72
    new=seeded(pre['frontend']['saved_state'],pre['frontend']['storage']['arena_descriptors'][0]) if seed else [0]*72
    require(raw('006')==old and pre['frontend']['arenas']==new,'known nonzero reset seed/default differs')
    require(raw('phase')==phase['frontend']['arenas']==raw(final)==state['frontend']['arenas']==[0]*72,'modeled arena fields escaped BSS reset or mutated before helper')
    for suffix,snapshot in [('006',pre),('phase',phase),(final,state)]:
        require(raw(suffix,True)==snapshot['frontend']['arena_descriptors']==DESCRIPTORS,'contextual descriptor defaults/mutability differ')
    entry=json.loads((native/'arena-entry.json').read_text());start,size=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_compiler_environment$',symbols,re.M).groups()]
    helper,helper_size=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_default_arena$',symbols,re.M).groups()]
    require(entry['symbol']=='initialize_compiler_environment' and entry['pc']==start and entry['args']=={} and entry['raw_entry_abi']['stack_arguments']==[],'not actual no-argument wrapper entry')
    require(state['symbol']=='initialize_default_arena' and helper<state['pc']<helper+helper_size and d.elf_byte(native,state['pc']-1)==0xcc and state['args']=={},'not genuine named helper INT3')
    # The stub's INT3 follows ordinary compiler setup; the actual caller frame
    # supplies its saved return address, not an invented source line/address.
    ret=int(state['caller_return_pc']);require(start<ret<start+size,'pending helper caller ownership differs')
    code=bytes(d.elf_byte(native,ret-5+i) for i in range(5));require(code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==helper,'not actual native wrapper-to-helper CALL')
    old_entry=next(row for row in traps if row[1]=='00038df1');old_final=next(row for row in traps if row[1]=='00038e33')
    require(int(old_entry[2].split(',')[0],16)==0x10438c4d and int(old_final[2].split(',')[0],16)==0x10438df9,'original noarg wrapper/helper caller differs')
    require(entry['frontend']==state['frontend'] and entry['target_state']==state['target_state'],'pre-loop wrapper mutated modeled state')
    require((original/'arena-bss-entry.bin').read_bytes()==(original/'arena-bss-frontier.bin').read_bytes() and len((original/'arena-bss-entry.bin').read_bytes())==0xf69c,'pre-loop wrapper mutated any original BSS byte')
    suffix='%03d'%int(old_entry[0])
    for prefix in ('frontend-data-','frontend-snapshot-','state-'):
        require((original/(prefix+suffix+'.bin')).read_bytes()==(original/(prefix+final+'.bin')).read_bytes(),'pre-loop wrapper mutated initialized/saved/support state')
