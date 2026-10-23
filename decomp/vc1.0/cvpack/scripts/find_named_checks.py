"""EDX identity of the uninitialized four-byte named-type output local."""
import copy,json,re
from state import BINARY,load
from stack_output import compare

def check(directory,function='find_named_type'):
    config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text())
    descriptions=next(r for r in config['functions']if r['name']==function)['parameters'];item=next(r for r in descriptions if r['name']=='named')
    words={int(m[1]):int(m[2],16)for m in re.finditer(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
    fields=re.search(r'^registers (.*)$',events,re.M)[1].split();words.update((k,int(v,16))for k,v in zip(fields[::2],fields[1::2]))
    compare(item,descriptions,words,state['parameters'],events,pe)
    def reject(label,change):
        a,b,c,d=copy.deepcopy((item,words,state['parameters'],events));a,b,c,d=change(a,b,c,d)
        try:compare(a,descriptions,b,c,d,pe)
        except(AssertionError,KeyError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def old_pointer(a,b,c,d):b['edx'if function=='find_named_type'else 3]+=4;return a,b,c,d
    def new_pointer(a,b,c,d):c['named']['address']+=4;return a,b,c,d
    def owner(a,b,c,d):c['named']['local']['owner']='convert_new_type';return a,b,c,d
    def ret(a,b,c,d):b[0]=0x266ed;return a,b,c,d
    def absent(a,b,c,d):c['named']['prebytes']='';d=re.sub(r'^output-buffer [0-9a-f]+ [0-9a-f]+\n','',d,flags=re.M);return a,b,c,d
    def reg(a,b,c,d):a['register']='ecx';return a,b,c,d
    for label,change in [('named output original '+('EDX'if function=='find_named_type'else'argument3')+' points to another local',old_pointer),('named output native pointer not local base',new_pointer),('named output wrong owning frame',owner),('named output unknown caller return',ret),('named output both missing observations',absent),('named output unsupported register transport',reg)]:reject(label,change)
    values=copy.deepcopy(state['parameters']);values['named']['prebytes']='aabbccdd'
    compare(item,descriptions,words,values,events,pe)
    print('accepted: named output undefined entry bytes retained without cross-side equality')
