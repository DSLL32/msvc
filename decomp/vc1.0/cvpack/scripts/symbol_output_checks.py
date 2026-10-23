"""Actual allocation/frame ownership and own-tail-definedness controls."""
import copy,json,re
from pathlib import Path
from state import BINARY,load
from objects import original_walks
from symbol_output import compare,parse

def check(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());native=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());walks=original_walks(events,spec)
    def run(e=events,n=native,a=walks,b=state['walks']):return compare(config,pe,e,n,a,b,True)
    run()
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    without='\n'.join(l for l in events.splitlines()if not l.startswith(('symbol-allocation ','symbol-allocation-rearm ','symbol-output ')))
    deny('both packed symbol observations missing',lambda:run(without,[]))
    deny('packed output missing actual allocation',lambda:run('\n'.join(l for l in events.splitlines()if not l.startswith('symbol-allocation '))))
    deny('packed output missing return-site rearm',lambda:run(events.replace('symbol-allocation-rearm 0002504e 00025052','')))
    for key,value in [('size',5112),('input',1),('entry',1),('module',1),('number',4),('padding',366),('skipped',1),('cursor',1),('parent','unknown')]:
        changed=copy.deepcopy(native);changed[0][key]=value
        deny('packed output wrong '+key,lambda:run(n=changed))
    changed=copy.deepcopy(native);raw=bytearray.fromhex(changed[0]['postbytes']);raw[4]^=1;changed[0]['postbytes']=raw.hex()
    deny('packed output changed own unwritten tail',lambda:run(n=changed))
    changed=copy.deepcopy(native);raw=bytearray.fromhex(changed[0]['postbytes']);raw[0]^=1;changed[0]['postbytes']=raw.hex()
    deny('packed output changed defined header',lambda:run(n=changed))
    changed=copy.deepcopy(native);changed[0]['before']=''
    deny('packed output missing raw allocation entry',lambda:run(n=changed))
    changed=events.replace('before '+parse(events)[0][0]['before'],'before 00')
    deny('packed output original entry extent truncated',lambda:run(changed))
    old=parse(events)[0][0];changed=events.replace('esp %08x before'%old['esp'],'esp %08x before'%(old['esp']+4))
    deny('packed output allocation from wrong frame',lambda:run(changed))
    # Different undefined bytes are allowed only when they stay unchanged
    # within that same observed allocation; they gain no equality credit.
    changed=copy.deepcopy(native)
    for field in('before','postbytes'):
        raw=bytearray.fromhex(changed[0][field]);raw[4]^=1;changed[0][field]=raw.hex()
    run(n=changed)
