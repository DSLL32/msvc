"""Full-width recursive hash return/cache and compiler switch-exit coverage."""
import copy,json,re
from pathlib import Path
from state import BINARY,load
from objects import original_walks
from symbol_output import compare
from audit import audited_graph,publics
from binary import PE

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-symbol-output.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()))
    def run(text=events,rows=native,a=old,b=state['walks']):compare(config,pe,text,rows,a,b,True,state)
    run()
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    function=next(r for r in config['functions']if r['name']=='hash_recursive_type');linked=publics();rebuilt=PE(BINARY/'build/CVPACK.EXE');start=linked[function['symbol']];end=min(v for v in linked.values()if v>start)
    audited_graph(rebuilt,start,end,roots=[start+r for r in function['rebuilt_extra_roots']])
    reject('recursive hash missing canonical implicit switch-break root',lambda:audited_graph(rebuilt,start,end))
    found=re.search(r'^registers eax ([0-9a-f]+)',events,re.M);assert found
    text=events[:found.start(1)]+'%08x'%(int(found[1],16)|0x10000)+events[found.end(1):]
    reject('recursive hash return upper word differs despite matching AX',lambda:run(text))
    rows=copy.deepcopy(native);rows[0]['hash_bucket']+=1
    reject('recursive hash actual caller bucket differs from cache',lambda:run(rows=rows))
    a,b=copy.deepcopy((old,state['walks']));at=(4209-4096)*28+10
    for walks in(a,b):
        row=walks['local_type_records'][0];raw=bytearray.fromhex(row['bytes']);raw[at]^=1;row['bytes']=raw.hex()
    reject('both recursive hash caches differ from actual returned value',lambda:run(a=a,b=b))
