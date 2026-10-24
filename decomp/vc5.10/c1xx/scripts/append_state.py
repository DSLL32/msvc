"""Six actual vector appends and the ordered option-loop continuation.

The previously validated prefix ends at the first append entry. Every later
entry/return is checked against explicit semantic writes, including complete
owned32KiB allocation state; unmodeled original bytes are preservation guards.
"""
import copy
import json
import re
import struct
from pathlib import Path

from allocation_imports import pe_read
from option_action_state import check_signature
from scaler_probe import symbols
from scaler_state import fp, semantic


def check(s,d,original,native,trial,text,state,prefix):
    def need(ok,message):s.require(ok,'option append: '+message)
    def load(name):return json.loads((native/(name+'.json')).read_text())
    def reg(name,count=16):return struct.unpack('<%dI'%count,(original/(name+'-registers.bin')).read_bytes())
    def raw(name,part):return (original/('locale-'+name+'-'+part+'.bin')).read_bytes()
    def argv(path):
        blob=path.read_bytes();root,count=struct.unpack_from('<II',blob);at=8;rows=[]
        for _ in range(count):
            pointer,length=struct.unpack_from('<II',blob,at);at+=8;rows.append((pointer,blob[at:at+length]));at+=length
        need(count==26 and at==len(blob),'whole separate Invoke vector');return root,rows
    contract={'entry_rva':0x100ea,'return_rva':0x396bc,'calls':6,'vector_roots':['defines']*5+['include_directories'],'values':[{'Invoke_slot':a,'offset':b}for a,b in [(8,2),(11,2),(12,2),(18,2),(19,2),(25,0)]],'grow_calls':0,'header_bytes':12,'items_bytes':160,'committed_bytes':32768,'frontier_rva':0x397db,'frontier_caller_rva':0x390f9,'frontier_formals':['char *output','const char *input','unsigned capacity'],'frontier_capacity':520,'scope':'six actual append entry/RET pairs, all original/native ordered options/public calls, no grow or resolver body credit'}
    need(trial['trial']=='_append_option_vector'and trial['append_observers']==contract,'exact candidate and observation contract')
    need([r for r in trial['ranges']if r['owner']=='_append_option_vector']==[{'owner':'_append_option_vector','start_rva':0x100ea,'end_rva':0x10115,'entry':1}],'complete43B extent')
    names=symbols(native/'native-executable.elf');seed=bool(trial.get('test_seed'))
    oldroot,oldargs=argv(original/'invoke-entry-argv.bin');root,args=argv(native/'first-append-argv.bin')
    need([b for _,b in args]==[b for _,b in oldargs],'whole argument strings')
    first=load('first-append-state');front=first['frontend'];expected_front=copy.deepcopy(front);expected=copy.deepcopy(load('append-entries')[0]['guard'])
    firstphase='append-0-entry';old_bss=bytearray(raw(firstphase,'target-BSS'));old_data=bytearray((original/('frontend-data-locale-'+firstphase+'.bin')).read_bytes());old_saved=(original/('frontend-snapshot-locale-'+firstphase+'.bin')).read_bytes()
    fields={r['name']:r for r in load('state-model')['fields']};floating=semantic(load('append-entries')[0]['fp']);locale={k:v for k,v in load('append-entries')[0]['locale'].items()if k!='maps'}
    old_buffer=bytearray((original/'suffix-append-committed.bin').read_bytes());buffer=bytearray.fromhex(load('append-entries')[0]['committed'])
    arena_old=struct.unpack_from('<I',old_bss,0xc6528-0xb9000)[0];arena=load('append-entries')[0]['arena'][2]
    grammar=json.loads((Path(__file__).resolve().parents[1]/'cases/minimal/options-analysis.json').read_text());rules=load('options-grammar-layout')['option_rules']['address']
    arrays={k:load('actions-'+k)for k in ('matches','match-returns','entries','returns','outer-entries','outer-returns','inner-entries','inner-returns','signatures','signature-returns')}
    need([len(arrays[k])for k in arrays]==[21,21,20,20,21,21,5,5,23 if seed else 24,23 if seed else 24],'complete loop counts')
    appends,append_returns,resolvers=load('append-entries'),load('append-returns'),load('append-resolvers')
    integers,integer_returns=load('integer-entries'),load('integer-returns');apis,api_returns=load('integer-api-entries'),load('integer-api-returns');suffixes,suffix_returns=load('suffix-entries'),load('suffix-returns')
    need([len(x)for x in (appends,append_returns,resolvers,integers,integer_returns,apis,api_returns,suffixes,suffix_returns)]==[6,6,1,5,5,15,15,3,3],'complete per-function inventory')
    alias=load('suffix-append-entries');need(len(alias)==1 and all(appends[0].get(k)==v for k,v in alias[0].items()),'same actual first-append checkpoint/owners/vector/state as validated suffix prefix')
    a0=reg('append-0-entry',18);p0=reg('actions-append-entry-0');need(a0[:10]==p0[:10]and a0[10:13]==p0[10:13],'original same actual first-append prefix context')
    outer_slots=[1,3,5,7,8,9,10,11,12,13,14,15,17,18,19,20,21,22,23,24,None];inner_slots=[2,4,6,16,25]
    # This finite list follows the independently recovered100-record grammar.
    rule_indices=[]
    for slot in outer_slots[:-1]:
        value=args[slot][1][:-1].decode('latin1');matches=[]
        for i,rule in enumerate(grammar['rules']):
            pattern=rule['pattern'];plain=pattern.replace('%','').replace('$','#')
            if plain.endswith(('#','*')):
                if value.startswith(plain[:-1]):matches.append((len(plain[:-1]),i))
            elif value==plain:matches.append((len(plain)+1,i))
        need(matches,'finite grammar owner for actual option');rule_indices.append(max(matches)[1])
    callsites={}
    def call(pc,target):
        prior=callsites.get(pc)
        if prior is None:
            code=bytes(d.elf_byte(native,pc-5+i)for i in range(5));need(code[0]==0xe8 and pc+struct.unpack_from('<i',code,1)[0]==target,'actual immutable native call encoding');callsites[pc]=target
        else:need(prior==target,'same actual callsite target')
    def returned(entry,end,a,b):
        need(b[1]==a[10]and b[2]==a[2]+4 and b[6:10]==a[6:10]and end['pc']==entry['return_pc']and end['entry_esp']==entry['esp']and end['esp']==entry['esp']+4 and all(end[k]==entry[k]for k in ('ebx','esi','edi','ebp')),'actual RET0/stack/nonvolatile ABI')
    def write(name,old,value):
        field=fields[name];kind='data'if field['class']=='initialized'else'bss';base=front['storage']['first_invocation'][0]if kind=='data'else front['bss_address'];buf=bytearray.fromhex(expected[kind]);struct.pack_into('<I',buf,front['storage'][name][0]-base,value);expected[kind]=buf.hex();struct.pack_into('<I',old_data if kind=='data'else old_bss,field['original_address']-(0x104cc000 if kind=='data'else 0x104b9000),old)
        if name.startswith('options.'):
            expected_front['options_data' if kind=='data'else 'options_bss'][name[8:]]=value
        else:expected_front[name]=value
    events=[]
    def add(kind,entries,ends,stem,counts=16):
        for index,(entry,end)in enumerate(zip(entries,ends)):
            for phase,row in [('entry',entry),('return',end)]:
                name=stem(index,phase);record=reg(name,counts);events.append((record[0],kind,index,phase,name,row,record))
    for kind,a,b in [('outer',arrays['outer-entries'],arrays['outer-returns']),('inner',arrays['inner-entries'],arrays['inner-returns']),('match',arrays['matches'],arrays['match-returns']),('action',arrays['entries'],arrays['returns'])]:add(kind,a,b,lambda i,p,k=kind:'actions-%s-%s-%d'%(k,p,i))
    add('signature',arrays['signatures'],arrays['signature-returns'],lambda i,p:'actions-signature-%d-%s'%(i,p),17)
    add('append',appends,append_returns,lambda i,p:'append-%d-%s'%(i,p),18)
    add('integer',integers,integer_returns,lambda i,p:'integer-%d-%s'%(i,p))
    add('suffix',suffixes,suffix_returns,lambda i,p:'actions-suffix-%s-%d'%(p,i))
    for j,(entry,end)in enumerate(zip(apis,api_returns)):
        for phase,row in [('entry',entry),('return',end)]:
            name='integer-%d-api-%s-%s'%(j//3,entry['kind'],phase);record=reg(name,17);events.append((record[0],'api',j,phase,name,row,record))
    start=reg('append-0-entry',18)[0];events=sorted(x for x in events if x[0]>=start)
    need(len({x[0]for x in events})==len(events),'one real primary record per ordered checkpoint')
    def phase(name,row):
        need(row['guard']==expected and raw(name,'target-BSS')==bytes(old_bss)and(original/('frontend-data-locale-'+name+'.bin')).read_bytes()==bytes(old_data)and(original/('frontend-snapshot-locale-'+name+'.bin')).read_bytes()==old_saved,'whole modeled/padding/unknown/saved state '+name)
        need(row['last_error']==203 and semantic(row['fp'])==floating and{k:v for k,v in row['locale'].items()if k!='maps'}==locale,'full native owners/locale/errors/architectural FP '+name)
        for part in ('words','mbctype','ctype','old-ctype','result','errors','fp','CRT-argv','CRT-environment','Invoke-argv'):need(raw(name,part)==raw(firstphase,part),'full original provider/vector/error/112FP '+name+':'+part)
        native_name=re.sub(r'^actions-(.*)-return-(\d+)$',r'actions-\1-\2-return',name)
        for part,baseline in [('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')]:need((native/('locale-native-'+native_name+'-'+part+'.bin')).read_bytes()==(native/baseline).read_bytes(),'native private lifetime '+name)
    numeric_values=[1,8,64,5,0];numeric_slots=[(6,0),(9,3),(10,3),(13,2),(21,3)]
    append_roles=[(8,2),(11,2),(12,2),(18,2),(19,2),(25,0)]
    for sequence,kind,index,which,name,row,r in events:
        before=which=='entry'
        if kind in ('outer','inner')and not before:
            slot=(outer_slots if kind=='outer'else inner_slots)[index];argc=struct.unpack_from('<I',old_bss,fields['command_argc']['original_address']-0x104b9000)[0]-1
            write('command_argc',argc,argc)
            if slot is not None:write('command_argv',oldroot+slot*4,root+slot*4)
        if kind=='action'and not before:
            rule=grammar['rules'][rule_indices[index]];action=rule['action'];ae=arrays['entries'][index]
            if action in (1,5,34,36):
                field=next((f for f in fields.values()if f['original_address']==0x10400000+rule['destination_rva']),None);need(field is not None,'actual action destination maps a known field')
                if action in (1,5):old=value=1 if action==1 else 0
                elif action==34:old=reg('actions-action-entry-%d'%index)[12];value=ae['args'][1]
                else:
                    choices=[j for j,e in enumerate(integers)if e['args']==[ae['args'][1]]];need(len(choices)==1,'unique actual numeric action');old=value=numeric_values[choices[0]]
                write(field['name'],old,value)
            elif action==35:write('options.language_extensions',1,1)
            else:need(action==38,'only encountered actual action domain')
        if kind=='append'and not before:
            entry=appends[index];oldentry=reg('append-%d-entry'%index,18);slot,offset=append_roles[index]
            for buf,base,vector,value in [(old_buffer,arena_old,oldentry[11],oldargs[slot][0]+offset),(buffer,arena,entry['args'][0],args[slot][0]+offset)]:
                at=vector-base;count,capacity,owner,items=struct.unpack_from('<hhiI',buf,at);need(count<capacity==40 and owner==1,'signed-short count/capacity no growth');struct.pack_into('<I',buf,items-base+count*4,value);struct.pack_into('<h',buf,at,count+1)
        phase(name,row)
        if kind in ('outer','inner'):
            entries=arrays[kind+'-entries'];ends=arrays[kind+'-returns'];entry=entries[index];slot=(outer_slots if kind=='outer'else inner_slots)[index]
            if before:
                need(r[1]==0x10439428 and entry['pc']==names['next_argument'][0]and entry['args']==[]and entry['formal_inventory']==[]and entry['caller']==('process_compiler_options'if kind=='outer'else'match_option'),'actual no-formal iterator')
                if kind=='outer':call(entry['return_pc'],entry['pc'])
                else:need(bytes(d.elf_byte(native,entry['return_pc']-3+j)for j in range(3))==b'\xff\x55\x10','actual indirect matcher callback through next formal')
            else:
                returned(entry,row,reg('actions-%s-entry-%d'%(kind,index)),r);need(r[3]==(oldargs[slot][0]if slot is not None else 0)and row['eax']==(args[slot][0]if slot is not None else 0)and row['value']==(args[slot][1].hex()if slot is not None else''),'borrowed iterator return/NULL')
        elif kind=='match':
            entry=arrays['matches'][index]
            if before:
                slot=outer_slots[index];need(r[1]==0x1043944a and r[11:15]==(0x104d6160,oldargs[slot][0]if slot is not None else 0,0x10439428,0)and entry['args']==[rules,args[slot][0]if slot is not None else 0,names['next_argument'][0],0]and entry['formal_inventory']==[{'name':k,'type':v}for k,v in [('rules','CompilerOptionRule *'),('argument','char *'),('next','char *(*)(void)'),('copy','int')]]and entry['caller']=='process_compiler_options','actual matcher four slots');call(entry['return_pc'],entry['pc'])
                if slot is not None:write('current_argument',oldargs[slot][0],args[slot][0])
            else:returned(entry,row,reg('actions-match-entry-%d'%index),r);need(r[3]==row['eax']==(grammar['rules'][rule_indices[index]]['result']if index<20 else 0),'actual matcher grammar result')
        elif kind=='action':
            entry=arrays['entries'][index];rule=rule_indices[index]
            if before:
                need(r[1]==0x10439632 and r[11]==0x104d6160+12*rule and r[13]==0 and entry['args'][0]==rules+12*rule and entry['args'][2]==0 and r[10]in(0x10439589,0x104395cb,0x1043953c)and entry['formal_inventory']==[{'name':k,'type':v}for k,v in [('rule','CompilerOptionRule *'),('value','char *'),('copy','int')]]and entry['caller']=='match_option','actual action rule/copy formals');call(entry['return_pc'],entry['pc'])
                value=bytes.fromhex(entry['value']);matches=[(j,p)for j,(_,v)in enumerate(args)for p in range(len(v))if args[j][0]+p==entry['args'][1]and v[p:]==value]
                if entry['args'][1]:need(len(matches)==1 and r[12]==oldargs[matches[0][0]][0]+matches[0][1],'actual borrowed action value owner')
                else:need(r[12]==0 and value==b'','actual NULL action value')
            else:returned(entry,row,reg('actions-action-entry-%d'%index),r)
        elif kind=='append':
            entry=appends[index];a=reg('append-%d-entry'%index,18);slot,offset=append_roles[index]
            need(r[13]==index and r[14]==0x10400000 and r[16:]==a[11:13],'actual append index/physical slots lifetime')
            if before:
                oldvec=struct.unpack_from('<I',old_bss,(0xc06c4 if index<5 else 0xbf6f0)-0xb9000)[0];nv=front['options_bss']['defines'if index<5 else'include_directories']
                need(r[1]==0x104100ea and r[10:13]==(0x104396bc,oldvec,oldargs[slot][0]+offset)and entry['pc']==names['append_option_vector'][0]and entry['args']==[nv,args[slot][0]+offset]and entry['formal_inventory']==[{'name':'vector','type':'CompilerVector *'},{'name':'value','type':'char *'}]and entry['caller']=='apply_option_action','actual complete append ABI');call(entry['return_pc'],entry['pc'])
            else:returned(entry,row,a,r)
            for buf,base,pointer,header,items,committed in [(old_buffer,arena_old,a[11],(original/('append-%d-%s-header.bin'%(index,which))).read_bytes(),(original/('append-%d-%s-items.bin'%(index,which))).read_bytes(),(original/('append-%d-%s-committed.bin'%(index,which))).read_bytes()),(buffer,arena,entry['args'][0],bytes.fromhex(row['vector_header']),bytes.fromhex(row['items']),bytes.fromhex(row['committed']))]:
                h=bytes(buf[pointer-base:pointer-base+12]);array=struct.unpack_from('<I',h,8)[0];need(header==h and items==buf[array-base:array-base+160]and committed==buf,'entire32KiB only actual item/count writes')
            need(row['value']==args[slot][1][offset:].hex()and(original/('append-%d-%s-input.bin'%(index,which))).read_bytes()==args[slot][1][offset:],'whole append borrowed input')
        elif kind=='signature':
            if before:
                entry=arrays['signatures'][index];slots=[j for j,(p,v)in enumerate(args)if p==entry['args'][0]];need(len(slots)==1,'actual public borrowed input owner');slot=slots[0]
                check_signature(index,slot,r[10]-0x10400000,expected,old_bss,old_data,old_saved,arrays['signatures'],arrays['signature-returns'],original,native,names,need,locale,floating,raw,firstphase,d,args,oldargs)
            else:write('source_signature',r[3],row['eax'])
        elif kind in ('integer','suffix'):
            entry=(integers if kind=='integer'else suffixes)[index];a=reg(('integer-%d-entry'%index)if kind=='integer'else'actions-suffix-entry-%d'%index)
            if before:
                if kind=='integer':
                    need(a[1]==0x10439779 and entry['pc']==names['parse_option_integer'][0]and entry['formal_inventory']==[{'name':'value','type':'const char *'}]and entry['caller']=='apply_option_action','actual numeric complete function ABI')
                    slot,offset=numeric_slots[index];need(a[10:12]==(0x10439694,oldargs[slot][0]+offset)and entry['args']==[args[slot][0]+offset]and entry['value']==str(numeric_values[index]).encode().hex()+'00','actual numeric complete input')
                else:
                    need(a[1]==0x10439757 and entry['pc']==names['match_option_suffix'][0]and entry['formal_inventory']==[{'name':'pattern','type':'const char *'},{'name':'argument','type':'char *'},{'name':'action','type':'unsigned int'}]and entry['caller']=='match_option','actual suffix complete function ABI')
                    rule,slot,offset,action=[(29,7,2,35),(28,10,3,36),(48,23,3,34)][index];pattern=grammar['rules'][rule]['original_pattern_rva']+len(grammar['rules'][rule]['pattern'])-1
                    need(a[10:14]==(0x104394cf,0x10400000+pattern,oldargs[slot][0]+offset,action)and entry['args'][1:]==[args[slot][0]+offset,action]and entry['pattern']=='2a00'and entry['argument']==args[slot][1][offset:].hex(),'actual three suffix formals')
                call(entry['return_pc'],entry['pc'])
            else:returned(entry,row,a,r);need(r[3]==row['eax']==(numeric_values[index]if kind=='integer'else 1),'actual numeric/suffix result')
        elif kind=='api':
            entry=apis[index];a=reg('integer-%d-api-%s-entry'%(index//3,entry['kind']),17);which_api=index%3;slot,offset=numeric_slots[index//3];argument=oldargs[slot][0]+offset if which_api==2 else args[slot][1][offset];nativearg=args[slot][0]+offset if which_api==2 else argument
            need(a[1]==0x78000000+[0x1c0be,0x1c065,0x5d30][which_api]and entry['pc']==apis[which_api]['pc']and entry['provider']==apis[which_api]['provider']and entry['provider_entry_bytes']==apis[which_api]['provider_entry_bytes']and entry['formal_inventory'] is None,'actual original canonical export and same observed native named libc entry')
            need(r[10]==0x10400000+[0x39793,0x397a9,0x397bb][which_api]and r[11]==argument and entry['args']==[nativearg]and entry['caller']=='parse_option_integer'and entry['locale']['mb_cur_max']==1,'actual finite public numeric ABI/prestate')
            if not before:
                returned(entry,row,a,r);need(row['argument_after']==nativearg,'public physical slot preserved')
                if which_api==2:need(r[3]==row['eax']==numeric_values[index//3]and entry['input']==row['input']==args[slot][1][offset:].hex(),'exact signed32 atol')
                else:
                    table=raw('integer-%d-api-%s-entry'%(index//3,entry['kind']),'ctype');mask=8 if which_api==0 else 4
                    need(r[3]==(struct.unpack_from('<H',table,2*(argument+1))[0]&mask)and bool(r[3])==bool(row['eax'])==(which_api==1),'original raw mask and sole consumed ctype nonzero predicate')
    final=resolvers[0];r=reg('actions-resolver-entry-0');phase('actions-resolver-entry-0',final)
    need(r[1]==0x104397db and r[10]==0x104390f9 and r[11]==r[2]+32 and r[12:14]==(oldargs[17][0]+3,520)and final['pc']==names['resolve_option_path'][0]and final['args']==[final['output_owner']['address'],args[17][0]+3,520]and final['output_owner']=={'caller':'process_compiler_options','address':final['args'][0],'bytes':520}and final['formal_inventory']==[{'name':'output','type':'char *'},{'name':'input','type':'const char *'},{'name':'capacity','type':'unsigned int'}]and final['input']==b'vc50.pdb\0'.hex(),'genuine resolver three formals and uninitialized caller-output identity')
    need(bytes.fromhex(final['committed'])==buffer and(original/'append-final-committed.bin').read_bytes()==old_buffer,'whole final allocation')
    need(state['symbol']=='resolve_option_path'and state['args']==dict(zip(('output','input','capacity'),final['args']))and names['resolve_option_path'][0]<state['pc']<=sum(names['resolve_option_path'])and d.elf_byte(native,state['pc']-1)==0xcc,'actual native named INT3 resolver')
    tail=re.findall(r'^trap seq=(\d+) rva=000397db esp=(\w+).*stack=([0-9a-f,]+)$',text,re.M);need(len(tail)==1 and int(tail[0][0])+1==r[0]and int(tail[0][1],16)==r[2]and[int(x,16)for x in tail[0][2].split(',')][:4]==list(r[10:14])and re.findall(r'^frontier rva=(\w+)$',text,re.M)==['000397db'],'actual sole final original trap')
    seq=int(tail[0][0]);need((original/('pool-bss-%03d.bin'%(seq+1))).read_bytes()==old_bss and(original/('frontend-data-%03d.bin'%seq)).read_bytes()==old_data and(original/('frontend-snapshot-%03d.bin'%seq)).read_bytes()==old_saved and semantic(fp((original/('x87-context-%03d.bin'%seq)).read_bytes()))==floating,'direct final complete original guard')
    need(state['frontend']==expected_front and state['invocation']==first['invocation'] and state['target_state']==first['target_state'],'whole final typed frontend/Invoke/lifecycle state')
    need(state['frontend']['command_argc']==0 and state['frontend']['command_argv']==root+100 and state['frontend']['current_argument']==args[24][0],'whole option loop final cursor/borrowed lifetime')
    return dict(frontier='_resolve_option_path',candidate='_append_option_vector',code_bytes=43,appends=6,defines=5,includes=1,grow_calls=0,owned_allocation_bytes=32768,full_option_inventory={'matches':21,'actions':20,'outer':21,'inner':5,'numeric':5,'suffix':3,'CRC':23 if seed else 24},new_fields=0,unknown_initialized_bytes=6031,CRT_credit=0,prefix=prefix)
