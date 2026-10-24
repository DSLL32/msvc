"""Scalar linkage stack push, typed restore/reset and actual cdecl entry/return."""
import copy,json,re,struct
from allocation_imports import pe_read
from scaler_probe import symbols
from scaler_state import fp,semantic


def check(s,d,original,native,trial,text,pre,phase,state,seed,final,traps):
    def require(value,message):s.require(value,'language context: '+message)
    def read(name):return json.loads((native/(name+'.json')).read_text())
    def raw(suffix):
        blob=(original/('language-context-'+suffix+'.bin')).read_bytes();require(len(blob)==44,'typed depth/slot observation absent');return struct.unpack('<11i',blob)
    def values(snapshot):return(snapshot['frontend']['language_context_depth'],*snapshot['frontend']['language_contexts'])
    require(trial['trial']in ('_push_declaration_context','_initialize_include_aliases','_initialize_checksums','_initialize_multibyte_categories','_initialize_lowercase_table','_read_env_arguments','_new_option_vector','_next_argument','_match_option','_apply_option_action','_parse_option_integer','_match_option_suffix') and trial['language_context_observers']=={'entry_rva':0x2b834,'caller_return_rva':0x38fbe,'argument':0,'depth_rva':0xcd580,'slots_rva':0xc13d0,'slots':10,'scope':'scalar language context; actual first NULL-independent zero push, no mocked call'},'wrong scalar observer contract')
    require(pe_read(original/'loaded-target.bin',0xcd580,4)==struct.pack('<i',-1) and raw('000')==(-1,*([0]*10)),'own scalar/BSS defaults differ')
    require(raw('006')==values(pre)==((7,*[1+(i&1)for i in range(10)])if seed else(-1,*([0]*10))),'nonzero typed initializer input/default differs')
    require(raw('phase')==values(phase)==((3,*([0]*10))if seed else(-1,*([0]*10))),'new initialized depth restore or all40 BSS clear differs')
    require(pre['frontend']['saved_language_context_depth']==3 if seed else pre['frontend']['saved_language_context_depth']is None,'saved typed nonzero/default depth differs')
    require(phase['frontend']['saved_language_context_depth']==state['frontend']['saved_language_context_depth']==(3 if seed else-1),'saved depth changed')
    if seed:
        expected={'initializer_live_depth':7,'initializer_saved_depth':3,'initializer_slots':[1+(i&1)for i in range(10)],'helper_entry_slots':[1+(i&1)for i in range(10)],'helper_argument_unchanged':0,'code_or_helper_mock':False}
        require(trial['language_context_auxiliary']==expected and 'language-seed point=initializer live-depth=7 saved-depth=3 slots=alternating1,2 no-code-mock=1\n'in text and 'language-seed point=actual-helper-entry slots=alternating1,2 context-argument-unchanged=1 no-code-mock=1\n'in text,'undeclared data auxiliary roles')
    else:require('language_context_auxiliary'not in trial,'natural case gained data mocks')
    entries=read('options-helper-entries');returns=read('options-helper-returns');nexts=read('options-next-entries');require(len(entries)==len(returns)==1 and len(nexts)==(2 if '_initialize_include_aliases'in trial['accepted']and not seed else 1),'entry/return/next inventory differs');entry,returned,next_entry=entries[0],returns[0],nexts[0]
    names=symbols(native/'native-executable.elf');rows=[v for v in traps if v[1]=='0002b834'];require(len(rows)==1,'original scalar helper entry count differs');seq=int(rows[0][0]);words=[int(v,16)for v in rows[0][2].split(',')]
    require(words[:2]==[0x10438fbe,0] and entry['pc']==names['push_declaration_context'][0] and entry['args']==[0] and entry['formal_inventory']==[dict(name='context',type='int')],'actual scalar inputs/formals/caller differ')
    language_next_seq=next(int(v[0])for v in traps if v[1]==('000392a8'if seed else'00039282'));language_next_suffix='%03d'%language_next_seq
    before=raw('%03d'%seq);expected_before=(3,*[1+(i&1)for i in range(10)])if seed else(-1,*([0]*10));require(before==expected_before,'actual scalar depth/slots before push differ')
    data=bytearray.fromhex(entry['guard']['data']);bss=bytearray.fromhex(entry['guard']['bss']);require(struct.unpack_from('<i',data,2024)[0]==before[0] and struct.unpack_from('<10i',bss,1504)==before[1:],'native actual entry values do not match original typed field map')
    after=list(before);after[0]+=1;require(0<=after[0]<10,'case exceeds modeled stack bounds');after[1+after[0]]=0
    require(raw('language-return')==raw(final)==tuple(after)==values(state),'increment/exact indexed scalar store/continuation differs')
    struct.pack_into('<i',data,2024,after[0]);struct.pack_into('<i',bss,1504+after[0]*4,0);expected_guard=copy.deepcopy(entry['guard']);expected_guard.update(data=data.hex(),bss=bss.hex())
    require(returned['guard']==next_entry['guard']==expected_guard,'helper/next changed neighboring native modeled/padding/saved/external state')
    original_data=bytearray((original/('frontend-data-%03d.bin'%seq)).read_bytes());struct.pack_into('<i',original_data,0x1580,after[0]);require((original/'frontend-data-language-return.bin').read_bytes()==(original/('frontend-data-'+language_next_suffix+'.bin')).read_bytes()==bytes(original_data),'original initialized neighbor or unknown bytes changed')
    original_bss=bytearray((original/('pool-bss-%03d.bin'%(seq+1))).read_bytes());struct.pack_into('<i',original_bss,0xc13d0-0xb9000+after[0]*4,0)
    # A declared checksum data auxiliary seeds only after this real handoff.
    # Compare the observed pre-seed checkpoint, never erase the actual seed.
    next_bss=original/('checksum-entry-before-seed-BSS.bin' if seed and trial.get('checksum_observers') else 'pool-bss-%03d.bin'%(language_next_seq+1))
    require((original/'language-return-bss.bin').read_bytes()==next_bss.read_bytes()==bytes(original_bss),'original whole BSS changed beyond one actual scalar slot')
    saved=(original/('frontend-snapshot-%03d.bin'%seq)).read_bytes();require(saved==(original/'frontend-snapshot-language-return.bin').read_bytes()==(original/('frontend-snapshot-'+final+'.bin')).read_bytes(),'original complete saved internal snapshot changed')
    retrows=re.findall(r'^language-return pc=(\w+) esp=(\w+) eax=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+) error=(\d+)$',text,re.M);require(len(retrows)==1,'actual original return observation absent');old=retrows[0]
    entryesp=int(re.search(r'^trap seq=%d .*esp=(\w+)'%seq,text,re.M)[1],16);require(int(old[0],16)==0x10438fbe and int(old[1],16)==entryesp+4 and int(old[7])==203,'original actual return/RET0/error differs')
    regs=re.findall(r'^pool-registers seq=%d ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$'%(seq+1),text,re.M);require(len(regs)==1 and tuple(old[3:7])==tuple(regs[0]),'original nonvolatile registers changed')
    require(returned['pc']==entry['return_pc'] and returned['entry_esp']==entry['esp'] and returned['esp']==entry['esp']+4 and all(returned[r]==entry[r]for r in ('ebx','esi','edi','ebp')) and entry['last_error']==returned['last_error']==next_entry['last_error']==203,'native actual return/RET0/nonvolatile/error differs')
    require(semantic(fp((original/('x87-context-%03d.bin'%seq)).read_bytes()))==semantic(fp((original/'language-return-fp.bin').read_bytes()))==semantic(entry['fp'])==semantic(returned['fp'])==semantic(next_entry['fp']),'helper changed architectural FP state')
    next_name,oldret,oldrva=('initialize_checksums',0x38fd4,0x392a8)if seed else('initialize_include_aliases',0x38fcf,0x39282)
    require((trial['trial']!='_push_declaration_context'or state['symbol']==next_name) and (state['args']==({'capacity':40,'arena_index':1}if trial['trial']in ('_read_env_arguments','_new_option_vector','_next_argument','_match_option','_apply_option_action','_parse_option_integer','_match_option_suffix')else{'arguments':0}if trial['trial']=='_initialize_lowercase_table'else{})) and next_entry['pc']==names[next_name][0] and next_entry['formal_inventory']==[] and int(next(v[2]for v in traps if int(v[0])==language_next_seq).split(',')[0],16)==0x10400000+oldret and int(next(v[1]for v in traps if int(v[0])==language_next_seq),16)==oldrva,'actual next named noarg ABI/branch/caller differs')
    start,length=names['process_compiler_options'];ret=next_entry['return_pc'];code=bytes(d.elf_byte(native,ret-5+i)for i in range(5));require(start<ret<start+length and code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==next_entry['pc'],'next native call/owner differs')
    # EAX is caller-saved and its incidental helper value is not consumed.
    return dict(code_bytes=53,depth_default=-1,slots=10,scalar_bytes=4,new_initialized_semantic_bytes=4,new_BSS_semantic_bytes=40,nonzero_restore_and_slot_store=bool(seed),next=next_name,return_value_claimed=False,unknown_initialized_bytes=6543)
