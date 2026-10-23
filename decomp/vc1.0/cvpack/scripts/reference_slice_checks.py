"""Focused failures for approved97 return slice and genuine entry progression."""
import copy,json,re
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from symbol_references import compare as compare_references,parse as parse_references
from symbol_output import compare as compare_symbols
from progression import compare as compare_progression

def check(completion_original,completion_native,progression_original,progression_native):
    config,pe=load();completion_original,completion_native,progression_original,progression_native=map(Path,(completion_original,completion_native,progression_original,progression_native))
    events=(completion_original/'events').read_text();state=json.loads((completion_native/'gdb.json').read_text())
    from state import BINARY
    old=original_walks(events,json.loads((BINARY/'walks.json').read_text()));new=state['walks'];native=json.loads((completion_native/'symbol-references.json').read_text());original=parse_references(events);blocks=json.loads((completion_native/'symbol-blocks.json').read_text());symbols=compare_symbols(config,pe,events,json.loads((completion_native/'symbol-output.json').read_text()),old,new,True,state)
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError,StopIteration):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def run(change):
        a,b=copy.deepcopy((original,native));change(a,b)
        with patch('symbol_references.parse',return_value=a):return compare_references(config,pe,events,b,old,new,state,blocks,symbols)
    run(lambda a,b:None)
    reject('all reference invocations absent on both sides',lambda:run(lambda a,b:(a.clear(),b.clear())))
    reject('third actual reference omitted on both sides',lambda:run(lambda a,b:(a.pop(),b.pop())))
    reject('reference invocation order changed on both sides',lambda:run(lambda a,b:(a.reverse(),b.reverse())))
    for key,value,label in(('module',4,'module'),('offset',109,'source offset'),('kind',0x401,'kind'),('buckets',0,'bucket owner'),('record',1,'source record owner'),('hash',1,'hash output'),('name_offset',12,'name field'),('segment',2,'source segment'),('address',0,'source address'),('head_before',1,'old buckethead'),('published',0,'new buckethead'),('available',6600,'slab available'),('cursor',0,'slab cursor'),('base',1,'contained node address')):
        reject('both reference '+label+' wrong',lambda key=key,value=value:run(lambda a,b:[row[0].update({key:value})for row in(a,b)]))
    reject('reference returns from stale native caller PC',lambda:run(lambda a,b:b[0].update(return_pc=b[0]['caller_pc']+1)))
    reject('reference both saved and observed PC forged together',lambda:run(lambda a,b:b[0].update(caller_pc=1,return_pc=1)))
    reject('reference saved return CALL targets another function',lambda:run(lambda a,b:b[0].update(constructor_address=b[0]['constructor_address']+1)))
    reject('reference both caller source lines wrong',lambda:run(lambda a,b:b[0].update(caller_line=1,return_line=1)))
    reject('reference returns from stale native caller frame',lambda:run(lambda a,b:b[0].update(return_fp=b[0]['caller_fp']+4)))
    reject('reference returns from unknown native thread',lambda:run(lambda a,b:b[0].update(thread=0)))
    reject('reference original return ABI slot wrong',lambda:run(lambda a,b:a[0].update(return_esp=a[0]['return_esp']+4)))
    reject('reference post-return slab state changed',lambda:run(lambda a,b:a[0].update(return_cursor=a[0]['cursor']+4)))
    def byte(row,key,at,value):
        data=bytearray.fromhex(row[key]);data[at]=value;row[key]=data.hex()
    for at,label in((6,'headerreserved'),(32,'payloadpadding'),(34,'allocationtail')):
        reject('reference OWN '+label+' mutated after reservation',lambda at=at:run(lambda a,b:byte(b[0],'after',at,0x7d)))
    reject('reference reserved before/after packet disagrees with actual slab',lambda:run(lambda a,b:(byte(b[0],'before',32,0x7d),byte(b[0],'after',32,0x7d))))
    reject('reference full source bytes absent on both sides',lambda:run(lambda a,b:[row[0].update(source='')for row in(a,b)]))
    reject('reference bytes changed after observed original return',lambda:run(lambda a,b:a[0].update(frontier_bytes='')))
    pevents=(progression_original/'events').read_text();pstate=json.loads((progression_native/'gdb.json').read_text());trace=(progression_native/'trace').read_text().splitlines();compare_progression(config,pe,pevents,pstate,trace)
    def progress(change):
        s=copy.deepcopy(pstate);change(s);return compare_progression(config,pe,pevents,s,trace)
    for key,value,label in(('scope_depth',1,'scope'),('suppressed_depth',1,'suppression'),('signal',0,'real signal'),('module',9,'actual module'),('size',44,'input extent'),('capacity',4747,'allocation extent'),('cursor',0,'source owner/cursor'),('output_cursor',0,'output context')):
        reject('genuine next stub '+label+' changed',lambda key=key,value=value:progress(lambda s:s.update({key:value})))
    reject('progression falsely claims later heap equality',lambda:progress(lambda s:s.update(later_heap_equality=True)))
    reject('progression missing constructor entry trace',lambda:compare_progression(config,pe,pevents,pstate,[r for r in trace if r!='create_symbol_reference']))
    reject('progression wrong actual masked entry',lambda:compare_progression(config,pe,pevents.replace('exception 80000003 va 00025450 ','exception 80000003 va 00025480 '),pstate,trace))
    reject('progression source record disagrees with frozen DLL',lambda:progress(lambda s:s.update(bytes='00'+s['bytes'][2:])))
