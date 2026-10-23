"""Concrete first page lifetime: no undefined-byte equality or unknown owners."""
import copy,hashlib,json,tempfile
from pathlib import Path
from state import BINARY,load
from objects import original_walks
from type_pages import compare,parse,validate


def check(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-type-pages.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows,a=old,s=state):return compare(config,pe,e,n,a,s['walks'],s,'hello-debug-dll')
    assert run()['defined_ranges']==[]
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    without='\n'.join(l for l in events.splitlines()if not l.startswith(('type-page-allocation ','type-page-output ','type-page-rearm ')))
    deny('both type page lifetimes omitted',lambda:run(without,[]))
    deny('type page allocation omitted',lambda:run('\n'.join(l for l in events.splitlines()if not l.startswith('type-page-allocation '))))
    deny('type page allocation rearm omitted',lambda:run(events.replace('type-page-rearm 000199d7 000199da','')))
    for key,value in(('size',2047),('index',4097),('capacity',511),('owner',rows[0]['owner']+28),('frontier','save_type')):
        changed=copy.deepcopy(rows);changed[0][key]=value
        deny('type page wrong '+key,lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0]['before']=''
    deny('type page missing raw allocation bytes',lambda:run(n=changed))
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['postbytes']);raw[100]^=1;changed[0]['postbytes']=raw.hex()
    deny('type page unproved write before copy returns',lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0]['roots_before']='01000000'+changed[0]['roots_before'][8:]
    deny('type page roots already published at allocation',lambda:run(n=changed))
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['roots_after']);raw[0]^=4;changed[0]['roots_after']=raw.hex()
    deny('type page wrong root publication',lambda:run(n=changed))
    changed=copy.deepcopy(state);changed['globals']['type_pages'][1]=dict(null=False,address=rows[0]['base'])
    deny('type page duplicated extra static root',lambda:run(s=changed))
    changed_old=copy.deepcopy(old);changed_state=copy.deepcopy(state)
    for walks,base in((changed_old,parse(events)[0][0]['base']),(changed_state['walks'],rows[0]['base'])):
        walks['unexpected_owner']=[dict(address=base+16,bytes='00'*4)]
    deny('type page overlaps another allocation owner',lambda:run(a=changed_old,s=changed_state))
    changed_old=copy.deepcopy(old);changed_state=copy.deepcopy(state)
    for walks in(changed_old,changed_state['walks']):
        table=walks['local_type_records'][0];pointer=int.from_bytes(bytes.fromhex(table['bytes'])[19*28:19*28+4],'little')
        block=walks['current_type_server'][0]['objects']['type-storage-1'];raw=bytearray.fromhex(block['bytes']);raw[pointer-block['address']+6]^=1;block['bytes']=raw.hex()
    deny('both pending copied records differ from frozen defined input',lambda:run(a=changed_old,s=changed_state))
    changed=copy.deepcopy(rows)
    for field in('before','postbytes'):
        raw=bytearray.fromhex(changed[0][field]);raw[100]^=1;changed[0][field]=raw.hex()
    run(n=changed) # Distinct uninitialized contents are retained without credit.
    changed=copy.deepcopy(config);changed.pop('type_page_observations')
    deny('type page proof missing',lambda:validate(changed,pe))
    changed=copy.deepcopy(config);changed['type_page_observations']['evidence_sha256']='0'*64
    deny('type page proof stale',lambda:validate(changed,pe))
    proof=json.loads((BINARY/config['type_page_observations']['evidence']).read_text())
    scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
    with tempfile.TemporaryDirectory(prefix='type-page-proof-',dir=scratch)as folder:
        path=Path(folder)/'proof.json'
        for key,value in(('root_count',128),('native_allocation_line',1),('data.json_sha256','0'*64)):
            altered=copy.deepcopy(proof);altered[key]=value;path.write_text(json.dumps(altered))
            current=copy.deepcopy(config);current['type_page_observations']=dict(evidence=str(path),evidence_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            deny('type page wrong policy '+key,lambda:validate(current,pe))
