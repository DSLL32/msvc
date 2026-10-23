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
    known={'write_search_symbol':(0x1bee0,4,4,0x250d2,8,7,10),
           'convert_object_name':(0x255c0,16,4,0x25172,0,5,8),
           'convert_udt_symbol':(0x25800,40,28,0x25172,0,5,8),
           'intern_global':(0x1b500,40,28,0x2584c,8,7,10),
           'compact_new_type':(0x26520,40,202,0x26509,16,9,12),
           'compact_pointer':(0x27100,40,202,0x26a7a,52,18,21)}
    assert p.get('frontiers'),'missing packed symbol frontier contracts'
    for name,site in p['frontiers'].items():
        assert name in known and tuple(site[k]for k in('original','defined','input_offset','return_address','allocation_esp_delta','buffer_slot','input_slot'))==known[name],'unreviewed packed symbol initialized range/frame'
        if name!='write_search_symbol':
            assert (site['kind'],site['record_length'],site['callback_index'])==((9,24,31)if name=='convert_object_name'else((4,28,27)if name in('compact_new_type','compact_pointer')else(4,34,27))),'unreviewed packed symbol callback selection'
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

def compare(config,pe,events,native,old_walks,new_walks,required,native_state):
    proof=validate(config,pe);old,outputs=parse(events)
    if not(old or outputs or native):
        assert not required,'missing mandatory packed symbol allocation observations'
        return None
    assert proof and len(old)==len(outputs)==len(native)==1,'missing/extra packed symbol allocation observations'
    assert events.splitlines().count('symbol-allocation-rearm 0002504e 00025052')==1,'missing allocation return-site rearm'
    a,b=old[0],native[0];post=outputs[0];assert not b.get('error'),'native symbol allocation observer failed'
    assert post['base']==a['base']and post['size']==a['size']==b['size']==5113
    matches=[(name,r)for name,r in proof['frontiers'].items()if r['original']==post['frontier']]
    assert len(matches)==1,'unreviewed packed symbol write frontier'
    name,site=matches[0];defined=site['defined']
    assert b['frontier']==name and b['parent']=='pack_new_types'
    words={int(m[1]):int(m[2],16)for m in re.finditer(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)}
    esp=int(re.search(r'^registers .* esp ([0-9a-f]+) ',events,re.M)[1],16)
    assert a['esp']==esp+site['allocation_esp_delta'],'packed symbol allocation belongs to another parent frame'
    assert words[0]==site['return_address']and post['cursor']==a['base']+defined,'wrong packed symbol output caller/cursor'
    if 'argument_slot'in site:assert words[site['argument_slot']]==post['cursor']and words[2]&0xffff==1,'wrong packed symbol output argument'
    slot=site['buffer_slot'];source_slot=site['input_slot']
    assert words[slot]==words[slot+1]==a['base']and words[source_slot]==a['input']and words[source_slot+1]==a['entry']and words[source_slot+2]==a['module'],'wrong packed symbol parent saved ownership'
    memory={}
    for match in re.finditer(r'^memory ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):
        address=int(match[1],16);memory.update((address+i,v)for i,v in enumerate(bytes.fromhex(match[2])))
    old_input=int.from_bytes(bytes(memory[0x335e4+i]for i in range(4)),'little')
    old_cursor=int.from_bytes(bytes(memory[0x335f0+i]for i in range(4)),'little')
    assert old_cursor==post['cursor']and native_state['globals']['symbol_output']['address']==b['cursor'],'packed symbol observed cursor differs from actual global'
    assert old_input==a['input']+site['input_offset']and native_state['globals']['symbol_input']['address']==b['input']+site['input_offset'],'wrong actual packed symbol source cursor'
    if name in('convert_object_name','convert_udt_symbol','intern_global','compact_new_type','compact_pointer'):
        registers=re.search(r'^registers (.*)$',events,re.M)[1].split();registers={k:int(v,16)for k,v in zip(registers[::2],registers[1::2])}
        assert (words[1]if name=='compact_pointer'else registers['ebp'])&0xffff==site['record_length'],'wrong original callback record length'
        if name=='intern_global':
            assert site['nested_return_slot']==2 and site['nested_return_address']==0x25172 and words[2]==0x25172,'wrong original nested symbol frame'
            assert registers['edx']==0x1afe0 and registers['ecx']==old_input,'wrong original nested symbol callback arguments'
            assert b['nested_caller']=='convert_udt_symbol','wrong native nested symbol frame'
        elif name=='compact_pointer':
            assert site['nested_returns']=={'9':0x26509,'11':0x25830,'13':0x25172}and all(words[int(slot)]==value for slot,value in site['nested_returns'].items()),'wrong original pointer compact nested frames'
            assert site['type_index']==4134 and registers['ebp']&0xffff==4134,'wrong actual active pointer type'
            assert b['nested_callers']==['compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4134,'wrong native pointer compact nested frames/type'
            assert len(old_walks['local_type_records'])==len(new_walks['local_type_records'])==1
            offset=(4134-0x1000)*28
            assert registers['ecx']==old_walks['local_type_records'][0]['address']+offset and b['type_record']==new_walks['local_type_records'][0]['address']+offset,'wrong actual pointer type owner'
        elif name=='compact_new_type':
            assert site['nested_returns']=={'2':0x25830,'4':0x25172}and all(words[int(slot)]==value for slot,value in site['nested_returns'].items()),'wrong original compact nested frames'
            assert site['type_index']==4134 and registers['ecx']&0xffff==4134,'wrong original compact type argument'
            assert b['nested_callers']==['convert_new_type','convert_udt_symbol']and b['type_index']==4134,'wrong native compact nested frames/type'
        else:assert registers['edx']&0xffff==site['kind']and registers['ecx']==0x38110+site['callback_index']*8 and registers['eax']==site['original'],'wrong original callback context'
        assert (b['record_kind'],b['record_length'],b['callback_index'])==(site['kind'],site['record_length'],site['callback_index']),'wrong native callback context'
    for r,walks,after,cursor in ((a,old_walks,post['bytes'],post['cursor']),(b,new_walks,b['postbytes'],b['cursor'])):
        assert r['number']==3 and r['padding']==367 and r['skipped']==0 and r['size']==4746+r['padding']-r['skipped']
        assert r['base']and cursor==r['base']+defined
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
        assert after[defined:]==before[defined:],'packed symbol output changed undefined tail'
        assert after[:4]==bytes.fromhex(source['bytes'])[:4]==b'\x01\0\0\0','defined packed symbol header differs'
        if defined>=16:
            assert after[4:16]==bytes.fromhex('0a0005000000000001000000'),'packed symbol initialized search record differs'
        if defined==40:assert after[16:40]==bytes.fromhex(source['bytes'])[4:28],'packed symbol actual object-name copy differs'
    assert bytes.fromhex(post['bytes'])[:defined]==bytes.fromhex(b['postbytes'])[:defined],'packed symbol defined prefix differs'
    return dict(original=a,native=b,original_output=post,defined=defined,logical_owner='pack_new_types/module3/malloc25049',original_namespace=dict(address=a['base'],bytes=post['bytes']),native_namespace=dict(address=b['base'],bytes=b['postbytes']))
