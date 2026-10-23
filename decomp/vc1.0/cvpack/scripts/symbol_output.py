"""Concrete module3 packed-symbol malloc lifetime and initialized header."""
import hashlib,json,re
from state import BINARY
from objects import namespace

def validate(config,pe):
    ref=config.get('symbol_output_observations')
    required=any(r['name']=='pack_new_types'and r['status']!='stub'for r in config['functions'])
    assert ref or not required,'missing packed symbol allocation proof'
    if not ref:return None
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale packed symbol allocation proof'
    p=json.loads(path.read_text())
    assert p['size']==5113 and p['input_size']==4746 and p['defined']==4 and p['frontier']==0x1bee0
    for region in p['original_regions']:
        assert pe.read(region['start'],region['end']-region['start']).hex()==region['bytes'],'packed symbol instruction proof differs'
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'packed symbol source changed'
    lines=(BINARY/'src/types.c').read_text().splitlines();line=p['native_allocation_line']
    assert lines[line-2].strip()=='buffer = malloc(output_size);'and lines[line-1].strip()=='if (!buffer) fatal_error(14, NULL, NULL);','packed symbol native allocation line differs'
    return p

def parse(events):
    allocations=[];outputs=[]
    for line in events.splitlines():
        if line.startswith('symbol-allocation '):
            words=line.split()[1:];assert words[::2]==['base','size','input','entry','module','number','padding','skipped','esp','before'],'invalid symbol allocation event'
            r={k:(v if k=='before'else int(v,16 if k in('base','input','entry','module','esp')else 10))for k,v in zip(words[::2],words[1::2])};allocations.append(r)
        elif line.startswith('symbol-output '):
            words=line.split()[1:];assert words[::2]==['base','size','cursor','frontier','bytes'],'invalid symbol output event'
            outputs.append({k:(v if k=='bytes'else int(v,10 if k=='size'else 16))for k,v in zip(words[::2],words[1::2])})
    return allocations,outputs

def compare(config,pe,events,native,old_walks,new_walks,required):
    proof=validate(config,pe);old,outputs=parse(events)
    if not(old or outputs or native):
        assert not required,'missing mandatory packed symbol allocation observations'
        return None
    assert proof and len(old)==len(outputs)==len(native)==1,'missing/extra packed symbol allocation observations'
    assert events.splitlines().count('symbol-allocation-rearm 0002504e 00025052')==1,'missing allocation return-site rearm'
    a,b=old[0],native[0];post=outputs[0];assert not b.get('error'),'native symbol allocation observer failed'
    assert post['base']==a['base']and post['size']==a['size']==b['size']==5113
    assert post['frontier']==0x1bee0 and b['frontier']=='write_search_symbol'and b['parent']=='pack_new_types'
    words={int(m[1]):int(m[2],16)for m in re.finditer(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
    esp=int(re.search(r'^registers .* esp ([0-9a-f]+) ',events,re.M)[1],16)
    assert a['esp']==esp+8,'packed symbol allocation belongs to another parent frame'
    assert words[0]==0x250d2 and words[1]==post['cursor']==a['base']+4 and words[2]&0xffff==1,'wrong packed symbol output caller/argument'
    assert words[7]==words[8]==a['base']and words[10]==a['input']and words[11]==a['entry']and words[12]==a['module'],'wrong packed symbol parent saved ownership'
    for r,walks,after,cursor in ((a,old_walks,post['bytes'],post['cursor']),(b,new_walks,b['postbytes'],b['cursor'])):
        assert r['number']==3 and r['padding']==367 and r['skipped']==0 and r['size']==4746+r['padding']-r['skipped']
        assert r['base']and cursor==r['base']+4
        assert len(walks['type_data'])==len(walks['directory_entries'])==1
        source=walks['type_data'][0];assert r['input']==source['address']and len(bytes.fromhex(source['bytes']))==4746
        entries=walks['directory_entries'][0];assert r['entry']==entries['address']+7*12
        entry=bytes.fromhex(entries['bytes'])[84:96]
        assert entry[:4]==b'\x24\x01\x03\x00'and int.from_bytes(entry[8:12],'little')==4746
        modules=[m for m in walks['module_list']if m['address']==r['module']]
        assert len(modules)==1 and bytes.fromhex(modules[0]['bytes'])[:2]==b'\x03\x00','packed output module owner differs'
        for address,size,identity in namespace(walks):
            assert not(r['base']<address+size and address<r['base']+r['size']),'packed symbol allocation overlaps another owner'
        before,after=bytes.fromhex(r['before']),bytes.fromhex(after)
        assert len(before)==len(after)==r['size'],'packed symbol observed extent differs'
        assert after[4:]==before[4:],'packed symbol output changed undefined tail'
        assert after[:4]==bytes.fromhex(source['bytes'])[:4]==b'\x01\0\0\0','defined packed symbol header differs'
    return dict(original=a,native=b,original_output=post,defined=4,logical_owner='pack_new_types/module3/malloc25049',original_namespace=dict(address=a['base'],bytes=post['bytes']),native_namespace=dict(address=b['base'],bytes=b['postbytes']))
