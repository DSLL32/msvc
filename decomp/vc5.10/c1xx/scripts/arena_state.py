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
    member_pending='_initialize_default_arena' in trial['accepted']
    if not member_pending:
        require(state['symbol']=='initialize_default_arena' and helper<state['pc']<helper+helper_size and d.elf_byte(native,state['pc']-1)==0xcc and state['args']=={},'not genuine named helper INT3')
        ret=int(state['caller_return_pc']);require(start<ret<start+size,'pending helper caller ownership differs')
        code=bytes(d.elf_byte(native,ret-5+i) for i in range(5));require(code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==helper,'not actual native wrapper-to-helper CALL')
    else:
        member=json.loads((native/'arena-member-entry.json').read_text())
        member_start,member_size=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] _ZN13CompilerArena7ReserveEPK15ArenaDescriptorPh$',symbols,re.M).groups()]
        wanted={'this':entry['frontend']['storage']['arenas'][0],'descriptor':entry['frontend']['storage']['arena_descriptors'][0],'address':0}
        require(member['symbol']==state['symbol']=='CompilerArena::Reserve' and member['pc']==member_start and member['args']==state['args']==wanted,'actual pending member formals/receiver/descriptor identity differ')
        require([row['name'] for row in member['formal_inventory']]==['this','descriptor','address'],'thiscall formal inventory is incomplete')
        require(member['raw_entry_abi']['ecx']==wanted['this'] and member['raw_entry_abi']['stack_arguments']==[wanted['descriptor'],0],'native raw ECX/stack2 differ')
        ret=member['raw_entry_abi']['return_pc'];require(helper<ret<helper+helper_size and state['caller_return_pc']==ret,'native member caller differs')
        code=bytes(d.elf_byte(native,ret-5+i) for i in range(5));require(code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==member_start,'not actual native helper-to-member CALL')
        old_member=re.findall(r'^trap seq=\d+ rva=00038e70 .*ecx=(\w+) edi=\w+ stack=([0-9a-f,]+)$',text,re.M);require(len(old_member)==1,'missing original member ECX/stack witness')
        ecx,stack=old_member[0];words=[int(v,16) for v in stack.split(',')]
        require(int(ecx,16)==0x104c6500 and words[:3]==[0x10438e67,0x104d5af8,0],'original member receiver/descriptor/address/caller differ')
        require(member['frontend']==entry['frontend']==state['frontend'] and member['target_state']==state['target_state'],'member entry observation mutated target state')
        require((original/'arena-bss-member-frontier.bin').read_bytes()==(original/'arena-bss-frontier.bin').read_bytes(),'default-arena selection mutated original BSS')
    old_entry=next(row for row in traps if row[1]=='00038df1');old_helper=next(row for row in traps if row[1]=='00038e33')
    require(int(old_entry[2].split(',')[0],16)==0x10438c4d and int(old_helper[2].split(',')[0],16)==0x10438df9,'original noarg wrapper/helper caller differs')
    require(entry['frontend']==state['frontend'] and entry['target_state']==state['target_state'],'pre-loop wrapper mutated modeled state')
    require((original/'arena-bss-entry.bin').read_bytes()==(original/'arena-bss-frontier.bin').read_bytes() and len((original/'arena-bss-entry.bin').read_bytes())==0xf69c,'pre-loop wrapper mutated any original BSS byte')
    suffix='%03d'%int(old_entry[0])
    for prefix in ('frontend-data-','frontend-snapshot-','state-'):
        require((original/(prefix+suffix+'.bin')).read_bytes()==(original/(prefix+final+'.bin')).read_bytes(),'pre-loop wrapper mutated initialized/saved/support state')
