"""The real NULL path returns unchanged, then its caller seeds warnings."""
import copy,json,re,struct
from scaler_probe import symbols
from scaler_state import fp,semantic
from allocation_imports import pe_read


def check(s,d,original,native,trial,text,pre,phase,state,seed,final,traps):
    def need(value,message):s.require(value,'environment arguments: '+message)
    def row(name):
        rows=json.loads((native/('env-arguments-'+name+'.json')).read_text())
        need(len(rows)==1,'one actual '+name+' observation');return rows[0]
    entry,returned,next_entry=(row(name)for name in ('entries','returns','next'))
    names=symbols(native/'native-executable.elf')
    need(trial['trial']in ('_read_env_arguments','_new_option_vector') and trial['environment_argument_observers']=={'entry_rva':0x39413,'return_rva':0x39021,'next_rva':0x682f,'argument':None,'next_arguments':[40,1],'scope':'actual NULL input/RET0/no state mutation, then owned caller warning writes and genuine allocation frontier; nonNULL path source+graph only'},'exact bounded natural contract')
    need(entry['pc']==names['read_env_arguments'][0] and entry['args']==[0] and entry['formal_inventory']==[{'name':'arguments','type':'char *'}],'actual parser PC/formal/NULL input')
    need(next_entry['pc']==names['new_option_vector'][0] and next_entry['args']==[40,1] and next_entry['formal_inventory']==[{'name':'capacity','type':'unsigned int'},{'name':'arena_index','type':'int'}] and state['symbol']=='new_option_vector' and state['args']=={'capacity':40,'arena_index':1},'actual next allocator PC/complete formals/inputs')
    fold=json.loads((native/'path-fold-next-entries.json').read_text())
    need(len(fold)==1 and fold[0]==entry,'same actual fold-next/parser-entry boundary')
    need(returned['pc']==entry['return_pc'] and returned['entry_esp']==entry['esp'] and returned['esp']==entry['esp']+4 and all(returned[r]==entry[r]for r in ('ebx','esi','edi','ebp')),'actual parser RET0/nonvolatile; EAX has no semantic return')
    need(entry['guard']==returned['guard'] and entry['map']==returned['map'] and entry['classes']==returned['classes'],'NULL parser changes no target/saved/external bytes')
    owner=names['process_compiler_options']
    for current in (entry,next_entry):
        pc=current['return_pc'];need(owner[0]<pc<sum(owner),'real caller owner')
        code=bytes(d.elf_byte(native,pc-5+i)for i in range(5))
        need(code[0]==0xe8 and pc+struct.unpack_from('<i',code,1)[0]==current['pc'],'actual direct CALL/target')
    rows=re.findall(r'^trap seq=(\d+) rva=00039413 .*esp=(\w+).*stack=([0-9a-f,]+)$',text,re.M)
    need(len(rows)==1,'actual original parser entry')
    seq,oldesp,words=rows[0];oldesp=int(oldesp,16)
    need([int(v,16)for v in words.split(',')][:2]==[0x10439021,0],'actual original parser NULL input/caller')
    regs=re.findall(r'^pool-registers seq=%d ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$'%(int(seq)+1),text,re.M)
    ret=re.findall(r'^env-arguments-return pc=(\w+) esp=(\w+) entry-esp=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+) error=(\d+)$',text,re.M)
    need(len(regs)==len(ret)==1 and [int(v,16)for v in ret[0][:3]]==[0x10439021,oldesp+4,oldesp] and ret[0][3:7]==regs[0] and ret[0][-1]=='203','actual original RET0/nonvolatile/error')
    need(text.count('env-arguments-observer entry=00039413 return=00039021 argument=NULL next=0000682f no-code-mock=1\n')==1,'original observer/one real call')
    raw=pe_read(original/'loaded-target.bin',0x39413,21)
    need(raw==bytes.fromhex('83ec08568b74241085f60f85139003005e83c408c3'),'actual complete NULL branch instructions')
    for suffix in ('target-BSS','words','mbctype','ctype','old-ctype','result','fp','errors','CRT-argv','CRT-environment','Invoke-argv'):
        need((original/('locale-env-return-'+suffix+'.bin')).read_bytes()==(original/('locale-fold-next-'+suffix+'.bin')).read_bytes(),'original NULL-return state/lifetime '+suffix)
    for prefix in ('frontend-data-locale-','frontend-snapshot-locale-'):
        need((original/(prefix+'env-return.bin')).read_bytes()==(original/(prefix+'fold-next.bin')).read_bytes(),'whole original NULL-return initialized/snapshot')
    warnings=json.loads((original/'options-analysis.json').read_text())['warning_defaults']
    # The actual caller writes these ten already-modeled warning bytes after
    # return and before its first allocator. No allocation result is mocked.
    next_phase='vector-0-entry'if trial.get('vector_observers')else'env-next'
    old_data=bytearray((original/'frontend-data-locale-env-return.bin').read_bytes())
    guard=copy.deepcopy(returned['guard']);new_data=bytearray.fromhex(guard['data'])
    offset=200+state['frontend']['options_data_layout']['fields']['warning_levels'][0]
    for code,flags in warnings:
        old_data[0xccb84-0xcc000+code]=flags;new_data[offset+code]=flags
    guard['data']=new_data.hex()
    need(bytes(old_data)==(original/('frontend-data-locale-'+next_phase+'.bin')).read_bytes(),'only ten known warning writes in full original initialized data')
    need(next_entry['guard']==guard and next_entry['map']==returned['map'] and next_entry['classes']==returned['classes'],'only ten known warning writes in full native target/saved/external state')
    need((original/'frontend-snapshot-locale-env-return.bin').read_bytes()==(original/('frontend-snapshot-locale-'+next_phase+'.bin')).read_bytes() and (original/'locale-env-return-target-BSS.bin').read_bytes()==(original/('locale-'+next_phase+'-target-BSS.bin')).read_bytes(),'original caller preserves full saved/BSS state')
    for suffix in ('words','mbctype','ctype','old-ctype','result','fp','errors','CRT-argv','CRT-environment','Invoke-argv'):
        need((original/('locale-env-return-'+suffix+'.bin')).read_bytes()==(original/('locale-'+next_phase+'-'+suffix+'.bin')).read_bytes(),'original caller provider/vector preservation '+suffix)
    expected_fp=semantic(fp((original/'locale-fold-next-fp.bin').read_bytes()))
    for current in (returned,next_entry):
        need(current['locale']==entry['locale'] and current['last_error']==203 and semantic(current['fp'])==semantic(entry['fp'])==expected_fp,'native provider/owner/vector/error/full architectural FP preserved')
    for label in ('env-entry','env-return','env-next'):
        for name,baseline in (('CRT-argv','native-crt-argv.bin'),('CRT-environment','native-crt-environment.bin')):
            need((native/('locale-native-'+label+'-'+name+'.bin')).read_bytes()==(native/baseline).read_bytes(),'native private ordered vector content/lifetime')
    next_trap=next(row for row in traps if row[1]=='0000682f')
    need([int(v,16)for v in next_trap[2].split(',')][:3]==[0x10439059,40,1],'original actual next allocator two arguments/caller')
    return dict(code_bytes=338,natural_NULL_path=True,nonNULL_runtime_exercised=False,new_modeled_data_bytes=0,known_caller_warning_writes=len(warnings),next_arguments=[40,1])
