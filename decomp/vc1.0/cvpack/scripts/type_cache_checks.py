"""Focused rejection of wrong cache ownership, topology and defined bytes."""
import copy,json,struct
from pathlib import Path
from objects import original_walks
from type_cache import compare
from state import BINARY

def check(pair):
    pair=Path(pair);spec=json.loads((pair/'walks.json'if (pair/'walks.json').exists()else BINARY/'walks.json').read_text())
    original=original_walks((pair/'original-events').read_text(),spec)
    native=json.loads((pair/'native-gdb.json').read_text())['walks']
    def run(a,b):return compare(bytearray.fromhex(a['type_work_a'][0]['bytes']),bytearray.fromhex(b['type_work_a'][0]['bytes']),a['type_work_a'][0],b['type_work_a'][0],a,b)
    result=run(original,native);assert len(result)==17,'wrong golden cache inventory'
    def deny(label,action):
        try:action()
        except (AssertionError,KeyError):print('rejected:',label);return
        raise AssertionError('accepted invalid cache '+label)
    key=next(iter(native['type_work_a'][0]['objects']))
    def mutate(what,value,offset,size=4):
        a,b=copy.deepcopy(original),copy.deepcopy(native)
        for x in(a,b):
            node=x['type_work_a'][0]['objects'][key];raw=bytearray.fromhex(node['bytes'])
            raw[offset:offset+size]=value.to_bytes(size,'little');node['bytes']=raw.hex()
        deny(what,lambda:run(a,b))
    mutate('type cache unknown name owner',1,0)
    mutate('type cache unknown type owner',1,4)
    mutate('type cache mismatched Pascal name/index',0x1000,8,2)
    mutate('type cache initialized padding changed on both sides',1,10,2)
    mutate('type cache unknown next target',1,12)
    for label,change in [('both type-cache nodes omitted',lambda x:x['type_work_a'][0].pop('objects')),
             ('both one cache node omitted',lambda x:x['type_work_a'][0]['objects'].pop(key)),
             ('both extra cache objects',lambda x:x['type_work_a'][0]['objects'].__setitem__('cache-511-999',dict(address=1,bytes='00'*16)))]:
        a,b=copy.deepcopy(original),copy.deepcopy(native)
        for x in(a,b):change(x)
        deny(label,lambda:run(a,b))
    a,b=copy.deepcopy(original),copy.deepcopy(native)
    for x in(a,b):
        node=x['type_work_a'][0]['objects'][key];raw=bytearray.fromhex(node['bytes']);raw[12:16]=node['address'].to_bytes(4,'little');node['bytes']=raw.hex()
    deny('both cache cycles',lambda:run(a,b))
    a,b=copy.deepcopy(original),copy.deepcopy(native)
    for x in(a,b):
        root=x['type_work_a'][0];raw=bytearray.fromhex(root['bytes']);bucket=int(key.split('-')[1]);other=(bucket+1)%512;raw[other*4:other*4+4]=raw[bucket*4:bucket*4+4];root['bytes']=raw.hex()
    deny('both cache duplicate bucket ownership',lambda:run(a,b))
    a,b=copy.deepcopy(original),copy.deepcopy(native)
    node=b['type_work_a'][0]['objects'][key];raw=bytearray.fromhex(node['bytes']);raw[8]^=1;node['bytes']=raw.hex()
    deny('native cache index differs',lambda:run(a,b))


def compiled_checks():
    from audit import publics
    from binary import PE
    from type_cache import compiled_provenance,validate
    from state import ROOT
    symbols=publics();pe=PE(BINARY/'build/CVPACK.EXE');proof=compiled_provenance(pe,symbols)
    def deny(label,fn):
        try:fn()
        except AssertionError:print('rejected:',label);return
        raise AssertionError('accepted invalid '+label)
    def changed(address,offset,value):
        result=copy.copy(pe);data=bytearray(pe.data);data[pe.offset(address)+offset]=value;result.data=bytes(data);return result
    deny('cache compiled allocation wrong size',lambda:compiled_provenance(changed(proof['call']-2,1,15),symbols))
    deny('cache compiled wrong allocation owner',lambda:compiled_provenance(changed(proof['call'],10,(proof['result_frame_offset']&255)^4),symbols))
    deny('cache compiled wrong publication source',lambda:compiled_provenance(changed(proof['publication_start'],2,(proof['result_frame_offset']&255)^4),symbols))
    helper=proof['zero_helper'];end=min(v for v in symbols.values()if v>helper);body=pe.read(helper,end-helper);offset=body.index(b'\xc6\x04\x08\x00')+3
    deny('cache compiled helper fails zero initialization',lambda:compiled_provenance(changed(helper,offset,1),symbols))
    deny('cache zero loop wrong requested-size slot',lambda:compiled_provenance(changed(proof['zero_bound'],0,12),symbols))
    deny('cache zero loop wrong unsigned exit condition',lambda:compiled_provenance(changed(proof['zero_branch'],0,0x87),symbols))
    config=json.loads((BINARY/'functions.json').read_text());original=PE(ROOT/config['target']['path'])
    bad=copy.deepcopy(config);bad.pop('type_cache_observations')
    deny('cache allocation/publication proof omitted',lambda:validate(bad,original,pe,symbols))
    bad=copy.deepcopy(config);bad['type_cache_observations']['evidence_sha256']='0'*64
    deny('cache stale source/allocation proof',lambda:validate(bad,original,pe,symbols))
