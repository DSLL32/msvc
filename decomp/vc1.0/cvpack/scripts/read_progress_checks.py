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


def check_nested(config,pe,reject,paths=None):
    if paths:
        original,native=paths;events=(original/'events').read_text();state=json.loads((native/'gdb.json').read_text());records=json.loads((native/'read-progress.json').read_text())
    else:
        directory=BINARY/'cases/hello-debug-dll/evidence/0056-advance_file_page'
        assert directory.is_dir(),'missing accepted nested pending-read evidence'
        events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());records=json.loads((directory/'native-read-progress.json').read_text())
    frontier=next(r for r in config['functions']if r['name']=='flush_file_page')
    compare(config,pe,events,records,frontier,state)
    for key,value in(('shift',4),('frontier',0x2a5c0),('innerreturn',0x2a607),('caller',0x2a3de)):
        changed=copy.deepcopy(records);changed[0][key]=value
        reject('pending nested wrong '+key,lambda:compare(config,pe,events,changed,frontier,state))
    changed=copy.deepcopy(state);changed['frames'][1]='release_page'
    reject('pending nested wrong owning caller',lambda:compare(config,pe,events,records,frontier,changed))
    changed=copy.deepcopy(state);del changed['parameters']['page']
    reject('pending nested page owner missing',lambda:compare(config,pe,events,records,frontier,changed))
    for word in(0,2,3,4,5,14,16,18):
        import re
        match=re.search(r'^stack %d ([0-9a-f]+)$'%word,events,re.M);assert match
        changed=events[:match.start(1)]+('%08x'%(int(match[1],16)+1))+events[match.end(1):]
        reject('pending nested raw frame word '+str(word),lambda:compare(config,pe,changed,records,frontier,state))


def check_types(config,pe,reject,paths=None):
    if paths:
        old,new=paths;events=(old/'events').read_text();state=json.loads((new/'gdb.json').read_text());records=json.loads((new/'read-progress.json').read_text())
    else:
        directory=BINARY/'cases/hello-debug-dll/evidence/0058-read_types'
        assert directory.is_dir(),'missing accepted pending type-read evidence'
        events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());records=json.loads((directory/'native-read-progress.json').read_text())
    frontier=next(r for r in config['functions']if r['name']=='release_page')
    compare(config,pe,events,records,frontier,state)
    changed=copy.deepcopy(state);del changed['pending_read_entries'][0]['page_entry']
    omitted='\n'.join(line for line in events.splitlines()if not line.startswith('read-page-entry '))
    reject('both actual Page flags entry observations missing',lambda:compare(config,pe,omitted,records,frontier,changed))
    changed=copy.deepcopy(state);changed['pending_read_entries'][0]['page_entry']['flags']^=1
    reject('pending type read changes allocated flag before clear',lambda:compare(config,pe,events,records,frontier,changed))
    page=state['pending_read_entries'][0]['page_entry']['page']
    changed=copy.deepcopy(state);node=next(r for r in changed['walks']['used_pages']if r['address']==page);raw=bytearray.fromhex(node['bytes']);raw[20]^=1;node['bytes']=raw.hex()
    reject('pending type read clears another Page flag',lambda:compare(config,pe,events,records,frontier,changed))
    changed=copy.deepcopy(state);other=copy.deepcopy(records)
    changed['walks']['type_data'][0]['address']=page
    changed['pending_read_entries'][0]['address']=page;changed['pending_read_entries'][0]['page_entry']['address']=page
    other[0]['address']=other[0]['destination']=page
    reject('pending type buffer aliases Page allocation',lambda:compare(config,pe,events,other,frontier,changed))
    changed=copy.deepcopy(records);changed[0]['site']=0x17e69
    reject('pending type read uses public-read site/owner',lambda:compare(config,pe,events,changed,frontier,state))
    from reads import compare as read_compare,validate as validate_reads
    native=copy.deepcopy(state['pending_read_entries'][0]);native['count']=4747
    entry=next(line for line in events.splitlines()if line.startswith('read-entry 00017dc9 ')).split();entry[4]='4747'
    page_entry=next(line for line in events.splitlines()if line.startswith('read-page-entry '))
    oversized=' '.join(entry)+'\n'+page_entry+'\nread-rearm 00017dc9 0002a400\n'
    try:read_compare(oversized,[native],validate_reads(config,pe))
    except AssertionError as error:assert str(error)=='read exceeds site limit',error
    else:raise AssertionError('accepted4747-byte read beyond exact site limit')
    print('rejected: semantic read limit4746 despite transport8192')
