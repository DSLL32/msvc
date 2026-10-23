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


def check_nested_udt(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows):return compare(config,pe,e,n,old,state['walks'],True,state)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    deny('nested UDT wrong immediate return',lambda:run(events.replace('stack 0 0002584c','stack 0 0002584d')))
    deny('nested UDT wrong outer return',lambda:run(events.replace('stack 2 00025172','stack 2 00025173')))
    deny('nested UDT wrong callback register',lambda:run(re.sub(r'(^registers .* edx )0001afe0',r'\g<1>0001afe1',events,flags=re.M)))
    changed=copy.deepcopy(rows);changed[0]['nested_caller']='convert_object_name'
    deny('nested UDT wrong native parent frame',lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0].pop('nested_caller')
    deny('nested UDT missing native frame witness',lambda:run(n=changed))


def check_compact(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows):return compare(config,pe,e,n,old,state['walks'],True,state)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    for slot,address in[(0,0x26509),(2,0x25830),(4,0x25172)]:
        deny('compact wrong nested return '+str(slot),lambda slot=slot,address=address:run(events.replace('stack %d %08x'%(slot,address),'stack %d %08x'%(slot,address+1))))
    changed=copy.deepcopy(rows);changed[0]['nested_callers'].reverse()
    deny('compact wrong native nested frames',lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0]['type_index']+=1
    deny('compact wrong actual native type',lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0]['record_length']=34
    deny('compact stale earlier UDT length',lambda:run(n=changed))
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['postbytes']);raw[40]^=1;changed[0]['postbytes']=raw.hex()
    deny('compact changed own untouched output tail',lambda:run(n=changed))


def check_pointer(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows):return compare(config,pe,e,n,old,state['walks'],True,state)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    for slot,address in[(0,0x26a7a),(9,0x26509),(11,0x25830),(13,0x25172)]:
        deny('pointer compact wrong nested return '+str(slot),lambda slot=slot,address=address:run(events.replace('stack %d %08x'%(slot,address),'stack %d %08x'%(slot,address+1))))
    changed=copy.deepcopy(rows);changed[0]['nested_callers'].reverse()
    deny('pointer compact wrong native frame chain',lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0]['type_record']+=28
    deny('pointer compact wrong typed record owner',lambda:run(n=changed))
    changed=copy.deepcopy(rows);changed[0]['type_index']+=1
    deny('pointer compact wrong native active type',lambda:run(n=changed))
    changed=copy.deepcopy(rows);raw=bytearray.fromhex(changed[0]['postbytes']);raw[40]^=1;changed[0]['postbytes']=raw.hex()
    deny('pointer compact changed own undefined output tail',lambda:run(n=changed))


def check_type_list(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows):return compare(config,pe,e,n,old,state['walks'],True,state)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    for slot,address in[(0,0x26cfa),(9,0x2711d),(15,0x26a7a),(24,0x26509),(26,0x25830),(28,0x25172)]:
        deny('type-list wrong nested return '+str(slot),lambda slot=slot,address=address:run(events.replace('stack %d %08x'%(slot,address),'stack %d %08x'%(slot,address+1))))
    changed=copy.deepcopy(rows);changed[0]['nested_callers'].reverse()
    deny('type-list wrong native frame chain',lambda:run(n=changed))
    for key in('type_index','list_index','list_count'):
        changed=copy.deepcopy(rows);changed[0][key]+=1
        deny('type-list wrong actual '+key,lambda:run(n=changed))


def check_stored_list(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows):return compare(config,pe,e,n,old,state['walks'],True,state)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    for slot,address in[(0,0x27ab1),(12,0x26cfa),(21,0x2711d),(27,0x26a7a),(36,0x26509),(38,0x25830),(40,0x25172)]:
        deny('stored-list wrong nested return '+str(slot),lambda slot=slot,address=address:run(events.replace('stack %d %08x'%(slot,address),'stack %d %08x'%(slot,address+1))))
    changed=copy.deepcopy(rows);changed[0]['nested_callers'].reverse()
    deny('stored-list wrong native frame chain',lambda:run(n=changed))
    for key,delta in(('type_index',1),('list_count',1),('type_record',28)):
        changed=copy.deepcopy(rows);changed[0][key]+=delta
        deny('stored-list wrong actual '+key,lambda:run(n=changed))


def check_type_hash(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows):return compare(config,pe,e,n,old,state['walks'],True,state)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    for slot,address in[(0,0x19590),(6,0x27ab1),(18,0x26cfa),(27,0x2711d),(33,0x26a7a),(42,0x26509),(44,0x25830),(46,0x25172)]:
        deny('type-hash wrong nested return '+str(slot),lambda slot=slot,address=address:run(events.replace('stack %d %08x'%(slot,address),'stack %d %08x'%(slot,address+1))))
    changed=copy.deepcopy(rows);changed[0]['nested_callers'].reverse()
    deny('type-hash wrong native frame chain',lambda:run(n=changed))
    for key,delta in(('list_count',1),('type_record',28),('record',1)):
        changed=copy.deepcopy(rows);changed[0][key]+=delta
        deny('type-hash wrong actual '+key,lambda:run(n=changed))


def check_saved_type(pair):
    pair=Path(pair);config,pe=load();events=(pair/'original-events').read_text();state=json.loads((pair/'native-gdb.json').read_text());rows=json.loads((pair/'native-symbol-output.json').read_text())
    spec=json.loads((pair/'walks.json').read_text());old=original_walks(events,spec)
    def run(e=events,n=rows):return compare(config,pe,e,n,old,state['walks'],True,state)
    assert run()['defined']==40
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    for slot,address in[(0,0x196fa),(6,0x27ab1),(18,0x26cfa),(27,0x2711d),(33,0x26a7a),(42,0x26509),(44,0x25830),(46,0x25172)]:
        deny('saved-type wrong nested return '+str(slot),lambda slot=slot,address=address:run(events.replace('stack %d %08x'%(slot,address),'stack %d %08x'%(slot,address+1))))
    changed=copy.deepcopy(rows);changed[0]['nested_callers'].reverse()
    deny('saved-type wrong native frame chain',lambda:run(n=changed))
    for key,delta in(('list_count',1),('type_record',28),('bucket',1)):
        changed=copy.deepcopy(rows);changed[0][key]+=delta
        deny('saved-type wrong actual '+key,lambda:run(n=changed))
