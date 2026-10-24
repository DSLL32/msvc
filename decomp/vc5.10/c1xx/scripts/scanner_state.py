"""Encountered512-byte initialized projection and actual72-byte helper/API."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import struct

from scanner_projection import defaults
from scaler_probe import symbols
from scaler_state import fp,semantic


def check(s,d,original,native,trial,text,pre,phase,state,seed,final,traps):
    def require(ok,message):s.require(ok,'multibyte scanner: '+message)
    def read(name):return json.loads((native/('scanner-'+name+'.json')).read_text())
    def one(name):
        rows=read(name);require(len(rows)==1,'actual '+name+' inventory');return rows[0]
    spec=importlib.util.spec_from_file_location('scanner_original_multibyte',Path(__file__).with_name('check-isolated-multibyte.py'));reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)
    original_state=reference.check(original)
    require(trial['trial'] in ('_initialize_multibyte_categories','_initialize_lowercase_table','_read_env_arguments','_new_option_vector','_next_argument','_match_option','_apply_option_action','_parse_option_integer','_match_option_suffix') and (state['symbol'],state['args'])=={'_initialize_multibyte_categories':('initialize_lowercase_table',{}),'_initialize_lowercase_table':('read_env_arguments',{'arguments':0}),'_match_option_suffix':('new_option_vector',{'capacity':40,'arena_index':1}),'_parse_option_integer':('new_option_vector',{'capacity':40,'arena_index':1}),'_apply_option_action':('new_option_vector',{'capacity':40,'arena_index':1}),'_match_option':('new_option_vector',{'capacity':40,'arena_index':1}),'_next_argument':('new_option_vector',{'capacity':40,'arena_index':1}),'_new_option_vector':('new_option_vector',{'capacity':40,'arena_index':1}),'_read_env_arguments':('new_option_vector',{'capacity':40,'arena_index':1})}[trial['trial']],'candidate/next genuine noarg frontier')
    names=symbols(native/'native-executable.elf')
    entry,returned,before,after,next_entry=(one(name)for name in ('entries','returns','api-entries','api-returns','next-entries'))
    expected_defaults=b''.join(bytes(defaults()[name])for name in ('classes','flags'))
    live_seed=bytes((69*7+i+31)&255 for i in range(512));saved_seed=bytes((69*7+i+11)&255 for i in range(512))
    def table(snapshot,saved=False):
        row=snapshot['frontend']['saved_scanner'if saved else'scanner']
        return None if row is None else b''.join(bytes(row[name])for name in ('classes','flags'))
    require(table(pre)==(live_seed if seed else expected_defaults) and table(pre,True)==(saved_seed if seed else None),'actual typed nonzero/default pre-initializer input')
    require(table(phase)==table(phase,True)==(saved_seed if seed else expected_defaults),'new512 initialized snapshot/restore or inappropriate BSS clearing')
    require((original/'frontend-data-000.bin').read_bytes()[8:520]==expected_defaults,'own actual earliest default bytes')
    require((original/'frontend-data-006.bin').read_bytes()[8:520]==(live_seed if seed else expected_defaults) and (original/'frontend-data-phase.bin').read_bytes()[8:520]==(saved_seed if seed else expected_defaults),'complete actual original initial/restore arrays')
    if seed:
        require('scanner-seed point=initializer live-start=31 saved-start=11 bytes=512 no-code-mock=1\n'in text,'undeclared initialized-data seed role')
        require((original/'frontend-snapshot-006.bin').read_bytes()[8:520]==saved_seed,'actual nonzero saved array content')
    else:require('scanner-seed 'not in text,'natural trial gained array mock')
    require(entry['pc']==names['initialize_multibyte_categories'][0] and entry['formal_inventory']==[] and next_entry['pc']==names['initialize_lowercase_table'][0] and next_entry['formal_inventory']==[],'actual application entry/next noarg ABI')
    require(entry['scanner_address']==state['frontend']['storage']['scanner.classes'][0] and entry['scanner_address']+256==state['frontend']['storage']['scanner.flags'][0] and bytes.fromhex(entry['scanner'])==(saved_seed if seed else expected_defaults),'actual typed receiver/storage/input array identities')
    require(before['pc']==names['recon_setmbcp'][0] and before['args']==[-4] and before['formal_inventory']==[dict(name='codepage',type='int')],'actual public source formal/argument inventory')
    require(before['guard']==after['guard']==entry['guard'] and before['scanner']==after['scanner']==entry['scanner'],'public API changed target projection/neighbor/snapshot bytes')
    require(after['pc']==before['return_pc'] and after['entry_esp']==before['esp'] and after['esp']==before['esp']+4 and after['eax']==0 and all(after[r]==before[r]for r in ('ebx','esi','edi','ebp')),'actual public cdecl4/RET0/result/nonvolatile ABI')
    mb=bytes.fromhex(after['locale']['mbctype']);require(len(mb)==257,'complete provider MBtable')
    expected=bytearray.fromhex(entry['scanner'])
    for character in range(256):
        if mb[character+1]&4:expected[256+character]|=128;expected[character]=35
    guard=copy.deepcopy(entry['guard']);data=bytearray.fromhex(guard['data']);data[2028:2540]=expected;guard['data']=data.hex()
    require(returned['guard']==next_entry['guard']==guard and bytes.fromhex(returned['scanner'])==bytes.fromhex(next_entry['scanner'])==table(state)==bytes(expected),'all512 helper outputs/full native neighbors and lifetime')
    require(returned['pc']==entry['return_pc'] and returned['entry_esp']==entry['esp'] and returned['esp']==entry['esp']+4 and all(returned[r]==entry[r]for r in ('ebx','esi','edi','ebp')),'actual void helper RET0/nonvolatile; no incidental EAX claim')
    expected_fp=semantic(fp((original/'locale-mb-before-fp.bin').read_bytes()))
    require(all(semantic(row['fp'])==expected_fp and row['last_error']==203 for row in (entry,returned,before,after,next_entry)),'actual complete architectural FP/Last203')
    start,length=names['process_compiler_options']
    for row in (entry,next_entry):
        ret=row['return_pc'];code=bytes(d.elf_byte(native,ret-5+i)for i in range(5));require(start<ret<start+length and code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==row['pc'],'actual native CALL/owner')
    start,length=names['initialize_multibyte_categories'];ret=before['return_pc'];code=bytes(d.elf_byte(native,ret-5+i)for i in range(5));require(start<ret<start+length and code[0]==0xe8 and ret+struct.unpack_from('<i',code,1)[0]==before['pc'],'actual public CALL from ownhelper')
    old_rows=[row for row in traps if row[1]=='00039394'];require(len(old_rows)==1,'original genuine helper-entry count');seq=int(old_rows[0][0]);old_regs=re.findall(r'^pool-registers seq=%d ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$'%(seq+1),text,re.M)
    old_ret=re.findall(r'^mb-helper-return pc=(\w+) esp=(\w+) eax=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+) error=(\d+)$',text,re.M);require(len(old_regs)==len(old_ret)==1 and tuple(old_regs[0])==tuple(old_ret[0][3:7]),'original actual helper nonvolatiles')
    receipt=original_state['original_receipt_sha256']
    for label,row in (('mb-before',before),('mb-after',after),('mb-helper-return',returned),('mb-next',next_entry)):
        current=row['locale'];raw=original_state['phases'][label]['words'];selection=current['selection']
        require([current['mb_cur_max'],current['lc_codepage'],*current['lc_handle'],current['mb_gate'],current['mb_codepage']]==raw[1:11],'independent actual MBgate/codepage/MBmax/LC handles')
        require(current['errors']==[0,0,203] and selection==dict(bound=1,calls=1,reference=receipt,result=entry['locale']['selection']['result'],mb_calls=0 if label=='mb-before' else 1),'once-only ownreceipt/LC+MBcall/result/error inventory')
        for key in ('mbctype','ctype','old-ctype','result'):
            native_key='result_bytes'if key=='result'else key.replace('-','_')
            require(bytes.fromhex(current[native_key])==(original/('locale-'+label+'-'+key+'.bin')).read_bytes(),'complete current/oldtable/result and lifetime: '+key)
        require(current['pctype']==entry['locale']['pctype'] and current['old_pctype']==entry['locale']['old_pctype'] and current['result_pointer']==entry['locale']['result_pointer'] and current['result_pointer']==selection['result'],'existing locale table/result allocation identities preserved')
        for name,baseline in (('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):
            require((native/('locale-native-'+label+'-'+name+'.bin')).read_bytes()==(native/baseline).read_bytes(),'actual private vector storage/content/lifetime')
    require(table(state,True)==table(phase,True),'saved512byte array lifetime')
    return dict(code_bytes=72,initialized_semantic_bytes=512,full_unsigned_byte_domain=256,MB_last_error=203,provider_reference=receipt,unknown_initialized_bytes=6031,nonzero_initialized_restore=bool(seed),new_BSS_bytes=0,helper_return_value_claimed=False)
