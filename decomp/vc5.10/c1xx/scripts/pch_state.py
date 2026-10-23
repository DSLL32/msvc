"""PCH context input ABI and typed68-byte projection; opaque pointees stay opaque."""
import json
import re
import struct

FIELDS = [('image_count','int'),('root_image','struct PchImage *'),('images','struct PchImage **'),('stored_file_position','DWORD'),('file_name','const char *'),('file_offset','LONG'),('file','HANDLE')] + [('mapping_handles[%d]'%i,'HANDLE') for i in range(6)] + [('loaded_image_count','int'),('scratch_arena','struct CompilerArena *'),('original_pages','struct PchImage **'),('mapped_pages','struct PchImage **')]
SCALARS = {'image_count':17,'stored_file_position':23,'file_offset':-11,'loaded_image_count':29}


def decode(blob):
    values=list(struct.unpack('<17I',blob))
    if values[5]>=0x80000000:values[5]-=0x100000000
    return dict(zip([name for name,_ in FIELDS],values))


def seeded(pointer):
    values={};offset=0
    for name,_ in FIELDS:
        if name in SCALARS:values[name]=SCALARS[name]
        else:offset+=4;values[name]=pointer+offset
    return values


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    model=json.loads((native/'state-model.json').read_text())
    s.require(model['pch_projection']=={'original_base':0x104c64b8,'bytes':68,'fields':[list(row) for row in FIELDS],'member_rva':0x3bd41,'member_abi':'ECX this, no stack arguments, EAX this, RET0; EDI preserved','pointees':'opaque until actual semantic consumer; no image/arena/page layout modeled'},'unreviewed PCH field/type scope')
    def raw(suffix):
        blob=(original/('pch-context-'+suffix+'.bin')).read_bytes();s.require(len(blob)==68,'missing complete PCH field projection');return decode(blob)
    zero=dict.fromkeys([name for name,_ in FIELDS],0)
    s.require(raw('000')==zero,'original PCH defaults differ')
    old_owned=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0]
    s.require(raw('006')==(seeded(old_owned) if seed else zero) and pre['frontend']['pch_context']==(seeded(pre['frontend']['saved_state']) if seed else zero),'PCH nonzero BSS input differs')
    s.require(raw('phase')==phase['frontend']['pch_context']==zero,'PCH context escaped BSS clearing')
    row=next(row for row in trial['driver'] if row['name']=='PchContext::Initialize')
    s.require(row['symbol']=='?Initialize@PchContext@@QAEPAU1@XZ' and row['native_symbol']=='_ZN10PchContext10InitializeEv','wrong genuine PCH member symbol mapping')
    entries=[r for r in traps if r[1]=='0003bd41'];s.require(len(entries)==1,'missing/extra actual PCH member entry')
    entry=json.loads((native/'pch-entry.json').read_text());receiver=entry['frontend']['storage']['pch_context.image_count'][0]
    native_start=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] _ZN10PchContext10InitializeEv$',symbols,re.M)[1],16)
    wrapper=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_frontend_37$',symbols,re.M)
    abi=entry['raw_entry_abi']
    s.require(entry['pc']==native_start and entry['symbol']=='PchContext::Initialize' and entry['args']=={'this':receiver} and abi=={'ecx':receiver,'edi':entry['edi'],'esp':entry['esp'],'return_pc':abi['return_pc'],'stack_arguments':[]},'PCH actual raw ECX/EDI/ESP/no-stackargs input differs')
    s.require(int(wrapper[1],16)<abi['return_pc']<int(wrapper[1],16)+int(wrapper[2],16),'native PCH caller-return PC is not in actual compiler-emitted wrapper')
    trap=entries[0];line=next(line for line in text.splitlines() if line.startswith('trap seq='+trap[0]+' '))
    s.require('ecx=104c64b8 ' in line and re.search(r' edi=[0-9a-f]{8} ',line) and int(trap[2].split(',')[0],16)==0x104385f3,'original PCH tail-inherited receiver/EDI/initializer-return PC differs')
    aux=bool(trial.get('test_pch_seed'))
    if aux:
        s.require(trial['auxiliary_pch_data_seed']=={'owner':'PchContext::Initialize','point':'actual member entry after BSS reset','receiver_unchanged':True,'scalars':SCALARS,'pointer_fields':'owned snapshot interiors +4..+52, never dereferenced','code_or_allocator_mock':False} and 'pch-seed actual-entry=0003bd41 bytes=68 receiver-unchanged=1 code-mock=0\n' in text,'undeclared PCH data auxiliary')
    else:s.require('auxiliary_pch_data_seed' not in trial and 'pch-seed ' not in text,'hidden PCH data auxiliary')
    suffix='%03d'%int(trap[0]);owned=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0]
    s.require(raw(suffix)==(seeded(owned) if aux else zero) and entry['frontend']['pch_context']==(seeded(entry['frontend']['saved_state']) if aux else zero),'PCH member input field projection differs')
    if trial.get('prospective_only'):
        s.require(trial['trial']=='_initialize_frontend_37' and len(trial['accepted'])==58 and trial['accepted'][-1]=='_initialize_frontend_37' and 'PchContext::Initialize' not in trial['accepted'] and row.get('pending_extent') and row['end']==row['start']+1,'prospective chain broadened or member unmasked')
        s.require(state['symbol']=='PchContext::Initialize' and raw(final)==state['frontend']['pch_context']==zero,'prospective member did not remain a genuine stub frontier')
        return
    pair=['_initialize_frontend_37','PchContext::Initialize']
    s.require(trial['accepted'][57:59]==pair and not row.get('pending_extent') and row['end']==0x1043bd53,'PCH bodies are not the reviewed ordered prefix')
    if trial.get('ordered_closure'):
        s.require(trial['ordered_closure']==pair and len(trial['accepted'])==59,'not the authorized strict ordered two-body closure')
    else:s.require(trial['trial'] not in pair,'PCH promotion requires the explicit joint proof')
    ret=json.loads((native/'pch-return.json').read_text())
    s.require(ret['symbol']=='initialize_frontend_37' and ret['pc']==abi['return_pc'] and ret['eax']==receiver and ret['esp']==abi['esp']+4 and ret['edi']==abi['edi'],'native PCH actual EAX/EDI/RET0 return differs')
    original_esp=int(re.search(r' esp=(\w+)',line)[1],16);original_edi=int(re.search(r' edi=(\w+)',line)[1],16)
    returns=re.findall(r'^pch-return pc=(\w+) eax=(\w+) esp=(\w+) edi=(\w+)$',text,re.M)
    s.require(returns==[('104385f3','104c64b8','%08x'%(original_esp+4),'%08x'%original_edi)] and 'pch-return-observer actual-caller=104385f3 byte=eb armed-at-member-entry=1\n' in text,'original PCH actual EAX/EDI/RET0 inherited return differs')
    s.require(raw('pch-return')==ret['frontend']['pch_context']==raw(final)==state['frontend']['pch_context']==zero,'PCH constructor did not clear all68 bytes')
    old=dict(entry['frontend']);new=dict(ret['frontend']);old.pop('pch_context');new.pop('pch_context')
    s.require(old==new,'PCH constructor changed other native modeled state')
    before=(original/'pch-bss-entry.bin').read_bytes();after=(original/'pch-bss-return.bin').read_bytes();offset=0xc64b8-0xb9000
    s.require(len(before)==len(after)==0xf69c and before[:offset]==after[:offset] and before[offset+68:]==after[offset+68:] and decode(before[offset:offset+68])==(seeded(owned) if aux else zero) and after[offset:offset+68]==bytes(68),'PCH constructor changed unrelated original BSS bytes')
    for frame in (entry,ret):s.require(frame['target_state']['last_error']==203 and frame['control_word']==0x27f,'PCH member changed caller context')
