"""Register type field4 and the ninth typed resolver call must remain exact."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import BINARY,load
from binary import PE
from audit import publics
from symbol_type_fields import validate
from resolver_returns import compare as compare_resolver,parse as parse_resolver
from symbol_output import compare as compare_output
from objects import original_walks

def check(directory):
    config,pe=load();rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();proof=validate(config,pe,rebuilt,linked,'convert_register_symbol');assert proof['field']==4
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    bad=copy.deepcopy(rebuilt);raw=bytearray(bad.data);at=bad.offset(proof['field_creation']+7);assert raw[at]==4;raw[at]=6;bad.data=bytes(raw)
    reject('register converter selects register-number field rather than type',lambda:validate(config,pe,bad,linked,'convert_register_symbol'))
    bad=copy.deepcopy(rebuilt);raw=bytearray(bad.data);at=bad.offset(proof['rebuilt_call']+7);raw[at]^=4;bad.data=bytes(raw)
    reject('register conversion stores through wrong field pointer',lambda:validate(config,pe,bad,linked,'convert_register_symbol'))
    bad=copy.deepcopy(rebuilt);raw=bytearray(bad.data);at=bad.offset(proof['rebuilt_call']+10);assert raw[at]==1;raw[at]=0x11;bad.data=bytes(raw)
    reject('register conversion stores DX rather than returned AX',lambda:validate(config,pe,bad,linked,'convert_register_symbol'))
    directory=Path(directory);events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];native=json.loads((directory/'native-resolver-returns.json').read_text());original=parse_resolver(events)
    returned=compare_resolver(config,pe,events,native,old,new,'hello-debug-dll');assert len(returned)==9 and returned[-1]['type']==96 and returned[-1]['parent_type']==4097
    a,b=copy.deepcopy((original,native))
    for r in(a,b):r['entries'].pop();r['returns'].pop()
    def missing():
        with patch('resolver_returns.parse',return_value=a):compare_resolver(config,pe,events,b,old,new,'hello-debug-dll')
    reject('both ninth module-local resolver calls omitted',missing)
    rows=json.loads((directory/'native-symbol-output.json').read_text());rows[0]['record_kind']=7
    reject('next end callback has wrong record kind',lambda:compare_output(config,pe,events,rows,old,new,True,state))
    rows=json.loads((directory/'native-symbol-output.json').read_text());rows[0]['callback_index']=9
    reject('next end callback has wrong table selection',lambda:compare_output(config,pe,events,rows,old,new,True,state))
