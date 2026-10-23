"""Bind three actual module-address ABI arguments to the owning procedure."""
import copy,json
from pathlib import Path
from state import load
from objects import original_walks
from symbol_output import compare

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-symbol-output.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks']
    compare(config,pe,events,native,old,new,True,state)
    assert set(state['parameters'])=={'module','segment','offset'}and state['parameters']['segment']==1 and state['parameters']['offset']==0x120 and state['globals']['symbol_scope_depth']=='01000000'
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    for key,value in(('module_argument',0),('segment_argument',2),('offset_argument',0x121)):
        rows=copy.deepcopy(native);rows[0][key]=value
        reject('module-address actual '+key+' differs',lambda rows=rows:compare(config,pe,events,rows,old,new,True,state))
    wrong=events.replace('stack 5 00025172','stack 5 00025173',1)
    reject('module-address nested callback return differs',lambda:compare(config,pe,wrong,native,old,new,True,state))
    def changed_source():
        a,b=copy.deepcopy((old,new))
        for walks in(a,b):
            row=walks['type_data'][0];raw=bytearray.fromhex(row['bytes']);raw[4283+0x20:4283+0x22]=(2).to_bytes(2,'little');row['bytes']=raw.hex()
        compare(config,pe,events,native,a,b,True,state)
    reject('both procedure source segment fields differ from actual argument',changed_source)
    rows=copy.deepcopy(native);rows[0]['record_kind']=0x204
    reject('module-address callback has wrong procedure kind',lambda:compare(config,pe,events,rows,old,new,True,state))
