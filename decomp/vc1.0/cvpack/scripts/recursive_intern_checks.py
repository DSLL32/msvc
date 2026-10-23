"""Current recursive interning frontier retains exact ECX/parent ownership."""
import copy,json,re
from pathlib import Path
from state import load
from objects import original_walks
from symbol_output import compare
from audit import audited_graph

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-symbol-output.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()))
    def run(text,rows):compare(config,pe,text,rows,old,state['walks'],True,state)
    run(events,native)
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    function=next(r for r in config['functions']if r['name']=='intern_type');start,end=function['extent']
    reject('recursive intern original flag-writing filler undeclared',lambda:audited_graph(pe,start,end))
    found=re.search(r'^registers eax [0-9a-f]+ ebx [0-9a-f]+ ecx ([0-9a-f]+)',events,re.M);assert found
    changed=events[:found.start(1)]+'%08x'%(int(found[1],16)+28)+events[found.end(1):]
    reject('recursive hash ECX points at a different TypeRecord',lambda:run(changed,native))
    rows=copy.deepcopy(native);rows[0]['nested_callers']=rows[0]['nested_callers'][1:]
    reject('recursive hash lacks actual intern_type caller frame',lambda:run(events,rows))
