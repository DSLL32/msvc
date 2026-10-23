"""A copied recursive root is published before its ordinary hash node exists."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from buffer_storage import compare,parse
from global_type_cache import compare as compare_cache
from symbol_output import compare as compare_symbol

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-buffer-storage.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];original=parse(events)
    context=compare(config,pe,events,native,old,new,state,'hello-debug-dll');assert len(context['comparison']['copies'])==72 and len(context['comparison']['hash_nodes'])==71 and context['comparison']['unpublished_type_indices']==[0x1047]
    def run(change):
        a,b,c,d=copy.deepcopy((original,native,old,new));change(a,b,c,d)
        with patch('buffer_storage.parse',return_value=a):compare(config,pe,events,b,c,d,state,'hello-debug-dll')
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    reject('both pending recursive root copies omitted',lambda:run(lambda a,b,c,d:[r[k].pop()for r in(a,b)for k in('entries','returns')]))
    reject('pending root waiver hides a missing ordinary hash node',lambda:run(lambda a,b,c,d:[r['hash_nodes'].pop(next(iter(r['hash_nodes'])))for r in(a,b)]))
    def fabricated(a,b,c,d):
        for r in(a,b):r['hash_nodes']['fabricated-pending-root']=copy.deepcopy(next(iter(r['hash_nodes'].values())))
    reject('ordinary hash node fabricated before its constructor',lambda:run(fabricated))
    def wrong_root(a,b,c,d):
        for walks in(c,d):
            primary=next(r for r in walks['type_work_a'][0]['objects'].values()if int.from_bytes(bytes.fromhex(r['bytes'])[8:10],'little')==4209)
            provider=bytes.fromhex(primary['bytes'])[4:8];table=walks['local_type_records'][0];raw=bytearray.fromhex(table['bytes']);at=(4209-4096)*28;raw[at:at+4]=provider;table['bytes']=raw.hex()
    reject('both copied roots lack actual data-pointer publication',lambda:run(wrong_root))
    def wrong_index(a,b,c,d):
        row=d['local_type_records'][0];raw=bytearray.fromhex(row['bytes']);at=(4209-4096)*28+4;raw[at:at+2]=(0x1046).to_bytes(2,'little');row['bytes']=raw.hex()
    reject('pending root index is not its observed append ordinal',lambda:run(wrong_index))
    def cache_missing():
        a,b=copy.deepcopy((old,new))
        for walks in(a,b):
            root=walks['type_work_b'][0];key=next(k for k,r in root['objects'].items()if int.from_bytes(bytes.fromhex(r['bytes'])[8:10],'little')==0x1047);del root['objects'][key]
        x,y=a['type_work_b'][0],b['type_work_b'][0];compare_cache(bytearray.fromhex(x['bytes']),bytearray.fromhex(y['bytes']),x,y,a,b,context)
    reject('both newly published recursive name-cache nodes missing',cache_missing)
    rows=json.loads((directory/'native-symbol-output.json').read_text());rows[0]['named_lookup_result']=3
    reject('uninitialized named output falsely treated as a match',lambda:compare_symbol(config,pe,events,rows,old,new,True,state))
