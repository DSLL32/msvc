"""Completed-loop typed observations and exact one-past identity controls."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load,BINARY
from binary import PE
from audit import publics
from objects import original_walks,pointer_identity
from module_address_returns import compare as module_compare,parse as module_parse
from symbol_output import compare as symbol_compare
from completed_symbol_input import compare,validate

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks']
    modules=json.loads((directory/'native-module-address.json').read_text());original=module_parse(events)
    assert len(module_compare(config,pe,events,modules,old,new,'hello-debug-dll'))==3
    symbol=symbol_compare(config,pe,events,json.loads((directory/'native-symbol-output.json').read_text()),old,new,True,state)
    accepted=json.loads((directory/'proof.json').read_text())
    definitions={'type_data':accepted['observed_objects']['type_data'][0]['allocation']}
    left=old['type_data'][0]['address']+4746;right=new['type_data'][0]['address']+4746
    def run(change):
        args=copy.deepcopy([config,pe,'symbol_input',left,right,old,new,state,definitions,events,symbol]);change(args);return compare(*args)
    run(lambda a:None)
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError,StopIteration):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def module_run(change):
        a,b=copy.deepcopy((original,modules));change(a,b)
        with patch('module_address_returns.parse',return_value=a):return module_compare(config,pe,events,b,old,new,'hello-debug-dll')
    reject('all completed procedure returns absent on both sides',lambda:module_run(lambda a,b:(a.clear(),b.clear())))
    reject('second and third procedures absent on both sides',lambda:module_run(lambda a,b:(a.__delitem__(slice(1,None)),b.__delitem__(slice(1,None)))))
    reject('DllMain procedure absent on both sides',lambda:module_run(lambda a,b:(a.pop(),b.pop())))
    reject('HelloMain and DllMain return order swapped',lambda:module_run(lambda a,b:(a.reverse(),b.reverse())))
    reject('DllMain module-test offset wrong on both sides',lambda:module_run(lambda a,b:[rows[2].update(offset=0)for rows in(a,b)]))
    reject('DllMain actual source omitted on both sides',lambda:module_run(lambda a,b:[rows[2].update(record='')for rows in(a,b)]))
    reject('one-past identity reused for a different global',lambda:run(lambda a:a.__setitem__(2,'symbol_output')))
    reject('one-past identity outside exact frontier',lambda:run(lambda a:a[7]['frames'].__setitem__(0,'convert_procedure32')))
    reject('one-past owner absent',lambda:run(lambda a:a[5]['type_data'].clear()))
    reject('one-past observed owner base moved',lambda:run(lambda a:a[6]['type_data'][0].update(address=a[6]['type_data'][0]['address']+1)))
    reject('one-past actual pointer one byte beyond',lambda:run(lambda a:a.__setitem__(4,right+1)))
    reject('one-past actual pointer one byte short',lambda:run(lambda a:a.__setitem__(4,right-1)))
    reject('one-past requested allocation size changed',lambda:run(lambda a:a[8]['type_data'].update(size=4747)))
    reject('one-past allocation read extent incomplete',lambda:run(lambda a:a[8]['type_data'].update(defined_ranges=[[0,4745]])))
    reject('one-past defining read site changed',lambda:run(lambda a:a[8]['type_data']['write_evidence'][0].update(site=0)))
    reject('one-past native loop exit evidence missing',lambda:run(lambda a:a[10]['native'].pop('completed_input')))
    reject('one-past allocation contains invented extra byte',lambda:run(lambda a:a[6]['type_data'][0].update(bytes=a[6]['type_data'][0]['bytes']+'00')))
    reject('generic resolver must not grant one-past ownership',lambda:pointer_identity({'input':new['type_data']},right))
    rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();validate(config,pe,rebuilt,linked)
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(rebuilt.read(linked['_pack_new_types'],0x500),linked['_pack_new_types']));call=next(i for i,r in enumerate(code)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_link_symbol_scopes'])
    for instruction,label in((code[call-7],'completed cursor write targets wrong global'),(code[call-2],'completed scope ABI size uses wrong local')):
        altered=copy.deepcopy(rebuilt);raw=bytearray(altered.data);offset=altered.offset(instruction.address);raw[offset+len(instruction.bytes)-1]^=4;altered.data=bytes(raw)
        reject(label,lambda:validate(config,pe,altered,linked))


def check_scope_links(directory):
    from scope_links import compare as compare_links
    config,pe=load();p=json.loads((BINARY/config['symbol_output_observations']['evidence']).read_text())
    rows=json.loads((Path(directory)/'native-symbol-output.json').read_text());assert len(rows)==1 and rows[0]['frontier']=='record_symbol_offsets'
    after=bytes.fromhex(rows[0]['postbytes']);compare_links(p,after)
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    for at,value,label in((8,0,'missing search first-procedure link'),(112,1,'invented Paint parent'),(116,604,'wrong Paint end scope'),(120,496,'skipped HelloMain next link'),(304,292,'wrong HelloMain end'),(308,0,'missing DllMain next link'),(504,0,'unclosed DllMain scope'),(520,1,'unrelated defined procedure byte changed')):
        raw=bytearray(after);raw[at:at+4]=value.to_bytes(4,'little')
        reject('both scope outputs '+label,lambda:compare_links(p,raw))
    wrong=copy.deepcopy(p);wrong['scope_link_writes']['writes'][0][2]=296
    reject('scope link preimage-policy target widened',lambda:compare_links(wrong,after))
    rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();validate(config,pe,rebuilt,linked)
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(rebuilt.read(linked['_pack_new_types'],0x500),linked['_pack_new_types']));call=next(i for i,r in enumerate(code)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_record_symbol_offsets'])
    for ins,label in((code[call-5],'successor module loaded from wrong formal'),(code[call-2],'successor size loaded from wrong local')):
        altered=copy.deepcopy(rebuilt);raw=bytearray(altered.data);offset=altered.offset(ins.address);raw[offset+len(ins.bytes)-1]^=4;altered.data=bytes(raw)
        reject(label,lambda:validate(config,pe,altered,linked))
