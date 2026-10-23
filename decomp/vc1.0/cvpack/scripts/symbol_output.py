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
           'compact_pointer':(0x27100,40,202,0x26a7a,52,18,21),
           'compact_type_list':(0x272a0,40,202,0x26cfa,112,33,36),
           'store_type':(0x19580,40,202,0x27ab1,160,45,48),
           'hash_type':(0x1c1a0,40,202,0x19590,184,51,54),
           'save_type':(0x19860,40,202,0x196fa,184,51,54),
           'append_buffer':(0x22f40,40,202,0x19a65,224,61,64),
           'finish_recursive_types':(0x262d0,40,202,0x26511,16,9,12),
           'find_named_type':(0x1ad40,40,384,0x26686,56,19,22),
           'find_global_type':(0x1af10,40,384,0x1ae37,84,26,29),
           'find_local_type':(0x1ae40,40,384,0x1ae0e,172,48,51),
           'record_derived_types':(0x19770,40,384,0x19759,168,47,50),
           'convert_global_data32':(0x256f0,40,966,0x25172,0,5,8),
           'get_type_record':(0x1e700,40,1321,0x265c5,144,41,44),
           'add_recursive_offset':(0x27b10,40,2227,0x2792f,100,30,33),
           'intern_type':(0x1ac80,40,2227,0x26f9d,60,20,23),
           'hash_recursive_type':(0x1c640,40,2227,0x1ac8e,76,24,27),
           'commit_recursive_type':(0x1eac0,40,2227,0x1ad37,76,24,27),
           'publish_recursive_type':(0x19ca0,40,2227,0x1ec0e,116,34,37),
           'hash_referenced_type':(0x1c880,40,2227,0x1c7a0,192,53,56),
           'publish_packed_type':(0x19c20,40,2227,0x1ed24,196,54,57)}
    assert p.get('frontiers'),'missing packed symbol frontier contracts'
    for name,site in p['frontiers'].items():
        assert name in known and tuple(site[k]for k in('original','defined','input_offset','return_address','allocation_esp_delta','buffer_slot','input_slot'))==known[name],'unreviewed packed symbol initialized range/frame'
        if name!='write_search_symbol':
            assert (site['kind'],site['record_length'],site['callback_index'])==((4,24,27)if name in('add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type')else(4,29,27)if name=='get_type_record'else(0x202,43,12)if name=='convert_global_data32'else(4,21,27)if name in('find_named_type','find_global_type','find_local_type','record_derived_types')else((9,24,31)if name=='convert_object_name'else((4,28,27)if name in('compact_new_type','compact_pointer','compact_type_list','store_type','hash_type','save_type','append_buffer','finish_recursive_types')else(4,34,27)))),'unreviewed packed symbol callback selection'
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
    if name in('convert_object_name','convert_udt_symbol','intern_global','compact_new_type','compact_pointer','compact_type_list','store_type','hash_type','save_type','append_buffer','finish_recursive_types','find_named_type','find_global_type','find_local_type','record_derived_types','convert_global_data32','get_type_record','add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type'):
        registers=re.search(r'^registers (.*)$',events,re.M)[1].split();registers={k:int(v,16)for k,v in zip(registers[::2],registers[1::2])}
        length_slot={'compact_pointer':1,'compact_type_list':16,'store_type':28,'hash_type':34,'save_type':34,'append_buffer':44,'find_named_type':2,'find_global_type':9,'find_local_type':31,'record_derived_types':30,'get_type_record':24,'add_recursive_offset':13,'intern_type':3,'hash_recursive_type':7,'commit_recursive_type':7,'publish_recursive_type':17,'hash_referenced_type':36,'publish_packed_type':37}.get(name)
        length=words[length_slot]if length_slot is not None else registers['ebp']
        assert length&0xffff==site['record_length'],'wrong original callback record length'
        if name=='intern_global':
            assert site['nested_return_slot']==2 and site['nested_return_address']==0x25172 and words[2]==0x25172,'wrong original nested symbol frame'
            assert registers['edx']==0x1afe0 and registers['ecx']==old_input,'wrong original nested symbol callback arguments'
            assert b['nested_caller']=='convert_udt_symbol','wrong native nested symbol frame'
        elif name=='publish_packed_type':
            expected={'10':0x1ecaf,'20':0x1ecaf,'30':0x1ad37,'34':0x26f9d,'45':0x26509,'47':0x25830,'49':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong packed-type nested frames'
            assert b['nested_callers']==['commit_recursive_type','commit_recursive_type','commit_recursive_type','intern_type','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4199,'wrong packed-type owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4199-0x1000)*28
            assert registers['esi']==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong packed-type current parent owner'
            a_record=bytes.fromhex(old_table['bytes'])[offset:offset+28];b_record=bytes.fromhex(new_table['bytes'])[offset:offset+28]
            assert registers['ecx']==registers['edi']==int.from_bytes(a_record[:4],'little')and b['record']==int.from_bytes(b_record[:4],'little'),'wrong packed-type ECX copied pointer'
            assert registers['edx']&0xffff==int.from_bytes(a_record[4:6],'little')==int.from_bytes(b_record[4:6],'little')==b['global_type']==0x1049,'wrong packed-type lowDX index'
            assert words[3]==words[9]==old_table['address']+(4208-0x1000)*28,'wrong packed-type middle recursive owner'
            assert words[13]==words[16]==words[19]==words[22]==words[25]==words[26]==words[32]==words[35]==old_table['address']+(4209-0x1000)*28 and b['root_type_record']==new_table['address']+(4209-0x1000)*28,'wrong packed-type outer root ownership'
            assert b['fixed_field']==0x1047 and a_record[20]==b_record[20]==0x80,'wrong packed-pointer resolved field/recursive mark'
        elif name=='hash_referenced_type':
            expected={'7':0x19ccf,'9':0x1ec0e,'19':0x1ecaf,'29':0x1ad37,'33':0x26f9d,'44':0x26509,'46':0x25830,'48':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong referenced-hash nested frames'
            assert b['nested_callers']==['hash_recursive_type','publish_recursive_type','commit_recursive_type','commit_recursive_type','intern_type','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4208,'wrong referenced-hash owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4208-0x1000)*28
            assert registers['ecx']==registers['esi']==words[3]==words[8]==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong referenced-hash ECX owner'
            assert registers['edx']&0xffff==0 and registers['edi']&0xffff==registers['ebp']&0xffff==b['partial_hash'],'wrong referenced-hash ordinal/partial hash'
            a_record=bytes.fromhex(old_table['bytes'])[offset:offset+28];b_record=bytes.fromhex(new_table['bytes'])[offset:offset+28]
            assert int.from_bytes(a_record[10:12],'little')==int.from_bytes(b_record[10:12],'little')==0xfff,'referenced-hash cache not actually pending'
            assert words[12]==words[15]==words[18]==words[21]==words[24]==words[25]==words[31]==words[34]==old_table['address']+(4209-0x1000)*28 and b['root_type_record']==new_table['address']+(4209-0x1000)*28,'wrong referenced-hash outer root ownership'
        elif name=='publish_recursive_type':
            expected={'10':0x1ad37,'14':0x26f9d,'25':0x26509,'27':0x25830,'29':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong recursive-publication nested frames'
            assert b['nested_callers']==['commit_recursive_type','intern_type','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4209,'wrong recursive-publication owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4209-0x1000)*28
            assert registers['ecx']==registers['esi']==words[2]==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong recursive-publication ECX owner'
            a_record=bytes.fromhex(old_table['bytes'])[offset:offset+28];b_record=bytes.fromhex(new_table['bytes'])[offset:offset+28]
            assert registers['edi']==int.from_bytes(a_record[:4],'little')and b['packed_original']==int.from_bytes(b_record[:4],'little'),'wrong recursive-publication packed original owner'
            assert words[8]==b['named_lookup_result']==0,'recursive-publication uninitialized named output would be consumed'
            assert int.from_bytes(a_record[4:6],'little')==int.from_bytes(b_record[4:6],'little')==0x1047,'wrong pending recursive global type index'
            assert int.from_bytes(a_record[10:12],'little')==int.from_bytes(b_record[10:12],'little')==b['hash_bucket'],'recursive-publication hash cache differs from actual parent bucket'
        elif name=='commit_recursive_type':
            expected={'4':0x26f9d,'15':0x26509,'17':0x25830,'19':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong recursive-commit nested frames'
            assert b['nested_callers']==['intern_type','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4209,'wrong recursive-commit owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4209-0x1000)*28
            assert registers['edi']==words[5]==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong recursive-commit parent record owner'
            assert registers['ecx']&0xffff==words[6]&0xffff==4209,'wrong actual recursive-commit type'
            old_hash=int.from_bytes(bytes.fromhex(old_table['bytes'])[offset+10:offset+12],'little');new_hash=int.from_bytes(bytes.fromhex(new_table['bytes'])[offset+10:offset+12],'little')
            assert registers['eax']==old_hash==new_hash==b['hash_bucket']and old_hash<4096,'recursive hash full return/cache/bucket differs'
            assert registers['ebx']==0 and int.from_bytes(bytes.fromhex(old_walks['name_hash_buckets'][0]['bytes'])[old_hash*4:old_hash*4+4],'little')==0,'recursive commit did not take actual empty bucket path'
        elif name=='hash_recursive_type':
            expected={'4':0x26f9d,'15':0x26509,'17':0x25830,'19':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong recursive-hash nested frames'
            assert b['nested_callers']==['intern_type','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4209,'wrong recursive-hash owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4209-0x1000)*28
            assert registers['ecx']==registers['edi']==words[5]==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong recursive-hash ECX record owner'
            assert words[6]&0xffff==4209,'wrong recursive-hash parent type argument'
        elif name=='intern_type':
            expected={'11':0x26509,'13':0x25830,'15':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong recursive-intern nested frames'
            assert b['nested_callers']==['compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4209,'wrong recursive-intern owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4209-0x1000)*28
            assert words[1]==registers['esi']==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong recursive-intern typed parent owner'
            assert words[2]&0xffff==4209,'wrong actual recursive-intern type argument'
        elif name=='add_recursive_offset':
            expected={'12':0x26b1e,'21':0x26509,'23':0x25830,'25':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong recursive-offset nested frames'
            assert b['nested_callers']==['compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4208,'wrong recursive-offset owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4208-0x1000)*28
            assert registers['ecx']==registers['ebp']==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong recursive-offset typed list owner'
            assert registers['edx']&0xffff==b['recursive_offset']==54,'wrong actual recursive-offset argument'
        elif name=='get_type_record':
            expected={'11':0x277d8,'23':0x26b1e,'32':0x26509,'34':0x25830,'36':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong pending resolver nested frames'
            assert b['nested_callers']==['compact_new_type','compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4218,'wrong pending resolver owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4218-0x1000)*28
            assert registers['ebp']&0xffff==4218 and b['type_record']==new_table['address']+offset,'wrong pending resolver parent record'
            old_record=bytes.fromhex(old_table['bytes'])[offset:offset+28];new_record=bytes.fromhex(new_table['bytes'])[offset:offset+28]
            assert int.from_bytes(old_record[6:8],'little')==int.from_bytes(new_record[6:8],'little')==4222 and old_record[24]&0x40 and int.from_bytes(old_record[24:26],'little')&0x1000,'wrong actual resolved forward index/state'
            assert words[1]&0xffff==126,'wrong actual resolver relative input'
        elif name=='record_derived_types':
            expected={'8':0x2706c,'17':0x277d8,'29':0x26b1e,'38':0x26509,'40':0x25830,'42':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong derived type nested frames'
            assert b['nested_callers']==['store_type','compact_new_type','compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4192,'wrong derived type owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4192-0x1000)*28
            assert registers['ebx']==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong derived type parent record owner'
            assert words[1]&0xffff==0x1007 and words[2]&0xffff==0x1006,'wrong actual derived type/list indices'
        elif name=='find_local_type':
            expected={'8':0x26755,'18':0x277d8,'30':0x26b1e,'39':0x26509,'41':0x25830,'43':0x25172}
            assert site['nested_returns']==expected and all(words[int(k)]==v for k,v in expected.items()),'wrong local type nested frames'
            assert b['nested_callers']==['find_named_type','compact_new_type','compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4097 and b['kind']==5 and words[1]&0xffff==5 and words[4]==0,'wrong local type input/owner path'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4097-0x1000)*28
            old_data=int.from_bytes(bytes.fromhex(old_table['bytes'])[offset:offset+4],'little');new_data=int.from_bytes(bytes.fromhex(new_table['bytes'])[offset:offset+4],'little')
            assert registers['ebp']&0xffff==4097 and registers['edi']==old_data and b['type_record']==new_table['address']+offset,'wrong local type record owner'
            assert words[2]==registers['ebx']==old_data+16 and b['name']==new_data+16,'wrong local Pascal type name'
        elif name=='find_global_type':
            assert site['nested_returns']=={'7':0x26686,'17':0x26509,'19':0x25830,'21':0x25172}and all(words[int(k)]==v for k,v in site['nested_returns'].items()),'wrong global type nested frames'
            assert b['nested_callers']==['find_named_type','compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4185 and b['kind']==5 and words[1]&0xffff==5,'wrong global named type arguments'
            old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0];offset=(4185-0x1000)*28
            old_data=int.from_bytes(bytes.fromhex(old_table['bytes'])[offset:offset+4],'little');new_data=int.from_bytes(bytes.fromhex(new_table['bytes'])[offset:offset+4],'little')
            assert registers['ebp']&0xffff==4185 and registers['edi']==old_data and b['type_record']==new_table['address']+offset,'wrong nested named type record owner'
            assert words[2]==registers['ebx']==old_data+16 and b['name']==new_data+16,'wrong actual Pascal type name field'
        elif name=='find_named_type':
            assert site['nested_returns']=={'10':0x26509,'12':0x25830,'14':0x25172}and all(words[int(k)]==v for k,v in site['nested_returns'].items()),'wrong named type caller frames'
            assert b['nested_callers']==['compact_new_type','convert_new_type','convert_udt_symbol']and b['type_index']==4185,'wrong native named type owner frames'
            assert registers['ebp']&0xffff==4185 and registers['ecx']==registers['esi'],'wrong original named type owner register'
            assert b['type_record']==new_walks['local_type_records'][0]['address']+(4185-0x1000)*28 and registers['ecx']==old_walks['local_type_records'][0]['address']+(4185-0x1000)*28,'wrong named type allocation owner'
        elif name=='finish_recursive_types':
            assert site['nested_returns']=={'2':0x25830,'4':0x25172}and words[2]==0x25830 and words[4]==0x25172,'wrong completed compact caller frames'
            assert b['nested_callers']==['convert_new_type','convert_udt_symbol']and b['type_index']==4134 and b['converted']==0x1002,'wrong completed compact native caller'
            assert registers['esi']&0xffff==b['converted'],'wrong compact result at recursive finalization'
        elif name=='append_buffer':
            expected={'10':0x196fa,'16':0x27ab1,'28':0x26cfa,'37':0x2711d,'43':0x26a7a,'52':0x26509,'54':0x25830,'56':0x25172}
            assert site['nested_returns']==expected and all(words[int(slot)]==value for slot,value in expected.items()),'wrong original append buffer nested frames'
            assert (site['type_index'],site['list_count'])==(4115,1)and (b['type_index'],b['list_count'])==(4115,1),'wrong appended list identity/count'
            assert b['nested_callers']==['save_type','store_type','compact_type_list','compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol'],'wrong native append buffer frame chain'
            assert len(old_walks['local_type_records'])==len(new_walks['local_type_records'])==1
            offset=19*28;old_table,new_table=old_walks['local_type_records'][0],new_walks['local_type_records'][0]
            assert registers['ebx']==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong appended record owner'
            assert registers['ecx']==0x3301c and words[1]==8 and registers['edx']==int.from_bytes(bytes.fromhex(old_table['bytes'])[offset:offset+4],'little'),'wrong pending append copy arguments'
            old_page=int.from_bytes(bytes(memory[0x37198+i]for i in range(4)),'little')
            assert registers['esi']==old_page and b['page']==native_state['globals']['type_pages'][0]['address'],'wrong append caller type page owner'
        elif name in('hash_type','save_type'):
            assert site['nested_returns']=={'6':0x27ab1,'18':0x26cfa,'27':0x2711d,'33':0x26a7a,'42':0x26509,'44':0x25830,'46':0x25172}and all(words[int(slot)]==value for slot,value in site['nested_returns'].items()),'wrong original type hash nested frames'
            assert (site['type_index'],site['list_count'])==(4115,1)and (b['type_index'],b['list_count'])==(4115,1),'wrong hashed list identity/count'
            assert b['nested_callers']==['store_type','compact_type_list','compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol'],'wrong native type hash frame chain'
            assert len(old_walks['local_type_records'])==len(new_walks['local_type_records'])==1
            offset=(4115-0x1000)*28;old_table=old_walks['local_type_records'][0];new_table=new_walks['local_type_records'][0]
            old_data=int.from_bytes(bytes.fromhex(old_table['bytes'])[offset:offset+4],'little');new_data=int.from_bytes(bytes.fromhex(new_table['bytes'])[offset:offset+4],'little')
            assert registers['ebx']==old_table['address']+offset and b['type_record']==new_table['address']+offset,'wrong type hash typed record owner'
            if name=='hash_type':
                assert registers['ebp']==registers['ebx']and registers['ecx']==old_data and b['record']==new_data,'wrong hash input record ownership'
            else:
                assert registers['ecx']==registers['ebx'],'wrong stored type argument owner'
                bucket=((int(native_state['globals']['type_hash_high'],16)<<8)|int(native_state['globals']['type_hash_low'],16))&0xfff
                assert 0<=b['bucket']<4096 and b['bucket']==registers['ebp']&0xffff==bucket,'wrong actual hash result bucket'
        elif name=='store_type':
            assert site['nested_returns']=={'12':0x26cfa,'21':0x2711d,'27':0x26a7a,'36':0x26509,'38':0x25830,'40':0x25172}and all(words[int(slot)]==value for slot,value in site['nested_returns'].items()),'wrong original stored list nested frames'
            assert (site['type_index'],site['list_count'])==(4115,1)and (b['type_index'],b['list_count'])==(4115,1),'wrong stored list identity/count'
            assert b['nested_callers']==['compact_type_list','compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol'],'wrong native stored list frame chain'
            assert len(old_walks['local_type_records'])==len(new_walks['local_type_records'])==1
            offset=(4115-0x1000)*28
            assert registers['ebp']==registers['ecx']==old_walks['local_type_records'][0]['address']+offset and b['type_record']==new_walks['local_type_records'][0]['address']+offset,'wrong stored list typed record owner'
        elif name=='compact_type_list':
            assert site['nested_returns']=={'9':0x2711d,'15':0x26a7a,'24':0x26509,'26':0x25830,'28':0x25172}and all(words[int(slot)]==value for slot,value in site['nested_returns'].items()),'wrong original list compact nested frames'
            assert site['type_index']==4133 and registers['ebp']&0xffff==4133,'wrong active procedure type'
            assert (site['list_index'],site['list_count'])==(4115,1)and registers['ecx']&0xffff==4115 and registers['edx']&0xffff==1,'wrong original type list arguments'
            assert b['nested_callers']==['compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol']and (b['type_index'],b['list_index'],b['list_count'])==(4133,4115,1),'wrong native compact list frames/arguments'
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
