"""Exact switch-table transport, typed settings and all real argv parser calls."""
import json
import re
import struct


FIELDS=('option_zb','option_bm','option_zm_percent')
OFFSETS=(0x109c,0x14ac,0x1c28)


def check_table(require,trial,metadata,snapshot,masked,target):
    record=next(row for row in trial['driver'] if row['name']=='_process_early_option')
    require((record['start'],record['end'],record['code_chunks'],record['shared'])==(0x10438d56,0x10438dda,[[0x104720fc,0x10472150]],[[0x10438de0,0x10438df0]]),'wrong code/data ownership')
    table=next(row for row in trial['ranges'] if row.get('kind')=='text_data')
    require(table=={'owner':'_process_early_option','start_rva':0x38de0,'end_rva':0x38df0,'entry':0,'kind':'text_data'} and sum(row.get('kind')=='text_data' for row in trial['ranges'])==(2 if '_match_option'in trial['accepted']else 1),'wrong restored table range/entry')
    meta=metadata['functions']['_process_early_option']['original'];targets=[0x10438d70,0x10472116,0x10438dc6,0x1047212a]
    require(meta=={'text_data':[[0x10438de0,0x10438df0]],'switches':{str(0x10438d69):targets}},'wrong switch/data metadata')
    a=0x38de0-0x1000
    require(struct.unpack_from('<4I',snapshot,a)==tuple(targets) and masked[a:a+16]==snapshot[a:a+16] and masked[a-6:a]==bytes([0xcc])*6,'actual relocated table/readback/padding differs')
    from allocation_imports import pe_read
    require(pe_read(target,0x38de0,16)==snapshot[a:a+16],'target file-backed table differs')


def check(s,d,original,native,trial,text,pre,phase,state,seed,final,symbols,traps):
    def require(value,message):s.require(value,'early switches: '+message)
    def read(name):return json.loads((native/name).read_text())
    def values(snapshot,saved=False):return [snapshot['frontend'][('saved_' if saved else '')+name] for name in FIELDS]
    def raw(suffix,saved=False):
        blob=(original/('frontend-'+('snapshot' if saved else 'data')+'-'+suffix+'.bin')).read_bytes();require(len(blob)==8520,'missing complete original initialized/saved bytes');return [struct.unpack_from('<i',blob,offset)[0] for offset in OFFSETS]
    def vector(path):
        blob=path.read_bytes();address,count=struct.unpack_from('<II',blob);cursor=8;pointers=[]
        for i in range(count):pointer,length=struct.unpack_from('<II',blob,cursor);cursor+=8+length;pointers.append(pointer)
        return pointers
    def symbol(name):return tuple(int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+name+'$',symbols,re.M).groups())
    if '_initialize_frontend_hook' in trial['accepted']:
        state=read('hook-entry.json')
        traps=traps[:next(i for i,row in enumerate(traps) if row[1]=='00038df0')+1]
        final='%03d'%int(traps[-1][0])
    check_table(require,trial,read('graph-metadata.json'),(original/'relocated-text.bin').read_bytes(),(original/'masked-text.bin').read_bytes(),s.ROOT/'toolchains/5.10/bin/C1XX.DLL')
    require(trial['early_switch_observers']=={'owner':'_process_early_option','scope':'25 real argv calls and actual returns; no environment arguments mocked','entry_rva':0x38d56,'return_rva':0x38d28,'shared_text_data_rvas':[[0x38de0,0x38df0]]},'undeclared call/return observer')
    defaults=[0,0,100];live=[43,-47,53] if seed else defaults;saved=[31,-37,41] if seed else defaults
    require(raw('000')==defaults and raw('006')==values(pre)==live,'original/live integer defaults or seeded values differ')
    require(values(pre,True)==([31,-37,41] if seed else [None]*3),'native saved input differs')
    if seed:require(raw('006',True)==saved,'original saved input differs')
    require(raw('phase')==raw('phase',True)==values(phase)==values(phase,True)==saved,'typed integer snapshot copy/restore differs')
    entries=read('early-switch-entries.json');returns=read('early-switch-returns.json');old_entries=re.findall(r'^early-option-entry index=(\d+) seq=(\d+) pc=(\w+) esp=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) return=(\w+) argument=(\w+) state=(\w+)$',text,re.M);old_returns=re.findall(r'^early-option-return index=(\d+) pc=(\w+) esp=(\w+) eax=(\w+) edi=(\w+) esi=(\w+) ebx=(\w+) values=([0-9a-f,]+) error=(\d+) fpu=(\w+)$',text,re.M)
    require(len(entries)==len(returns)==len(old_entries)==len(old_returns)==25,'missing/extra natural argv calls')
    old_pointers=vector(original/'invoke-entry-argv.bin');native_pointers=vector(native/'native-argv.bin');arguments=state['invocation']['argv'];pc,size=symbol('process_early_option');scanner,scanner_size=symbol('initialize_intermediate_files');wanted=list(saved)
    for index,(entry,result,left,right) in enumerate(zip(entries,returns,old_entries,old_returns)):
        require(entry['index']==result['index']==int(left[0])==int(right[0])==index,'call/return order differs')
        require(entry['pc']==pc and entry['args']==[native_pointers[index+1],0] and entry['formal_inventory']==[{'name':'argument','type':'char *'},{'name':'state','type':'int'}] and entry['values']==wanted,'actual native parser arguments/formals/input differ')
        require(int(left[2],16)==0x10438d56 and [int(word,16) for word in left[7:]]==[0x10438d28,old_pointers[index+1],0] and raw('%03d'%int(left[1]))==wanted,'actual original parser arguments/input differ')
        caller=entry['return_pc'];code=bytes(d.elf_byte(native,caller-5+i) for i in range(5));require(scanner<caller<scanner+scanner_size and code[0]==0xe8 and caller+struct.unpack_from('<i',code,1)[0]==pc,'native caller CALL differs')
        require(result['pc']==caller and result['esp']==entry['esp']+4 and result['eax']==0 and all(result[r]==entry[r] for r in ('edi','esi','ebx')),'native RET0/result/nonvolatile registers differ')
        require(int(right[1],16)==int(left[7],16) and int(right[2],16)==int(left[3],16)+4 and int(right[3],16)==0 and right[4:7]==left[4:7],'original RET0/result/nonvolatile registers differ')
        if arguments[index+1]=='-ZB64':wanted[0]=64
        require(result['values']==wanted and [struct.unpack('<i',struct.pack('<I',int(word,16)))[0] for word in right[7].split(',')]==wanted,'actual parser setting/result differs')
        require(entry['last_error']==result['last_error']==int(right[8])==203 and entry['fpu']==result['fpu']==(int(right[9],16)&0xffff)==0x27f,'parser error/FPU state differs')
    require(wanted==[64,saved[1],saved[2]] and raw(final)==values(state)==wanted and raw(final,True)==values(state,True)==saved,'known final setting/snapshot differs')
    before=read('early-options-entry.json')['frontend'];expected=dict(before)
    expected.update(dict(zip(FIELDS,wanted)))
    require(state['frontend']==expected and state['symbol']=='initialize_frontend_hook' and state['args']=={},'parser changed adjacent modeled state or wrong next ABI')
    require((original/'allocation-final-bss.bin').read_bytes()==(original/'early-options-final-bss.bin').read_bytes(),'parser changed original BSS')
    first='%03d'%int(old_entries[0][1]);blob=bytearray((original/('frontend-data-'+first+'.bin')).read_bytes());struct.pack_into('<i',blob,OFFSETS[0],64)
    require(bytes(blob)==(original/('frontend-data-'+final+'.bin')).read_bytes(),'parser changed unknown initialized bytes')
    require((original/('frontend-snapshot-'+first+'.bin')).read_bytes()==(original/('frontend-snapshot-'+final+'.bin')).read_bytes(),'parser changed saved snapshot bytes')
