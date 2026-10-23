"""Actual allocation/frame ownership and own-tail-definedness controls."""
import copy,json,re
from pathlib import Path
from state import BINARY,load
from objects import original_walks
from symbol_output import compare,parse

def check(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());native=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());walks=original_walks(events,spec)
    def run(e=events,n=native,a=walks,b=state['walks']):return compare(config,pe,e,n,a,b,True,state)
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


def check_search_write(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows,s=state):return compare(config,pe,e,n,old,s['walks'],True,s)
    proof=run();assert proof['defined']==16
    def deny(label,action):
        try:action()
        except AssertionError:print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    for key,value in [('record_kind',4),('record_length',25),('callback_index',30)]:
        changed=copy.deepcopy(rows);changed[0][key]=value
        deny('search-write wrong callback '+key,lambda:run(n=changed))
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['postbytes']);raw[16]^=1;changed[0]['postbytes']=raw.hex()
    deny('search-write changed own unwritten suffix',lambda:run(n=changed))
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['postbytes']);raw[12]^=1;changed[0]['postbytes']=raw.hex()
    deny('search-write wrong initialized segment',lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0]['cursor']=changed[0]['base']+40
    deny('search-write invented later copied prefix',lambda:run(n=changed))
    changed=copy.deepcopy(state);changed['globals']['symbol_input']['address']+=24
    deny('search-write wrong actual parent source cursor',lambda:run(s=changed))
    # Native undefined contents can differ from the original only if their own
    # before/after bytes stay equal beyond the proven16-byte prefix.
    changed=copy.deepcopy(rows)
    for field in('before','postbytes'):
        raw=bytearray.fromhex(changed[0][field]);raw[16]^=1;changed[0][field]=raw.hex()
    run(n=changed)


def check_object_copy(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(n=rows,s=state):return compare(config,pe,events,n,old,s['walks'],True,s)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except AssertionError:print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['postbytes']);raw[16]^=1;changed[0]['postbytes']=raw.hex()
    deny('object-name actual copied bytes differ',lambda:run(changed))
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['postbytes']);raw[40]^=1;changed[0]['postbytes']=raw.hex()
    deny('object-name own untouched tail changed',lambda:run(changed))
    changed=copy.deepcopy(rows);changed[0]['cursor']+=4
    deny('object-name observed cursor differs from actual global',lambda:run(changed))
    changed=copy.deepcopy(state);changed['globals']['symbol_output']['address']+=4
    deny('object-name actual global differs from observed cursor',lambda:run(s=changed))
    changed=copy.deepcopy(rows);changed[0]['callback_index']=31
    deny('object-name copy wrong next callback owner',lambda:run(changed))
    changed=copy.deepcopy(rows)
    for field in('before','postbytes'):
        raw=bytearray.fromhex(changed[0][field]);raw[40]^=1;changed[0][field]=raw.hex()
    run(changed)
