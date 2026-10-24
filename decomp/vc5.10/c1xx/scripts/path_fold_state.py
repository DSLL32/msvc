"""Paired typed filename map, complete actual public vector, ABI and lifetime."""
import copy,importlib.util,json,struct
from pathlib import Path
from scaler_probe import symbols
from scaler_state import fp,semantic
from path_fold_original import record


def check(s,d,original,native,trial,text,pre,phase,state,seed,final,traps):
    def need(condition,message):s.require(condition,'filename fold: '+message)
    from path_fold_original import check as check_original
    reference=check_original(original)
    def rows(name):return json.loads((native/('path-fold-'+name+'.json')).read_text())
    def one(name):
        result=rows(name);need(len(result)==1,'actual '+name+' inventory');return result[0]
    entry,returned,next_entry=(one(x)for x in ('entries','returns','next-entries'));calls,returns=rows('api-entries'),rows('api-returns');names=symbols(native/'native-executable.elf');count=reference['calls']
    need(len(calls)==len(returns)==count,'complete actual public call inventory')
    need(entry['pc']==names['initialize_lowercase_table'][0] and entry['formal_inventory']==[] and next_entry['pc']==names['read_env_arguments'][0] and next_entry['formal_inventory']==[{'name':'arguments','type':'char *'}] and next_entry['args']==[0] and ((state['symbol'],state['args'])==('read_env_arguments',{'arguments':0})if trial['trial']=='_initialize_lowercase_table'else(state['symbol'],state['args'])==('new_option_vector',{'capacity':40,'arena_index':1})),'actual helper/next PC/formals/NULL input')
    need(entry['map_address']==returned['map_address']==next_entry['map_address']==state['frontend']['storage']['filename_case_map'][0]==state['frontend']['bss_address']+2312,'actual512 WORD array storage/alias')
    preceding=json.loads((native/'scanner-next-entries.json').read_text())
    need(len(preceding)==1 and preceding[0]['pc']==entry['pc'],'same actual scanner-next/fold-entry boundary')
    # The only intervening auxiliary action writes this target's WORD map.
    # It cannot replace any previously authenticated CRT allocation/vector.
    owner_keys=('pctype','old_pctype','static_ctype','result_pointer','selection','crt_argc','CRT-argv','CRT-environment')
    need(all(entry['locale'][key]==preceding[0]['locale'][key]for key in owner_keys),'crossphase original CRT table/result/selection/vector owner identity')
    initial=struct.pack('<256H',*[(0x8101+37*i)&65535 for i in range(256)])if seed else bytes(512)
    need(struct.pack('<256H',*pre['frontend']['filename_case_map'])==initial and phase['frontend']['filename_case_map']==[0]*256,'all512 new fields/default/nonzero reset')
    need(bytes.fromhex(entry['before_seed']['map'])==bytes(512),'real helper input after BSS reset')
    working=[(0x4303+37*i)&65535 for i in range(256)]if seed else[0]*256
    need(bytes.fromhex(entry['map'])==struct.pack('<256H',*working),'actual labeled helper overwrite input')
    before=copy.deepcopy(entry['before_seed']['guard']);bss=bytearray.fromhex(before['bss']);bss[2312:2824]=struct.pack('<256H',*working);before['bss']=bss.hex();need(before==entry['guard'],'helper seed changed only exact512 known bytes')
    classes=bytes.fromhex(entry['classes']);arguments=[i for i in range(256)if classes[i]!=35];need(arguments==[r[0]for r in reference['results']],'actual source byte domain/lead skips')
    expected_fp=semantic(fp((original/'locale-mb-next-fp.bin').read_bytes()));locale=entry['locale'];guard=copy.deepcopy(entry['guard']);previous=-1
    need(s.sha(native/'native-executable.elf')==trial['bound_inputs']['build/native/c1xx-host'],'immutable captured ELF identity')
    call_return_pc=calls[0]['return_pc'];start,size=names['initialize_lowercase_table']
    need(start<call_return_pc<start+size,'actual public caller owner')
    code=bytes(d.elf_byte(native,call_return_pc-5+i)for i in range(5))
    need(code[0]==0xe8 and call_return_pc+struct.unpack_from('<i',code,1)[0]==names['recon_mbctolower'][0],'single authenticated public CALL encoding/target')
    for index,((character,result),a,b)in enumerate(zip(reference['results'],calls,returns)):
        for skipped in range(previous+1,character):need(classes[skipped]==35,'actual lead branch');working[skipped]=65535
        partial=struct.pack('<256H',*working);data=bytearray.fromhex(guard['bss']);data[2312:2824]=partial;guard['bss']=data.hex()
        need(a['pc']==names['recon_mbctolower'][0]and a['args']==[character]and a['formal_inventory']==[{'name':'character','type':'unsigned int'}],'actual public PC/unsigned formal/input')
        need(b['pc']==a['return_pc'] and b['esp']==a['esp']+4 and b['entry_esp']==a['esp'] and b['eax']==result and all(a[r]==b[r]for r in ('ebx','esi','edi','ebp')),'actual public cdecl4/RET0/full unsigned result/nonvolatile')
        need(a['guard']==b['guard']==guard and bytes.fromhex(a['map'])==bytes.fromhex(b['map'])==partial and a['classes']==b['classes']==entry['classes'],'all partial512 outputs/only owned writes/API no target mutation')
        for row in (a,b):
            need(row['last_error']==203 and semantic(row['fp'])==expected_fp and row['locale']==locale,'full API locale/oldtable/result/selection/error/architecturalFP state')
        for suffix in ('before','after'):
            for kind,baseline in (('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):
                need((native/('locale-native-fold-api-%03d-%s-%s.bin'%(index,suffix,kind))).read_bytes()==(native/baseline).read_bytes(),'private vectors/content/identity/lifetime')
        need(a['return_pc']==call_return_pc,'actual public callsite differs from the single authenticated site')
        working[character]=result;previous=character
    for skipped in range(previous+1,256):need(classes[skipped]==35,'actual final lead branch');working[skipped]=65535
    working[47]=92;raw=struct.pack('<256H',*working);bss=bytearray.fromhex(guard['bss']);bss[2312:2824]=raw;guard['bss']=bss.hex()
    need(returned['guard']==next_entry['guard']==guard and bytes.fromhex(returned['map'])==bytes.fromhex(next_entry['map'])==raw and state['frontend']['filename_case_map']==working,'complete512B output/forcedslash/fullneighbor/lifetime')
    need(returned['pc']==entry['return_pc'] and returned['esp']==entry['esp']+4 and returned['entry_esp']==entry['esp'] and all(returned[r]==entry[r]for r in ('ebx','esi','edi','ebp')),'actual helper RET0/nonvolatile; no incidental EAX claim')
    for row in (entry,returned,next_entry):need(row['locale']==locale and row['last_error']==203 and semantic(row['fp'])==expected_fp,'actual helper/next full provider state/FP/error preservation')
    for kind,suffix in (('mbctype','mbctype'),('ctype','ctype'),('old_ctype','old-ctype'),('result_bytes','result')):
        need(bytes.fromhex(locale[kind])==(original/('locale-mb-next-'+suffix+'.bin')).read_bytes(),'complete original/native provider current/retained bytes '+kind)
    need(locale['errors']==[0,0,203]and [locale['mb_cur_max'],locale['lc_codepage'],*locale['lc_handle'],locale['mb_gate'],locale['mb_codepage']]==[1,1252,0,0,1033,0,0,0,0,1252]and locale['selection']['calls']==locale['selection']['mb_calls']==1 and locale['selection']['reference']==reference['original_receipt_sha256'],'own boundreceipt/once-only fullpostselection prestate')
    start,size=names['process_compiler_options']
    for row in (entry,next_entry):
        need(start<row['return_pc']<start+size,'actual helper/next caller owner');code=bytes(d.elf_byte(native,row['return_pc']-5+i)for i in range(5));need(code[0]==0xe8 and row['return_pc']+struct.unpack_from('<i',code,1)[0]==row['pc'],'actual helper/next CALL target')
    environment=json.loads((native/'fold-getenv-entries.json').read_text());environment_return=json.loads((native/'fold-getenv-returns.json').read_text())
    need(len(environment)==len(environment_return)==1,'actual second getenv inventory')
    a,b=environment[0],environment_return[0]
    need(a['name']=='MSC_CMD_FLAGS' and a['caller']=='process_compiler_options' and a['bound']==b['bound']==1 and a['environment']['address']==b['environment'] and a['argc']==locale['crt_argc'] and b['eax']==0 and b['pc']==a['return_pc'] and b['esp']==a['esp']+4 and all(a[r]==b[r]for r in ('ebx','esi','edi','ebp'))and a['last_error']==b['last_error']==203 and a['fpu']==b['fpu']==0x27f,'native second target-bound getenv input/NULL/RET0/state')
    # Original evidence is its authenticated contiguous static call chain and
    # actual next-consumer NULL, not a claimed direct original API-entry trap.
    from allocation_imports import pe_read
    need(pe_read(original/'loaded-target.bin',0x3900d,0x14)==bytes.fromhex('6880664d10ff1548904c1083c40450e8f2030000')and pe_read(original/'loaded-target.bin',0xd6680,14)==b'MSC_CMD_FLAGS\0','original contiguous getenv/push/call-to-next/name chain')
    return dict(code_bytes=83,new_BSS_semantic_bytes=512,original_public_calls=count,full_unsigned_byte_domain=256,lead_skips=256-count,slash_override=[47,92],nonzero_reset_and_overwrite=bool(seed),helper_EAX_semantics_claimed=False,original_unknown_initialized_bytes=6031)
