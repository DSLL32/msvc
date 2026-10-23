"""Read-only rejection controls for the actual PDB read and owned graph."""
import copy,json
from pathlib import Path
from state import BINARY
from pdb_consumed import compare as consumed_compare
from pdb_read import compare as read_compare
from objects import original_walks,compare_walks

def check(config,pe,reject):
    directory=BINARY/'cases/hello-debug-dll/evidence/0048-type_server_read_records'
    assert directory.is_dir(),'missing accepted PDB reader proof'
    events=(directory/'original-events').read_text()
    state=json.loads((directory/'native-gdb.json').read_text())
    reads=json.loads((directory/'native-pdb-read-returns.json').read_text())
    native=json.loads((directory/'native-pdb-consumed.json').read_text())
    consumed=consumed_compare(config,pe,events,native,True)
    read_proof=read_compare(config,pe,events,reads,True)
    consumed.update(reads_original=read_proof,reads_native=reads)
    omitted='\n'.join(line for line in events.splitlines()if not line.startswith('pdb-consumed '))
    reject('both actual undefined-field reads omitted',lambda:consumed_compare(config,pe,omitted,[],True))
    changed=copy.deepcopy(native);changed[0]['value']^=1
    reject('actual undefined-field consumed value differs',lambda:consumed_compare(config,pe,events,changed,True))
    changed=copy.deepcopy(native);changed[0]['site']+=1
    reject('undefined-field observation from another instruction',lambda:consumed_compare(config,pe,events,changed,True))
    omitted='\n'.join(line for line in events.splitlines()if not(line.startswith('pdb-read ')and 'count 4140 'in line))
    reject('both PDB payload-return records omitted',lambda:read_compare(config,pe,omitted,reads[:1],True))
    for key,value in(('header','00'+reads[1]['header'][2:]),('returned',4139),('position',4203),('input_matches',0)):
        changed=copy.deepcopy(reads);changed[1][key]=value
        reject('PDB payload return '+key,lambda:read_compare(config,pe,events,changed,True))
    specifications=json.loads((BINARY/'walks.json').read_text());old=original_walks(events,specifications)
    item=next(r for r in specifications if r.get('pdb_graph'));name=item['name']
    old={name:old[name]};new={name:state['walks'][name]}
    def check_graph(left,right,record=consumed):return compare_walks([item],left,right,pdb_consumed=record)
    check_graph(old,new)
    changed=copy.deepcopy(consumed);changed['native'][0]['address']+=4
    reject('actual undefined-field read from another allocation',lambda:check_graph(old,new,changed))
    changed=copy.deepcopy(consumed);changed['reads_native'][1]['address']+=4
    reject('PDB payload read belongs to another allocation',lambda:check_graph(old,new,changed))
    changed=copy.deepcopy(new);changed[name][0]['objects']['index-storage-0']['address']=changed[name][0]['objects']['type-storage-1']['address']+5000
    reject('PDB allocation aliases another allocation unused tail',lambda:check_graph(old,changed))
    changed=copy.deepcopy(new);changed[name][0]['objects']['extra']=dict(address=0x1234,bytes='00')
    reject('unproved extra PDB allocation',lambda:check_graph(old,changed))
    for key,offset in(('offsets',0),('type-storage-0',8),('type-storage-1',12),('index-storage-0',0)):
        changed=copy.deepcopy(new);obj=changed[name][0]['objects'][key];raw=bytearray.fromhex(obj['bytes']);raw[offset]^=1;obj['bytes']=raw.hex()
        reject('PDB defined object byte '+key,lambda:check_graph(old,changed))
    changed=copy.deepcopy(new);obj=changed[name][0]['objects']['type-storage-1'];obj['bytes']=obj['bytes'][:-2]
    reject('PDB allocation requested extent changed',lambda:check_graph(old,changed))
    changed=copy.deepcopy(new);obj=changed[name][0]['objects']['type-storage-1'];raw=bytearray.fromhex(obj['bytes']);raw[-1]^=0xff;obj['bytes']=raw.hex()
    check_graph(old,changed) # Actual unused tail stays raw and earns no equality credit.
    print('proved: PDB unused pool tail retained without equality credit')
