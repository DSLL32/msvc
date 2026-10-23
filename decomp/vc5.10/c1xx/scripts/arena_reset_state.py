"""Nine actual arena receivers, x87 conversions, reservations and Reset returns."""
import copy,json,re,struct
from fractions import Fraction
from scaler_probe import describe,symbols as elf_symbols
from scaler_state import fp,semantic,value80
from allocation_imports import pe_read,export_rva


def check(s,d,original,native,trial,text,state,final,symbol_table,traps):
    def require(value,message):s.require(value,'arena Reset loop: '+message)
    def read(name):return json.loads((native/(name+'.json')).read_text())
    def one(name):
        rows=read(name);require(len(rows)==1,'wrong '+name+' inventory');return rows[0]
    def call(ret,destination):
        code=bytes(d.elf_byte(native,ret-5+i)for i in range(5));require(code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==destination,'not actual native CALL')
    names=elf_symbols(native/'native-executable.elf');metadata=read('scaler-probe-metadata');require(metadata==describe(native/'native-executable.elf'),'stale/guessed conversion instruction metadata')
    require(trial['trial'] in ('CompilerArena::Reset','_initialize_pool_arenas','CompilerPool::BindArena','CompilerPool::Refill') and 'import:_ftol' in trial['accepted'] and not trial.get('support_imports'),'not sole Reset candidate with durable compiler import')
    require(trial['arena_reset_observers']=={'entry_rva':0x38f0a,'return_rva':0x38ec8,'calls':9,'receiver_rvas':[0xc6500+i*32 for i in range(9)],'return_semantics':'void; caller overwrites EAX','full_original_bss_guard':True},'wrong declared Reset observer scope')
    rows=re.findall(r'^scaler point=(\d+) seq=(\d+) rva=(\w+) eax=(\w+) edx=(\w+) ecx=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) esp=(\w+) error=(\d+) cw=(\w+) sw=(\w+) tag=(\w+) stack=([0-9a-f,]+)$',text,re.M)
    points=[0x38ed7,0x38edd,0x38ee3,0x38ee9,0x38eee,0x38e81,0x38ea9,0x38eaf]
    require(len(rows)==72 and [(int(r[0]),int(r[1]),int(r[2],16))for r in rows]==[(j,i*8+j,rva)for i in range(9)for j,rva in enumerate(points)],'actual original 9x8 math/API point order differs')
    old=[]
    for i,row in enumerate(rows):
        item=dict(zip(('eax','edx','ecx','edi','esi','ebx','esp'),[int(v,16)for v in row[3:10]]));item.update(fp=fp((original/('scaler-fp-%03d.bin'%i)).read_bytes()),error=int(row[10]),stack=[int(v,16)for v in row[14].split(',')])
        require([int(v,16)&0xffff for v in row[11:14]]==[item['fp'][k]for k in ('cw','sw','tag')] and item['error']==203,'original raw arithmetic FP/error words differ');old.append(item)
    # This is the actual loaded hFile and resolved IAT, not a shipped-file guess.
    crt=original/'loaded-msvcrt.bin';require(s.sha(crt)=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210' and export_rva(crt,'_ftol')==0x4dc1,'wrong compiler provider hFile/export')
    module=int(re.search(r'^module name=MSVCRT.dll actual=(\w+) ',text,re.M|re.I)[1],16)
    require(re.findall(r'^ftol-provider iat=000c90c8 resolved=(\w+)$',text,re.M)==['%08x'%(module+0x4dc1)] and (original/'ftol-provider-code.bin').read_bytes()==pe_read(crt,0x4dc1,39),'actual _ftol provider/code differs')
    require(pe_read(original/'loaded-target.bin',0x38f04,6)==bytes.fromhex('ff25c8904c10') and pe_read(original/'loaded-target.bin',0xc93b0,8)==struct.pack('<d',-0.01),'original import thunk/point-of-use double constant differs')
    # The only supplied architecture is the independently reviewed empty entry.
    reference=s.sha(original/'receipt.json');binding=read('x87-entry-binding');initial=one('scaler-initial');profile=one('scaler-profile')
    require(binding['original_receipt_sha256']==reference and binding['original_directory']==read('system-info-binding')['original_directory'] and binding['original_float_save_sha256']==s.sha(original/'x87-context-000.bin') and binding['trial_sha256']==s.sha(original/'trial.json'),'entry profile closure differs')
    require((binding['guard_rva'],binding['cw'],binding['sw'],binding['tag'],binding['top'],binding['live_registers'])==(0x2e3c6,0x027f,0x0020,0xffff,0,[]) and s.sha(native/binding['factory_artifact'])==binding['factory_sha256'],'entry profile values/factory content differ')
    require(profile['pc']==names['recon_apply_x87_empty_entry_profile'][0] and profile['args']==[reference,0x027f,0x0020,0xffff] and profile['applications']==0 and semantic(profile['fp'])==dict(cw=0x027f,sw=0,tag=0xffff,top=0,live=[]) and profile['last_error']==203,'not actual one-time empty native prestate before target entry')
    require(initial['pc']==names['_DllMainCRTStartup'][0] and initial['args']==[state['registered_token'],1,0] and initial['return_pc'] and initial['esp'] and initial['profile']=={'reference':reference,'applications':1} and initial['last_error']==203 and semantic(initial['fp'])==semantic(fp((original/'x87-context-000.bin').read_bytes())),'actual first DLL entry/profile reference/count/ABI/FP words differ')
    inventories={name:read(name)for name in ('scaler-entries','scaler-returns','scaler-fpu','arena-api-entries','arena-api-returns','arena-reset-entries','arena-reset-returns','arena-member-entries')}
    require(all(len(v)==(90 if k=='scaler-fpu' else 9)for k,v in inventories.items()),'wrong repeated native inventory')
    old_api_before=re.findall(r'^arena-api-before address=(\w+) bytes=(\w+) kind=(\w+) protection=(\w+) provider=(\w+) esp=(\w+) error=(\d+)$',text,re.M)
    old_api_after=re.findall(r'^arena-api-after result=(\w+) esp=(\w+) error=(\d+) region-base=(\w+) bytes=(\w+) state=(\w+) protect=(\w+) allocation-protect=(\w+) type=(\w+)$',text,re.M)
    old_reset_entries=re.findall(r'^trap seq=(\d+) rva=00038f0a esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M)
    old_reset_returns=re.findall(r'^arena-reset-return index=(\d+) pc=(\w+) ecx=(\w+) eax=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) esp=(\w+) error=(\d+)$',text,re.M)
    old_members=re.findall(r'^trap seq=(\d+) rva=00038e70 esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=([0-9a-f,]+)$',text,re.M)
    require(len(old_api_before)==len(old_api_after)==len(old_reset_entries)==len(old_reset_returns)==len(old_members)==9,'wrong original receiver/API/Reset inventory')
    seeded=bool(trial.get('test_arena_reset_seed'))
    expected_seed={'point':'each actual Reset entry before body','cursor':'owned reserved base+4','committed_limit':'owned reserved base+8','committed_bytes':17,'reset_value':37,'other_fields_unchanged':True,'code_or_API_mock':False}
    require((not seeded and 'auxiliary_arena_reset_data_seed' not in trial) or trial.get('auxiliary_arena_reset_data_seed')==expected_seed,'undeclared Reset data seed')
    require(re.findall(r'^arena-reset-seed index=(\d+) cursor=base\+4 limit=base\+8 committed=17 reset=37 code-mock=0$',text,re.M)==([str(i)for i in range(9)]if seeded else []),'wrong original seed inventory')
    descriptors=list(struct.unpack('<45I',(original/'arena-descriptors-000.bin').read_bytes()));data=(original/'scaler-data-000.bin').read_bytes();percent=struct.unpack_from('<i',data,0x1c28)[0]
    first=read('arena-size-entry');arena_base=first['frontend']['storage']['arenas'][0];descriptor_base=first['frontend']['storage']['arena_descriptors'][0]
    native_arena_offset=arena_base-first['frontend']['storage']['saved_state'][0]
    require(native_arena_offset>=0 and first['frontend']['arena_native_layout']['size']==32,'wrong native arena layout')
    original_bss=bytearray((original/'arena-bss-member-frontier.bin').read_bytes());require(len(original_bss)==0xf69c,'wrong full original BSS size')
    native_guard=copy.deepcopy(inventories['arena-member-entries'][0]['guard']);native_bss=bytearray.fromhex(native_guard['bss'])
    original_offset=0xc6500-0xb9000
    expected_final=copy.deepcopy(first['frontend']);expected_final['arenas']=[0]*72
    native_owned=[];original_owned=[];results=[]
    kernel=int(re.search(r'^module name=KERNEL32.dll actual=(\w+) ',text,re.M)[1],16)
    member_pc,member_size=names['_ZN13CompilerArena7ReserveEPK15ArenaDescriptorPh'];reset_pc,reset_size=names['_ZN13CompilerArena5ResetEv'];api_pc=names['recon_VirtualAlloc'][0]
    def guard(row,bss):
        expected=copy.deepcopy(native_guard);expected['bss']=bytes(bss).hex();require(row['guard']==expected and row['last_error']==203,'native modeled/padding/saved/external guard or LastError changed')
    for i in range(9):
        q=i*8;old_math=old[q:q+8];entry=inventories['scaler-entries'][i];returned=inventories['scaler-returns'][i];records=inventories['scaler-fpu'][i*10:(i+1)*10]
        member=inventories['arena-member-entries'][i];before=inventories['arena-api-entries'][i];after=inventories['arena-api-returns'][i];reset=inventories['arena-reset-entries'][i];reset_return=inventories['arena-reset-returns'][i]
        desc=descriptors[i*5:(i+1)*5];reserve=struct.unpack('<i',struct.pack('<I',desc[1]))[0];unit=desc[2];kind=0x102000 if i==0 else 0x2000
        old_member=old_members[i];old_words=[int(v,16)for v in old_member[5].split(',')]
        require(int(old_member[3],16)==0x104c6500+i*32 and old_words[:3]==[0x10438e67 if i==0 else 0x10438e14,0x104d5af8+i*20,0],'original ordered Reserve receiver/stack2/caller differs')
        require(member['pc']==member_pc and member['ecx']==arena_base+i*32 and member['args']==[descriptor_base+i*20,0],'native ordered Reserve ECX/stack2 differs');guard(member,native_bss);call(member['return_pc'],member_pc)
        caller=names['initialize_default_arena' if i==0 else 'initialize_compiler_environment'];require(caller[0]<member['return_pc']<sum(caller),'wrong Reserve caller ownership')
        struct.pack_into('<I',original_bss,original_offset+i*32+16,0x104d5af8+i*20);struct.pack_into('<I',native_bss,native_arena_offset+i*32+16,descriptor_base+i*20)
        require((original/('scaler-bss-%03d.bin'%q)).read_bytes()==bytes(original_bss),'inter-iteration original BSS/descriptor store differs');guard(entry,native_bss)
        require(entry['pc']==metadata['start'] and entry['descriptor']==descriptor_base+i*20 and semantic(entry['fp'])==semantic(old_math[0]['fp']),'ordered calculator descriptor/entry FP differs')
        require([r['label']for r in records]==['instruction-%d'%j for j in range(9)]+['converted'] and [r['pc']for r in records[:9]]==[r['pc']for r in metadata['instructions']] and records[-1]['pc']==metadata['converted_pc'],'actual conversion instruction inventory differs')
        for a,b in [(0,0),(1,1),(3,2),(5,3),(9,4)]:require(semantic(records[a]['fp'])==semantic(old_math[b]['fp']),'actual percall live80/status/tag/depth mismatch')
        require(value80(old_math[1]['fp']['registers'][0])==reserve and value80(old_math[2]['fp']['registers'][0])==reserve*percent,'signed reserve/percent operands differ')
        require([len(r['fp']['live'])for r in records[:6]]==[0,1,2,1,2,1] and value80(records[2]['fp']['registers'][0])==percent and records[2]['fp']['registers'][1]==old_math[1]['fp']['registers'][0] and value80(records[4]['fp']['registers'][0])==Fraction.from_float(-0.01) and records[4]['fp']['registers'][1]==old_math[2]['fp']['registers'][0],'extra temporary operands differ')
        for j in (2,4):
            now,previous=records[j]['fp'],records[j-1]['fp'];require(now['cw']==previous['cw']==0x027f and now['sw']&~0x3800==previous['sw']&~0x3800 and now['top']==6 and now['tag']==0x0fff,'extra FP temporary status differs')
        require(records[6]['fp']['cw']==0x027f and records[7]['fp']['cw']==records[8]['fp']['cw']==0x0e7f,'truncating control differs')
        converted=records[-1];qword=bytes.fromhex(converted['qword_bytes']);require(converted['qword_address']==converted['ebp']+metadata['qword_ebp_displacement'] and len(qword)==8 and struct.unpack('<Q',qword)[0]==(old_math[4]['edx']<<32)|old_math[4]['eax'] and struct.unpack('<q',qword)[0]==int(value80(old_math[3]['fp']['registers'][0])),'signed QWORD conversion/result location differs')
        rounded=(unit-(struct.unpack('<Q',qword)[0]&0xffffffff)-1)&(~(unit-1)&0xffffffff);rounded=rounded or unit
        require(returned['eax']==old_math[5]['eax']==rounded and returned['pc']==entry['return_pc'] and returned['esp']==entry['esp']+4 and all(returned[r]==entry[r]for r in ('edi','esi','ebx')) and semantic(returned['fp'])==semantic(old_math[5]['fp'])==semantic(converted['fp']),'calculator whole return/RET0/nonvolatile differs');call(returned['pc'],metadata['start'])
        for r in records+[returned]:guard(r,native_bss)
        for j in range(8):
            if j<6:require((original/('scaler-bss-%03d.bin'%(q+j))).read_bytes()==bytes(original_bss),'math changed full original BSS')
            for name in ('data','saved','support'):require((original/('scaler-%s-%03d.bin'%(name,q+j))).read_bytes()==(original/('scaler-%s-000.bin'%name)).read_bytes(),'math/API changed initialized/saved/support storage')
        struct.pack_into('<I',original_bss,original_offset+i*32+20,rounded);struct.pack_into('<I',native_bss,native_arena_offset+i*32+20,rounded)
        require(before['pc']==api_pc and before['args']==[0,rounded,kind,4] and member_pc<before['return_pc']<member_pc+member_size and after['pc']==before['return_pc'] and after['esp']==before['esp']+20 and all(after[r]==before[r]for r in ('edi','esi','ebx')),'actual reservation API args/caller/RET16 differs');call(before['return_pc'],api_pc)
        b,a=old_api_before[i],old_api_after[i];require([int(v,16)for v in b[:4]]==[0,rounded,kind,4] and int(b[4],16)==kernel+export_rva(original/'loaded-kernel32.bin','VirtualAlloc') and int(a[1],16)==int(b[5],16)+16 and int(b[6])==int(a[2])==203,'original API provider/args/RET16/error differs')
        old_base=int(a[0],16);new_base=after['eax'];require(old_base and new_base and (i!=0 or not(old_base%65536 or new_base%65536)),'reservation result/TOP_DOWN alignment differs')
        require(struct.unpack('<7I',(original/('arena-region-%03d.bin'%i)).read_bytes())==(old_base,old_base,4,rounded,0x2000,0,0x20000) and [int(v,16)for v in a[3:]]==[old_base,rounded,0x2000,0,4,0x20000],'original reserved region differs')
        require(after['owned_reservations']==[{'address':new_base,'bytes':rounded}],'native ownership size differs')
        mappings=[]
        for line in after['maps'].splitlines():
            cols=line.split();lo,hi=[int(v,16)for v in cols[0].split('-')]
            if lo<=new_base<hi:mappings.append((lo,hi,cols[1]))
        require(len(mappings)==1 and mappings[0][1]>=new_base+rounded and mappings[0][2]=='---p','native reservation is not wholly opaque PROT_NONE')
        require(all(not(new_base<end and start<new_base+rounded)for start,end in native_owned) and all(not(old_base<end and start<old_base+rounded)for start,end in original_owned),'reservation identities overlap')
        native_owned.append((new_base,new_base+rounded));original_owned.append((old_base,old_base+rounded))
        guard(before,native_bss);guard(after,native_bss);require(semantic(before['fp'])==semantic(after['fp'])==semantic(returned['fp']),'API changed actual FP state')
        require((original/('scaler-bss-%03d.bin'%(q+6))).read_bytes()==(original/('scaler-bss-%03d.bin'%(q+7))).read_bytes()==bytes(original_bss),'size/API changed unrelated original BSS')
        struct.pack_into('<I',original_bss,original_offset+i*32+8,old_base);struct.pack_into('<I',native_bss,native_arena_offset+i*32+8,new_base)
        if seeded:
            for bss,offset,base in [(original_bss,original_offset,old_base),(native_bss,native_arena_offset,new_base)]:
                for member_offset,value in [(0,base+4),(4,base+8),(12,17),(24,37)]:struct.pack_into('<I',bss,offset+i*32+member_offset,value)
        old_entry=old_reset_entries[i];old_return=old_reset_returns[i];old_entry_stack=[int(v,16)for v in old_entry[5].split(',')]
        require(int(old_entry[3],16)==0x104c6500+i*32 and old_entry_stack[0]==0x10438ec8 and int(old_return[0])==i and int(old_return[1],16)==0x10438ec8 and int(old_return[7],16)==int(old_entry[1],16)+4 and int(old_return[8])==203 and int(old_return[4],16)==int(old_entry[4],16) and [int(old_return[j],16)for j in (5,6)]==[old_math[7][r]for r in ('esi','ebx')],'actual original Reset ECX/RET0/EDI/error differs')
        require(reset['pc']==reset_pc and reset['ecx']==arena_base+i*32 and reset['formal_inventory']==[{'name':'this','type':'CompilerArena *'}] and member_pc<reset['return_pc']<member_pc+member_size and reset_return['pc']==reset['return_pc'] and reset_return['esp']==reset['esp']+4 and all(reset_return[r]==reset[r]for r in ('edi','esi','ebx')),'actual native Reset receiver/formals/RET0/nonvol differs');call(reset['return_pc'],reset_pc)
        require((original/('reset-entry-bss-%03d.bin'%i)).read_bytes()==bytes(original_bss),'original Reset entry/nonzero seed differs');guard(reset,native_bss)
        for bss,offset,base in [(original_bss,original_offset,old_base),(native_bss,native_arena_offset,new_base)]:
            for member_offset,value in [(0,base),(4,base),(12,0),(24,0)]:struct.pack_into('<I',bss,offset+i*32+member_offset,value)
        require((original/('reset-return-bss-%03d.bin'%i)).read_bytes()==bytes(original_bss),'Reset changed full original BSS beyond exact four fields');guard(reset_return,native_bss)
        for name in ('data','saved','support'):require((original/('reset-return-%s-%03d.bin'%(name,i))).read_bytes()==(original/('scaler-%s-000.bin'%name)).read_bytes(),'Reset changed original initialized/saved/support state')
        require(semantic(fp((original/('x87-context-%03d.bin'%int(old_entry[0]))).read_bytes()))==semantic(fp((original/('reset-return-fp-%03d.bin'%i)).read_bytes()))==semantic(reset['fp'])==semantic(reset_return['fp'])==semantic(returned['fp']),'Reset changed FP state')
        expected_final['arenas'][i*8:i*8+8]=[new_base,new_base,new_base,0,descriptor_base+i*20,rounded,0,0];results.append({'index':i,'signed_qword':struct.unpack('<q',qword)[0],'rounded':rounded,'allocation_kind':kind})
    pool=one('arena-pool-entries');pool_pc,pool_size=names['initialize_pool_arenas'];wrapper_pc,wrapper_size=names['initialize_compiler_environment']
    require((trial['trial']!='CompilerArena::Reset' or state['symbol']=='initialize_pool_arenas' and state['args']=={}) and pool['pc']==pool_pc and pool['args']==[] and pool['formal_inventory']==[] and wrapper_pc<pool['return_pc']<wrapper_pc+wrapper_size,'actual final noarg frontier/formals/caller differs');call(pool['return_pc'],pool_pc);guard(pool,native_bss)
    require(int(next(row[2]for row in traps if row[1]=='00038f1b').split(',')[0],16)==0x10438e2f,'original next pool initializer caller differs')
    loop_state=copy.deepcopy(state['frontend'])
    if 'CompilerPool::BindArena' in trial['accepted']:loop_state['pools'][9]['arena']=0
    require((original/('arenas-'+final+'.bin')).read_bytes()==bytes(original_bss[original_offset:original_offset+288]) and loop_state==expected_final,'final typed arenas/neighbors differ')
    require(semantic(pool['fp'])==semantic(inventories['arena-reset-returns'][-1]['fp']),'loop continuation changed FP state')
    # No EAX return value is claimed for void Reset; the original caller overwrites it.
    return {'calls':9,'nonzero_reset_inputs':seeded,'conversions':results,'regular_reserve_alignment_claimed':False,'opaque_reserved_bytes':sum(r['rounded']for r in results)}
