"""Void hook, actual CRT namespace and one fully bound GetSystemInfo response."""
import copy
import json
from pathlib import Path
import re
import struct


def check(s,d,original,native,trial,text,state,final,symbols,traps):
    def require(value,message):s.require(value,'system info: '+message)
    def read(name):return json.loads((native/name).read_text())
    def symbol(name):return tuple(int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [TtWw] '+name+'$',symbols,re.M).groups())
    def call(return_pc,destination):
        code=bytes(d.elf_byte(native,return_pc-5+i) for i in range(5));require(code[0]==0xe8 and return_pc+struct.unpack_from('<i',code,1)[0]==destination,'not actual native CALL')
    def owned_vectors(paths):
        ranges=[]
        for path in paths:
            raw=path.read_bytes();address,count=struct.unpack_from('<II',raw);cursor=8;ranges.append((address,address+4*(count+1)))
            for i in range(count):
                pointer,length=struct.unpack_from('<II',raw,cursor);cursor+=8+length;ranges.append((pointer,pointer+length))
        ranges.sort();require(all(a<b for a,b in ranges) and all(left[1]<=right[0] for left,right in zip(ranges,ranges[1:])),'observed vector/string storage aliases; requires explicit identity model')
    require(trial['system_info_observers']=={'owner':'_main','before_rva':0x38c42,'after_rva':0x38c48,'provider_iat_rva':0xc91f4,'destination_rva':0xcd7b8,'output_bytes':36,'scope':'one actual GetSystemInfo; no GlobalMemoryStatus or fabricated fields'},'undeclared API observations')
    hook=read('hook-entry.json');hook_pc,_=symbol('initialize_frontend_hook');main_pc,main_size=symbol('c1xx_main');require(hook['symbol']=='initialize_frontend_hook' and hook['pc']==hook_pc and hook['args']=={} and hook['raw_entry_abi']['stack_arguments']==[],'actual void hook entry differs');ret=hook['raw_entry_abi']['return_pc'];require(main_pc<ret<main_pc+main_size,'hook caller outside main');call(ret,hook_pc)
    old_hook=next(row for row in traps if row[1]=='00038df0');hook_suffix='%03d'%int(old_hook[0]);require(int(old_hook[2].split(',')[0],16)==0x10438c3d,'original void hook return differs')
    original_argv,host_argv=d.b.vector(original/'original-crt-argv.bin');original_env,host_environment=d.b.vector(original/'original-crt-environment.bin');native_argv,observed_argv=d.b.vector(native/'native-crt-argv.bin');native_env,observed_environment=d.b.vector(native/'native-crt-environment.bin')
    require(host_argv==observed_argv and host_environment==observed_environment and len(host_argv)==3 and original_argv!=d.b.vector(original/'invoke-entry-argv.bin')[0] and native_argv!=state['invocation']['argv_address'] and len(state['invocation']['argv'])==26,'original HOST CRT inputs conflated with Invoke inputs or wrong vector')
    owned_vectors([original/name for name in ('original-crt-argv.bin','original-crt-environment.bin','invoke-entry-argv.bin')])
    owned_vectors([native/name for name in ('native-crt-argv.bin','native-crt-environment.bin','native-argv.bin')])
    require(not any(value.partition('=')[0].upper()=='MSC_CMD_FLAGS' for value in host_environment),'actual CRT environment unexpectedly supplies MSC_CMD_FLAGS')
    ge=read('target-getenv-entries.json');gr=read('target-getenv-returns.json');require(len(ge)==len(gr)==1,'wrong target getenv inventory');ge,gr=ge[0],gr[0];getter,_=symbol('recon_getenv');scan,scan_size=symbol('initialize_intermediate_files')
    require(ge['pc']==getter and ge['name']=='MSC_CMD_FLAGS' and ge['bound']==gr['bound']==1 and ge['argc']==3 and ge['argv']=={'address':native_argv,'count':3} and ge['environment']=={'address':native_env,'count':len(host_environment)} and gr['environment']==native_env and gr['eax']==0 and gr['esp']==ge['esp']+4 and gr['pc']==ge['return_pc'] and scan<gr['pc']<scan+scan_size,'real target getenv/bound CRT namespace differs');call(gr['pc'],getter)
    binding=read('system-info-binding.json');reference=s.sha(original/'receipt.json');raw=(original/'system-info-after.bin').read_bytes();require(binding['original_receipt_sha256']==reference and binding['trial_sha256']==s.sha(original/'trial.json') and binding['provider_hfile_sha256']==s.sha(original/'loaded-kernel32.bin') and binding['raw_output_sha256']==s.sha(original/'system-info-after.bin') and binding['GlobalMemoryStatus_supplied'] is False,'wrong original response closure')
    from allocation_imports import export_rva
    old_before=re.findall(r'^system-info-before pc=(\w+) esp=(\w+) argument=(\w+) provider=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) error=(\d+) fpu=(\w+)$',text,re.M);old_after=re.findall(r'^system-info-after pc=(\w+) esp=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) error=(\d+) fpu=(\w+)$',text,re.M)
    require(len(old_before)==len(old_after)==1,'original API inventory differs');ob,oa=old_before[0],old_after[0];kernel=int(re.search(r'^module name=KERNEL32.dll actual=([0-9a-f]+) ',text,re.M)[1],16)
    require([int(ob[i],16) for i in (0,2,3)]==[0x10438c42,0x104cd7b8,kernel+export_rva(original/'loaded-kernel32.bin','GetSystemInfo')] and int(oa[0],16)==0x10438c48 and int(oa[1],16)==int(ob[1],16)+4 and oa[2:5]==ob[4:7] and int(ob[7])==int(oa[5])==203 and int(ob[8],16)&0xffff==int(oa[6],16)&0xffff==0x27f,'actual original GetSystemInfo argument/provider/RET4/register/context differs')
    refresh=binding.get('native_only_trial_refresh')
    if refresh and refresh['kind'].startswith('checked filename-map reset-only'):
        from path_fold_reset_refinement import check as check_reset
        require(s.sha(native/'path-fold-reset-refinement-source.py')==refresh['validator_sha256']and s.sha(native/'path-fold-reset-refinement.json')==refresh['rule_sha256'],'frozen exact reset derivation source/rule identity')
        current=check_reset(original,native);current['validator_sha256']=refresh['validator_sha256']
        require(current==refresh,'current static reset-only derivation does not match native capture binding')
    require(binding['target_sha256']==trial['target']['sha256'] and binding['canonical_crt_sha256']==s.sha(original/'loaded-msvcrt.bin') and binding['profile_bytes']==104 and (native/'system-info-profile.bin').read_bytes()==b'RSI1'+reference.encode('ascii')+raw and binding['profile_sha256']==s.sha(native/'system-info-profile.bin'),'wrong shared capability/profile bytes')
    require(binding['original_crt_inputs']=={name:s.sha(original/name) for name in ('original-crt-argv.bin','original-crt-environment.bin')} and all((native/name).read_bytes()==(original/name).read_bytes() for name in binding['original_crt_inputs']),'wrong captured CRT input closure')
    require(set(binding['controllers'])=={'lib/recon/system_profile.py','decomp/vc5.10/c1xx/scripts/system_info_reference.py'} and all(s.sha(native/row['artifact'])==row['sha256'] for row in binding['controllers'].values()),'wrong profile encoder/factory content snapshots')
    before=read('system-info-entries.json');after=read('system-info-returns.json');require(len(before)==len(after)==1,'wrong GetSystemInfo inventory');before,after=before[0],after[0];api,_=symbol('recon_GetSystemInfo')
    expected_profile={'bound':2,'system_calls':0,'memory_calls':0,'reference':reference};require(before['profile']==expected_profile,'wrong/unbound pre-call capability');expected_profile['system_calls']=1;require(after['profile']==expected_profile,'profile not consumed once or memory capability supplied')
    require(before['pc']==api and before['argument']==hook['frontend']['storage']['system_info'][0] and before['formal_inventory']==[{'name':'output','type':'LPSYSTEM_INFO'}] and main_pc<before['return_pc']<main_pc+main_size and after['pc']==before['return_pc'] and after['esp']==before['esp']+8 and all(after[r]==before[r] for r in ('edi','esi','ebx')),'actual GetSystemInfo destination/caller/RET4/register ABI differs');call(before['return_pc'],api)
    require(before['last_error']==after['last_error']==203 and before['fpu']==after['fpu']==0x27f,'API error/FPU differs')
    words=list(struct.unpack('<8I2H',raw));expected=copy.deepcopy(hook['frontend']);expected['system_info']=words
    api_entry=read('system-info-entry.json');api_return=read('system-info-return.json');boundary=read('arena-entry.json') if '_initialize_compiler_environment' in trial['accepted'] else state;require(api_entry['args']=={'output':before['argument']} and api_entry['frontend']==hook['frontend'] and api_return['frontend']==boundary['frontend']==expected and boundary['symbol']=='initialize_compiler_environment' and boundary['args']=={},'API output overwrite/adjacent modeled state/next ABI differs')
    require(api_entry['target_state']==api_return['target_state']==state['target_state']==hook['target_state'],'API changed other target context')
    initial=(original/('frontend-data-'+hook_suffix+'.bin')).read_bytes();require(initial==(original/'frontend-data-system-info-before.bin').read_bytes() and initial[0x17b8:0x17dc]==(original/'system-info-before.bin').read_bytes(),'original API destination input differs');wanted=bytearray(initial);wanted[0x17b8:0x17dc]=raw
    stable_suffix=('%03d'%int(next(v[0]for v in traps if v[1]=='0002b834')))if '_push_declaration_context'in trial['accepted']else final
    require(bytes(wanted)==(original/'frontend-data-system-info-after.bin').read_bytes()==(original/('frontend-data-'+stable_suffix+'.bin')).read_bytes(),'API changed unknown initialized bytes before actual next mutating helper')
    require((original/'system-info-bss-before.bin').read_bytes()==(original/'system-info-bss-after.bin').read_bytes()==(original/'allocation-final-bss.bin').read_bytes(),'API changed original BSS')
    saved=(original/('frontend-snapshot-'+hook_suffix+'.bin')).read_bytes();require(saved==(original/'frontend-snapshot-system-info-before.bin').read_bytes()==(original/'frontend-snapshot-system-info-after.bin').read_bytes()==(original/('frontend-snapshot-'+final+'.bin')).read_bytes(),'API changed saved snapshot')
    require((original/('state-'+hook_suffix+'.bin')).read_bytes()==(original/('state-'+final+'.bin')).read_bytes(),'API changed unknown target support globals')
