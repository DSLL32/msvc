"""Actual argv scan entry and first argument/state call, before parser semantics."""
import json
import re
import struct


def check(s,d,original,native,trial,text,state,final,symbols,traps):
    def require(value,message):s.require(value,'early options: '+message)
    def read(name):return json.loads((native/name).read_text())
    def symbol(name):return tuple(int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+name+'$',symbols,re.M).groups())
    def pointers(path):
        raw=path.read_bytes();address,count=struct.unpack_from('<II',raw);cursor=8;values=[]
        for i in range(count):
            pointer,length=struct.unpack_from('<II',raw,cursor);cursor+=8+length;values.append(pointer)
        return address,values
    def call(return_pc,destination):
        code=bytes(d.elf_byte(native,return_pc-5+i) for i in range(5));require(code[0]==0xe8 and return_pc+struct.unpack_from('<i',code,1)[0]==destination,'not actual native caller CALL')
    require(trial['early_options_observers']=={'owner':'_initialize_intermediate_files','entry_rva':0x38d07,'frontier_rva':0x38d56,'purpose':'actual argv entry and first option/state ABI; full original BSS at frontier; no mocked inputs or returns'},'undeclared observer roles')
    entry=read('early-options-entry.json');parser=read('early-option-entry.json');old_vector,old_pointers=pointers(original/'invoke-entry-argv.bin');vector,native_pointers=pointers(native/'native-argv.bin')
    pc,size=symbol('initialize_intermediate_files');parser_pc,_=symbol('process_early_option');main_pc,main_size=symbol('c1xx_main')
    require(entry['symbol']=='initialize_intermediate_files' and entry['pc']==pc and entry['args']=={'argv':vector} and entry['formal_inventory']==[{'name':'argv','type':'char **'}] and entry['raw_entry_abi']['stack_arguments']==[vector],'actual scanner argv/formals differ')
    caller=entry['raw_entry_abi']['return_pc'];require(main_pc<caller<main_pc+main_size,'scanner called outside actual main');call(caller,pc)
    require(parser['symbol']=='process_early_option' and parser['pc']==parser_pc and parser['args']=={'argument':native_pointers[1],'state':0} and parser['formal_inventory']==[{'name':'argument','type':'char *'},{'name':'state','type':'int'}] and parser['raw_entry_abi']['stack_arguments']==[native_pointers[1],0],'actual first parser arguments/formals differ')
    caller=parser['raw_entry_abi']['return_pc'];require(pc<caller<pc+size,'parser called outside actual scanner');call(caller,parser_pc)
    require(state['symbol']=='process_early_option' and state['args']==parser['args'] and state['invocation']['argv'][1]=='-il','wrong natural first argument/parser frontier')
    before=next(row for row in traps if row[1]=='00038d07');old_args=[int(word,16) for word in before[2].split(',')];new_args=[int(word,16) for word in traps[-1][2].split(',')]
    require(old_args[:2]==[0x10438c35,old_vector] and new_args[:3]==[0x10438d28,old_pointers[1],0],'actual original scanner/parser caller/formals differ')
    require(entry['frontend']==parser['frontend']==state['frontend'] and entry['target_state']==parser['target_state']==state['target_state'],'scanner changed modeled state or caller context before parser')
    require(entry['control_word']==parser['control_word']==state['control_word']==0x27f and state['target_state']['last_error']==203,'scanner changed FPU/error state')
    require((original/'allocation-final-bss.bin').read_bytes()==(original/'early-options-final-bss.bin').read_bytes(),'scanner changed unknown original BSS before parser')
    for name in ('frontend-data','frontend-snapshot'):
        require((original/('%s-%03d.bin'%(name,int(before[0])))).read_bytes()==(original/('%s-%s.bin'%(name,final))).read_bytes(),'scanner changed original initialized/saved bytes')
