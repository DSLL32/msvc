"""Primitive identity conversions still must use the exact field and return AX."""
import copy,json
from pathlib import Path
from state import BINARY,load
from binary import PE
from audit import publics
from symbol_type_fields import validate
from symbol_output import compare
from objects import original_walks

def check(directory):
    config,pe=load();rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();proof=validate(config,pe,rebuilt,linked);assert proof['field']==8 and proof['bits']==16
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    bad=copy.deepcopy(rebuilt);raw=bytearray(bad.data);at=bad.offset(proof['field_creation']+7);assert raw[at]==8;raw[at]=6;bad.data=bytes(raw)
    reject('BP-relative canonical source field changed despite primitive identity output',lambda:validate(config,pe,bad,linked))
    bad=copy.deepcopy(rebuilt);raw=bytearray(bad.data);at=bad.offset(proof['rebuilt_call']+7);raw[at]^=4;bad.data=bytes(raw)
    reject('BP-relative return stored through different local',lambda:validate(config,pe,bad,linked))
    bad=copy.deepcopy(rebuilt);raw=bytearray(bad.data);at=bad.offset(proof['rebuilt_call']+9);assert raw[at]==0x89;raw[at+1]=0x11;bad.data=bytes(raw)
    reject('BP-relative store uses DX rather than returned AX',lambda:validate(config,pe,bad,linked))
    directory=Path(directory);events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-symbol-output.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];compare(config,pe,events,native,old,new,True,state)
    for field,value in(('record_kind',0x200),('record_length',12),('callback_index',22)):
        rows=copy.deepcopy(native);rows[0][field]=value
        reject('next register callback '+field+' differs',lambda rows=rows:compare(config,pe,events,rows,old,new,True,state))
