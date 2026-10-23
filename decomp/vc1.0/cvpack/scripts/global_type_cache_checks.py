"""Case-specific ownership and constructor controls for first global cache."""
import copy,json
from pathlib import Path
from state import BINARY,load
from objects import original_walks
from buffer_storage import compare as compare_buffer
from global_type_cache import compare,compiled
from type_cache import compare as compare_primary

def changed(raw,offset,value,size=4):
    data=bytearray.fromhex(raw);data[offset:offset+size]=value.to_bytes(size,'little');return data.hex()

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text())
    old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks']
    context=compare_buffer(config,pe,events,json.loads((directory/'native-buffer-storage.json').read_text()),old,new,state,'hello-debug-dll')
    def run(a,b,c):
        x,y=a['type_work_b'][0],b['type_work_b'][0]
        return compare(bytearray.fromhex(x['bytes']),bytearray.fromhex(y['bytes']),x,y,a,b,c)
    proof=run(old,new,context);assert len(proof)==1 and proof[0]['input_type']==4192 and proof[0]['index']==0x1007
    def reject(label,change):
        a,b,c=copy.deepcopy((old,new,context));change(a,b,c)
        try:run(a,b,c)
        except(AssertionError,KeyError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def node(walks):return next(iter(walks['type_work_b'][0]['objects'].values()))
    def both_missing(a,b,c):
        for w in(a,b):w['type_work_b'][0]['objects']={}
    reject('both global named-cache node inventories missing',both_missing)
    reject('extra global named-cache allocation',lambda a,b,c:b['type_work_b'][0]['objects'].__setitem__('unknown',copy.deepcopy(node(b))))
    reject('global named-cache uninitialized flag credit',lambda a,b,c:node(b).__setitem__('bytes',changed(node(b)['bytes'],10,1,2)))
    reject('global named-cache wrong index',lambda a,b,c:node(b).__setitem__('bytes',changed(node(b)['bytes'],8,0x1006,2)))
    reject('global named-cache wrong record pointer',lambda a,b,c:node(b).__setitem__('bytes',changed(node(b)['bytes'],4,int.from_bytes(bytes.fromhex(node(b)['bytes'])[4:8],'little')+4)))
    reject('global named-cache name points inside wrong field',lambda a,b,c:node(b).__setitem__('bytes',changed(node(b)['bytes'],0,int.from_bytes(bytes.fromhex(node(b)['bytes'])[:4],'little')+1)))
    def wrong_bucket(a,b,c):
        root=b['type_work_b'][0];raw=bytearray.fromhex(root['bytes']);nonzero=[i for i in range(512)if int.from_bytes(raw[i*4:i*4+4],'little')];assert len(nonzero)==1
        i=nonzero[0];raw[(i+1)*4:(i+2)*4]=raw[i*4:i*4+4];raw[i*4:i*4+4]=b'\0'*4;root['bytes']=raw.hex()
    reject('global named-cache wrong hash bucket',wrong_bucket)
    def alias(a,b,c):
        root=b['type_work_b'][0];address=next(iter(b['type_work_a'][0]['objects'].values()))['address'];raw=bytearray.fromhex(root['bytes']);i=next(i for i in range(512)if int.from_bytes(raw[i*4:i*4+4],'little'));raw[i*4:i*4+4]=address.to_bytes(4,'little');root['bytes']=raw.hex();node(b)['address']=address
    reject('global named-cache allocation aliases primary node',alias)
    def primary(a,b,c):
        x,y=a['type_work_a'][0],b['type_work_a'][0]
        return compare_primary(bytearray.fromhex(x['bytes']),bytearray.fromhex(y['bytes']),x,y,a,b,c)
    primary(old,new,context)
    try:primary(old,new,None)
    except AssertionError:print('rejected: moved primary cache type without packed-copy proof')
    else:raise AssertionError('accepted missing primary copy proof')
    a,b,c=copy.deepcopy((old,new,context));r=b['local_type_records'][0];at=(4192-0x1000)*28+4;r['bytes']=changed(r['bytes'],at,0x1006,2)
    try:primary(a,b,c)
    except AssertionError:print('rejected: moved primary cache type has wrong packed index')
    else:raise AssertionError('accepted wrong primary packed index')
    a,b,c=copy.deepcopy((old,new,context));objects=b['type_work_a'][0]['objects'];r=next(r for r in objects.values()if int.from_bytes(bytes.fromhex(r['bytes'])[8:10],'little')==4192);raw=bytes.fromhex(r['bytes']);r['bytes']=changed(r['bytes'],4,int.from_bytes(raw[4:8],'little')+2)
    try:primary(a,b,c)
    except AssertionError:print('rejected: primary cache wrong original provider record')
    else:raise AssertionError('accepted wrong provider record')
    from audit import publics
    from binary import PE
    image=PE(BINARY/'build/CVPACK.EXE');linked=publics();p=compiled(image,linked)
    for label,address,value in [('global named-cache calloc size changed',p['call']-1,32)]:
        modified=copy.copy(image);modified.data=bytearray(image.data);modified.data[modified.offset(address)]=value
        try:compiled(modified,linked)
        except AssertionError:print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    root=linked['_type_work_b'].to_bytes(4,'little');start=linked['@save_type@4'];end=min(v for v in linked.values()if v>start);raw=image.read(start,end-start)
    pattern=b'\x8b\x0d'+root+b'\x8b\x04\x81';at=raw.index(pattern, p['call']-start)
    modified=copy.copy(image);modified.data=bytearray(image.data);position=modified.offset(start+at+2);modified.data[position:position+4]=linked['_type_work_a'].to_bytes(4,'little')
    try:compiled(modified,linked)
    except AssertionError:print('rejected: global named-cache publication reads wrong root')
    else:raise AssertionError('accepted wrong global cache root')
    from unittest.mock import patch
    from buffer_storage import parse
    old_rows=parse(events);native_rows=json.loads((directory/'native-buffer-storage.json').read_text())
    for label,change in[
        ('both sides omit new RECT copy inventory',lambda a,b:(a.__setitem__('entries',a['entries'][:6]),a.__setitem__('returns',a['returns'][:6]),b.__setitem__('entries',b['entries'][:6]),b.__setitem__('returns',b['returns'][:6]))),
        ('RECT field-list copy has wrong extent',lambda a,b:b['entries'][6].__setitem__('size',60)),
        ('RECT class copy has wrong parent owner',lambda a,b:b['entries'][7].__setitem__('owner',b['entries'][7]['owner']-28))]:
        a,b=copy.deepcopy((old_rows,native_rows));change(a,b)
        try:
            with patch('buffer_storage.parse',return_value=a):compare_buffer(config,pe,events,b,old,new,state,'hello-debug-dll')
        except AssertionError:print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    from type_pages import compare as compare_pages
    rows=json.loads((directory/'native-type-pages.json').read_text());rows[0]['postbytes']=changed(rows[0]['postbytes'],28,int.from_bytes(bytes.fromhex(rows[0]['postbytes'])[28:32],'little')+4)
    try:compare_pages(config,pe,events,rows,dict(old,**context['original_namespace']),dict(new,**context['native_namespace']),state,'hello-debug-dll',context)
    except AssertionError:print('rejected: eighth type page slot not the observed RECT return')
    else:raise AssertionError('accepted wrong eighth type slot')
    import tempfile,hashlib
    from global_type_cache import validate
    with tempfile.TemporaryDirectory(prefix='global-cache-proof-',dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as folder:
        path=Path(folder)/'proof.json';p=json.loads((BINARY/config['buffer_storage_observations']['evidence']).read_text());p.pop('global_cache');path.write_text(json.dumps(p));current=copy.deepcopy(config);current['buffer_storage_observations']=dict(evidence=str(path),evidence_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        try:validate(current,pe)
        except AssertionError:print('rejected: typed global cache without constructor provenance')
        else:raise AssertionError('accepted missing global cache provenance')
