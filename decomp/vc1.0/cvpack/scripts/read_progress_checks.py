"""Frozen209-byte partial-read proof controls, never an original execution."""
import copy,json
from state import BINARY
from read_progress import compare,validate

def check(config,pe,reject):
    directory=BINARY/'cases/hello-debug-dll/evidence/0055-reset_type_hash'
    assert directory.is_dir(),'missing accepted pending-read evidence'
    events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-read-progress.json').read_text())
    frontier=next(r for r in config['functions']if r['name']=='advance_file_page')
    compare(config,pe,events,native,frontier,state)
    omitted='\n'.join(line for line in events.splitlines()if not line.startswith('read-progress '))
    reject('both pending-read progress observations missing',lambda:compare(config,pe,omitted,[],frontier,state))
    for key,value in(('count',208),('copied',196),('remaining',13),('caller',0x2a3de),('readreturn',0x17e6f),('address',native[0]['address']+1),('slot',native[0]['slot']+40),('cursor',native[0]['cursor']-1),('position',53247),('module',4),('returned',True),('pending',False)):
        changed=copy.deepcopy(native);changed[0][key]=value
        reject('pending-read wrong '+key,lambda:compare(config,pe,events,changed,frontier,state))
    changed=copy.deepcopy(native);raw=bytearray.fromhex(changed[0]['bytes']);raw[197]^=1;changed[0]['bytes']=raw.hex()
    reject('pending read mutates its own unwritten tail',lambda:compare(config,pe,events,changed,frontier,state))
    changed=copy.deepcopy(native);raw=bytearray.fromhex(changed[0]['bytes']);raw[196]^=1;changed[0]['bytes']=raw.hex()
    reject('pending read defined byte differs',lambda:compare(config,pe,events,changed,frontier,state))
    changed=copy.deepcopy(state);changed['pending_read_entries']=[]
    reject('pending native entry/prebuffer missing',lambda:compare(config,pe,events,native,frontier,changed))
    changed=copy.deepcopy(state);changed['walks']['symbol_data'][0]['address']+=1
    reject('pending destination has different allocation identity',lambda:compare(config,pe,events,native,frontier,changed))
    changed=copy.deepcopy(config);changed['partial_read_observations']['evidence_sha256']='0'*64
    reject('pending-read source/config evidence stale',lambda:validate(changed,pe))
