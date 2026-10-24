"""Three real vectors, arena1 commitment and the genuine provider entry."""
import copy,json,re,struct
from scaler_probe import symbols
from scaler_state import fp,semantic
from allocation_imports import pe_read,export_rva


def check(s,d,original,native,trial,text,state,prefix):
    def need(ok,message):s.require(ok,'vector allocation: '+message)
    def rows(name):return json.loads((native/('vector-'+name+'.json')).read_text())
    def one(name):
        result=rows(name);need(len(result)==1,'one actual '+name);return result[0]
    def raw(phase,kind):return(original/('locale-'+phase+'-'+kind+'.bin')).read_bytes()
    def data(phase,saved=False):return(original/('frontend-'+('snapshot'if saved else'data')+'-locale-'+phase+'.bin')).read_bytes()
    def registers(name,count):
        result=(original/name).read_bytes();need(len(result)==count*4,'exact original register record');return struct.unpack('<%dI'%count,result)
    names=symbols(native/'native-executable.elf');seed=bool(trial.get('test_seed'))
    need(trial['trial']=='_new_option_vector'and trial['vector_observers']=={'entry_rva':0x682f,'calls':3,'arguments':[40,1],'header_bytes':12,'items_bytes':160,'arena_index':1,'committed_bytes':32768,'normal_frontier':'public:mspdb50!SigForPbCb','restore_frontier':'_next_argument','provider_execution':False},'exact application candidate/observer scope')
    entries,returns=rows('entries'),rows('returns');need(len(entries)==len(returns)==3,'complete three-call inventory')
    need(prefix['symbol']=='new_option_vector'and prefix['args']=={'capacity':40,'arena_index':1}and prefix['pc']==entries[0]['pc']and prefix['esp']==entries[0]['esp'],'actual structured pre-body checkpoint identity')
    need(prefix['frontend']['arenas'][8:16]==entries[0]['arena'],'actual structured checkpoint/arena1 input')
    preceding=json.loads((native/'env-arguments-next.json').read_text())
    need(len(preceding)==1 and preceding[0]['pc']==entries[0]['pc']and preceding[0]['esp']==entries[0]['esp']and preceding[0]['guard']==entries[0]['guard']and preceding[0]['locale']==entries[0]['locale'],'same actual pre-body phase and already-validated provider/vector owner identity')
    source=original/'loaded-target.bin';first=entries[0];base=first['arena'][2];arena_address=first['arena_address'];old_arena=0xc6520;old_bss_base=0xb9000;old_arena_offset=old_arena-old_bss_base
    old_first=raw('vector-0-entry','target-BSS');old_base=struct.unpack_from('<I',old_first,old_arena_offset+8)[0]
    need(first['arena'][:4]==[base,base,base,0]and base and old_base and base%65536==old_base%65536==0,'own fresh arena cursor/reservation identity')
    need(all(row['arena_address']==arena_address for row in entries+returns),'stable actual arena1 receiver')
    warning_input=first['guard'];old_data=data('vector-0-entry');old_saved=data('vector-0-entry',True)
    expected_native=copy.deepcopy(first['guard']);expected_old=bytearray(old_first)
    old_objects=[old_base+172*i for i in range(3)];objects=[base+172*i for i in range(3)]
    pointer_names=['defines','include_directories','forced_includes'];old_pointer_rvas=[0xc06c4,0xbf6f0,0xc0584]
    storage=prefix['frontend']['storage'];offsets=[storage['options.'+name][0]-prefix['frontend']['bss_address']for name in pointer_names]
    member,member_return=one('member-entries'),one('member-returns');api,api_return=one('api-entries'),one('api-returns')
    initial_fp=semantic(fp(raw('vector-0-entry','fp')))
    def locale_value(row):return {k:v for k,v in row['locale'].items()if k!='maps'}
    def mapping(row,committed):
        covered=0;reserve=first['arena'][5]
        for line in row['locale']['maps'].splitlines():
            fields=line.split();lo,hi=[int(v,16)for v in fields[0].split('-')];lo,hi=max(lo,base),min(hi,base+reserve)
            if lo>=hi:continue
            need(lo==base+covered,'complete actual ordered reservation mapping')
            if committed and lo<base+32768:need(hi<=base+32768 and fields[1]=='rw-p','exact32KiB writable committed mapping')
            else:need(fields[1]=='---p','remaining reserved bytes are inaccessible')
            covered+=hi-lo
        need(covered==reserve,'complete reservation mapping coverage')
    def stable_locale(phase,row):
        need(row['last_error']==203 and semantic(row['fp'])==initial_fp and locale_value(row)==locale_value(first),'native full architectural FP/errors/provider owners/tables/vectors')
        for suffix in ('words','mbctype','ctype','old-ctype','result','fp','errors','CRT-argv','CRT-environment','Invoke-argv'):
            need(raw(phase,suffix)==raw('vector-0-entry',suffix),'whole original provider/vector/FP lifetime '+suffix)
        need(data(phase)==old_data and data(phase,True)==old_saved,'whole original initialized/unknown/saved snapshot preservation')
        for name,baseline in (('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):
            need((native/('locale-native-'+phase+'-'+name+'.bin')).read_bytes()==(native/baseline).read_bytes(),'native private ordered vector bytes/lifetime')
    def direct_call(pc,target):
        start,size=names['process_compiler_options'];code=bytes(d.elf_byte(native,pc-5+i)for i in range(5));need(start<pc<start+size and code[0]==0xe8 and pc+struct.unpack_from('<i',code,1)[0]==target,'actual application caller/CALL encoding')
    for i,(entry,returned)in enumerate(zip(entries,returns)):
        before_phase='vector-%d-entry'%i;after_phase='vector-%d-return'%i
        need(entry['pc']==names['new_option_vector'][0]and entry['args']==[40,1]and entry['formal_inventory']==[{'name':'capacity','type':'unsigned int'},{'name':'arena_index','type':'int'}]and entry['caller']=='process_compiler_options','actual input PC/formals/40/1/caller')
        need(entry['layout']=={'size':12,'fields':{'count':[0,2,'short'],'capacity':[2,2,'short'],'arena_index':[4,4,'int'],'items':[8,4,'void **']}},'exact encountered12-byte typed header/field widths')
        direct_call(entry['return_pc'],entry['pc']);need(returned['pc']==entry['return_pc']and returned['entry_esp']==entry['esp']and returned['esp']==entry['esp']+4 and all(returned[r]==entry[r]for r in ('ebx','esi','edi','ebp')),'actual RET0/nonvolatile preservation')
        a=registers('vector-%d-entry-registers.bin'%i,14);b=registers('vector-%d-return-registers.bin'%i,14)
        need(a[1]==0x1040682f and list(a[8:11])==[0x10400000+[0x39059,0x3906a,0x3907b][i],40,1]and b[1]==a[8]and b[2]==a[2]+4 and b[11]==a[2]and b[4:8]==a[4:8]and b[3]==old_objects[i],'original actual inputs/RET0/result allocation/nonvolatiles')
        need(entry['guard']==expected_native and raw(before_phase,'target-BSS')==bytes(expected_old),'exact entry predecessor/full neighbor guard')
        need(returned['eax']==objects[i]and returned['header']==[0,40,1,objects[i]+12]and returned['raw_header']==struct.pack('<hhII',0,40,1,objects[i]+12).hex()and bytes.fromhex(returned['items'])==bytes(160),'real native header/returned owned object/pointer storage')
        need((original/('vector-%d-header.bin'%i)).read_bytes()==struct.pack('<hhII',0,40,1,old_objects[i]+12)and(original/('vector-%d-items.bin'%i)).read_bytes()==bytes(160),'real original12B header/160B untouched pointer storage')
        current=[base+172*(i+1),base+32768,base,32768,*first['arena'][4:]]
        need(returned['arena']==current,'native exact cursor/limit/committed bytes/descriptor reserve metadata')
        old_current=list(struct.unpack_from('<8I',expected_old,old_arena_offset));old_current[:4]=[old_base+172*(i+1),old_base+32768,old_base,32768]
        struct.pack_into('<8I',expected_old,old_arena_offset,*old_current)
        bss=bytearray.fromhex(expected_native['bss']);struct.pack_into('<8I',bss,arena_address-prefix['frontend']['bss_address'],*current);expected_native['bss']=bss.hex()
        need(returned['guard']==expected_native and raw(after_phase,'target-BSS')==bytes(expected_old),'only actual arena1 BSS fields changed/full unknown-neighbor guard')
        for j,(address,directory)in enumerate([(base,native),(old_base,original)]):
            expected=bytearray(32768)
            for k in range(i+1):struct.pack_into('<hhII',expected,k*172,0,40,1,address+k*172+12)
            need((directory/('vector-%d-committed.bin'%i)).read_bytes()==bytes(expected),'complete owned committed bytes/three headers/remaining zero pointer arrays and tail')
        for phase,row in ((before_phase,entry),(after_phase,returned)):stable_locale(phase,row)
        mapping(entry,i!=0);mapping(returned,True)
        # This assignment belongs to the accepted caller, after this return.
        struct.pack_into('<I',expected_old,old_pointer_rvas[i]-old_bss_base,old_objects[i]);bss=bytearray.fromhex(expected_native['bss']);struct.pack_into('<I',bss,offsets[i],objects[i]);expected_native['bss']=bss.hex()
    need(member['pc']==names['_ZN13CompilerArena12CommitExtentEv'][0]and member['ecx']==arena_address and member['args']==[]and member['formal_inventory']==[{'name':'this','type':'CompilerArena *'}]and member['caller']=='new_option_vector','real first-header CommitExtent thiscall receiver/formals')
    need(member_return['pc']==member['return_pc']and member_return['entry_esp']==member['esp']and member_return['esp']==member['esp']+4 and all(member_return[r]==member[r]for r in ('ebx','esi','edi','ebp')),'actual CommitExtent RET0/nonvolatiles')
    m=registers('vector-member-entry-registers.bin',12);mr=registers('vector-member-return-registers.bin',12)
    need(m[1]==0x1041d738 and m[8]==0x10400000+old_arena and m[9]==0x10406895 and mr[1]==m[9]and mr[2]==m[2]+4 and mr[4:8]==m[4:8],'original real member ECX/no-stackargs/RET0/nonvolatile')
    need(member['arena'][:4]==[base+12,base,base,0]and member_return['arena'][:4]==[base+12,base+32768,base,32768],'member actual before/after arena arithmetic')
    need(api['pc']==names['recon_VirtualAlloc'][0]and api['args']==[base,32768,4096,4]and api['caller_this']==arena_address and api_return['pc']==api['return_pc']and api_return['eax']==base and api_return['esp']==api['esp']+20 and all(api_return[r]==api[r]for r in ('ebx','esi','edi','ebp')),'actual complete commit API arguments/result/RET16/nonvolatiles')
    need(api['guard']==api_return['guard']==member['guard'],'commit API changes no target/saved/external bytes before member stores')
    need((native/'vector-commit-zero.bin').read_bytes()==(original/'vector-commit-zero.bin').read_bytes()==bytes(32768),'complete actual32KiB zero committed output')
    before=re.findall(r'^vector-commit-before seq=(\d+) esp=(\w+) address=(\w+) bytes=(\w+) kind=(\w+) protection=(\w+) provider=(\w+) error=(\d+)$',text,re.M);after=re.findall(r'^vector-commit-after seq=(\d+) esp=(\w+) before-esp=(\w+) result=(\w+) error=(\d+)$',text,re.M)
    need(len(before)==len(after)==1 and [int(v,16)for v in before[0][2:6]]==[old_base,32768,4096,4]and int(after[0][1],16)==int(before[0][1],16)+16 and after[0][2]==before[0][1]and int(after[0][3],16)==old_base and before[0][-1]==after[0][-1]=='203','actual original commit four args/result/RET16/errors')
    prefix_provider=re.findall(r'^pool-commit-before .*provider=(\w+) ',text,re.M);need(prefix_provider==[before[0][6]],'same authenticated actual kernel provider/IAT as earlier commit')
    for phase,row in [('vector-member-entry',member),('vector-member-return',member_return),('vector-API-entry',api),('vector-API-return',api_return)]:
        need(row['last_error']==203 and locale_value(row)==locale_value(first)and semantic(row['fp'])==initial_fp,'member/API provider-owner/FP/error invariants')
    for row,committed in [(member,False),(member_return,True),(api,False),(api_return,True)]:mapping(row,committed)
    need({'address':base,'bytes':first['arena'][5]}in api_return['owned_reservations'],'commit result stays in exact existing owned reservation')
    regions=re.findall(r'^vector-commit-region part=(\d+) base=(\w+) allocation=(\w+) allocation-protect=(\w+) bytes=(\w+) state=(\w+) protect=(\w+) type=(\w+)$',text,re.M)
    actual_regions=[[int(v,16)for v in row[1:]]for row in regions]
    need([row[0]for row in regions]==['0','1']and actual_regions==[[old_base,old_base,4,32768,0x1000,4,0x20000],[old_base+32768,old_base,4,first['arena'][5]-32768,0x2000,0,0x20000]],'original actual committed+opaque reservedtail VirtualQuery closure')
    observed_traps=re.findall(r'^trap seq=(\d+) rva=(\w+) .*$',text,re.M)
    first_sequence=registers('vector-0-entry-registers.bin',14)[0]
    tail=[(int(seq),rva)for seq,rva in observed_traps if int(seq)>=first_sequence]
    expected_tail=[(first_sequence,'0000682f'),(first_sequence+1,'0001d738'),(first_sequence+2,'0000682f'),(first_sequence+3,'0000682f')]
    if seed:expected_tail.append((first_sequence+4,'00039428'))
    need(tail==expected_tail,'complete actual target function suffix chronology, no hidden/extra target frontier')
    expected_frontend=copy.deepcopy(prefix['frontend']);expected_frontend['arenas'][8:16]=returns[-1]['arena']
    for name,address in zip(pointer_names,objects):expected_frontend['options_bss'][name]=address
    need(state['frontend']==expected_frontend,'final complete modeled state differs beyond arena1+three actual vector pointer stores')
    if not seed:
        public=one('public-entries');need(public['pc']==names['recon_mspdb_SigForPbCb'][0]and public['args']==[public['args'][0],3,0xffffffff]and public['input']=='43505000'and public['formal_inventory']==[{'name':'bytes','type':'const void *'},{'name':'byte_count','type':'long'},{'name':'signature','type':'unsigned long'}]and public['caller']=='process_compiler_options','genuine public adapter entry/read-only CPP/signedlong count/signature formals')
        need(state['symbol']=='recon_mspdb_SigForPbCb'and state['pc']==public['pc']and state['esp']==public['esp']and state['args']==dict(zip(('bytes','byte_count','signature'),public['args']))and state['raw_entry_abi']=={'esp':public['esp'],'return_pc':public['return_pc'],'stack_arguments':public['args']},'real pre-prologue raw public ABI, not DWARF guessed values')
        need(public['guard']==expected_native and raw('signature-entry','target-BSS')==bytes(expected_old),'actual public frontier complete state and last caller assignment')
        stable_locale('signature-entry',public)
        dep=registers('signature-entry-registers.bin',12);provider=original/'loaded-mspdb50.bin'
        need(s.sha(provider)=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'and export_rva(provider,'SigForPbCb')==0xf22d and dep[1]==dep[11]+0xf22d and dep[7:11]==(0x1043909f,0x104d6620,3,0xffffffff)and(original/'signature-input.bin').read_bytes()==b'CPP\0'and(original/'signature-provider-entry.bin').read_bytes()==pe_read(provider,0xf22d,53),'actual loaded provider hFile/name/export/entry/CPP ABI; no body execution')
        need(dep[0]==first_sequence+4,'actual dependency snapshot sequence follows exactly three returns')
        mapping(public,True)
        need(re.findall(r'^dependency-frontier module=MSPDB50.dll rva=0000f22d seq=(\d+)$',text,re.M)==[str(dep[0])]and not re.findall(r'^frontier rva=',text,re.M)and b'C1XX_PUBLIC_FRONTIER MSPDB50!SigForPbCb'in(native/'gdb.txt').read_bytes(),'explicit real dependency-entry frontier on both sides')
        frontier='public:mspdb50!SigForPbCb'
    else:
        following=one('next-entries');need(rows('public-entries')==[],'restore branch did not enter unsupported provider')
        need(prefix['frontend']['option_c9il']!=0 and struct.unpack_from('<I',old_data,0x1488)[0]==prefix['frontend']['option_c9il'],'actual nonzero restored branch discriminator')
        need(following['pc']==names['next_argument'][0]and following['args']==[]and following['formal_inventory']==[]and following['caller']=='process_compiler_options'and state['symbol']=='next_argument'and state['args']=={},'real target next stub/no source formals')
        direct_call(following['return_pc'],following['pc'])
        start,size=names['next_argument'];need(start<state['pc']<=start+size and d.elf_byte(native,state['pc']-1)==0xcc and b'Program received signal SIGTRAP'in(native/'gdb.txt').read_bytes(),'actual source INT3 next frontier')
        need(following['guard']==expected_native and locale_value(following)==locale_value(first)and following['last_error']==203 and semantic(following['fp'])==initial_fp,'actual native next full guard/provider-owner/error/FP continuity')
        mapping(following,True)
        final_seq=first_sequence+4
        need((original/('pool-bss-%03d.bin'%(final_seq+1))).read_bytes()==bytes(expected_old)and(original/('frontend-data-%03d.bin'%final_seq)).read_bytes()==old_data and(original/('frontend-snapshot-%03d.bin'%final_seq)).read_bytes()==old_saved,'direct original next-stub complete target/unknown/saved state')
        need(semantic(fp((original/('x87-context-%03d.bin'%final_seq)).read_bytes()))==initial_fp,'direct original next-stub architectural FP')
        last_trap=re.findall(r'^trap seq=%d rva=00039428 .*stack=([0-9a-f,]+)\nlast-error=(\d+)'%final_seq,text,re.M)
        need(len(last_trap)==1 and [int(v,16)for v in last_trap[0][0].split(',')][:3]==[0x104390b3,0x10439428,0]and last_trap[0][1]=='203'and re.findall(r'^frontier rva=(\w+)$',text,re.M)==['00039428'],'original actual next noarg caller; outer pending matcher arguments are not callee formals')
        # Derived continuity, not an additional original provider-table read:
        # the actual nonzero flag takes JNE390a7. The executed path writes only
        # forced_includes, adjusts/pushes the caller stack, then calls the stub.
        import capstone
        md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
        blocks=[list(md.disasm(pe_read(source,0x3907b,0x11),0x3907b)),list(md.disasm(pe_read(source,0x390a7,12),0x390a7))]
        need([(i.mnemonic,i.op_str)for i in blocks[0]]==[('mov','dword ptr [0x104c0584], eax'),('mov','eax, dword ptr [0x104cd488]'),('add','esp, 8'),('test','eax, eax'),('jne','0x390a7')]and[(i.mnemonic,i.op_str)for i in blocks[1]]==[('push','0'),('push','0x10439428'),('call','0x39428')],'complete decoded executed tail: no intermediate import/call/unknown memory store')
        need('dependency-frontier 'not in text,'restore has target frontier only')
        for name,baseline in (('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):
            need((native/('locale-native-vector-next-'+name+'.bin')).read_bytes()==(native/baseline).read_bytes(),'actual native next private vector lifetime')
        frontier='_next_argument'

    return dict(frontier=frontier,code_bytes=116,calls=3,object_bytes=36,pointer_storage_bytes=480,commit_bytes=32768,provider_executed=False,provider_return_claimed=False)
