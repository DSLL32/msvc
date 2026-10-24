"""Whole free→pending transition and constructor ownership, at mapped-open entry."""
import json
import struct
from file_open_state import slot_symbols
from mapped_input_contract import verify


def words(data):
    assert len(data)==40
    return struct.unpack('<10I',data)


def cache(slots,post=False):
    return dict(file_newest=slots[14 if post else 15],pending_files=slots[15]if post else 0,cached_files=0,cached_file_tail=0,cached_file_count=0)


def records(slots,post=False,name=0):
    result=[]
    for i in range(16):
        if post and i==15:result.append((name,0x13 if name else 0,15,0,0,0,0,0,0,0))
        else:result.append((0,0,i,0,0,0,0,0,0,slots[i-1]if i else 0))
    return result


def compare(original,state,trap,events,native_directory=None):
    verify()
    completed=state['function']in ('create_library','find_library','initialize_library_enumerator')
    mapping=state['function']in ('initialize_file_mapping','create_library','find_library','initialize_library_enumerator')
    assert native_directory is not None
    receipt=json.loads((native_directory/'receipt.json').read_text())
    symbols=slot_symbols(str(native_directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
    assert state['file_cache_slots']==symbols
    oslots=list(struct.unpack('<16I',(original/'file-slots.bin').read_bytes()))
    nslots=list(struct.unpack('<16I',bytes.fromhex(state['file_slots_bytes'])))
    assert len(set(oslots))==len(set(nslots))==16 and all(oslots)and all(nslots)
    flow=[e for e in events if e['event']=='file_flow_observation']
    assert [(e['kind'],e['before'],e['index'])for e in flow]==[('acquire',1,0),('available',1,0),('move',1,0),('move',0,0),('available',0,0),('name',1,0),('name',0,0),('acquire',0,0)]
    obefore,oafter=[e for e in flow if e['kind']=='move']
    omargs=[oslots[15],0x46753c,0,0x4674d8,0,0]
    nmargs=[nslots[15],symbols['file_newest'],0,symbols['pending_files'],0,0]
    assert obefore['arguments']==oafter['arguments']==omargs
    assert (obefore['site'],oafter['site'])==(0x40b951,0x40b956)
    assert obefore['stack']==obefore['entry_stack']==oafter['entry_stack']
    assert oafter['stack']==obefore['stack']+24
    for post,e in enumerate((obefore,oafter)):
        assert e['cache']==cache(oslots,bool(post))
        phase='file-flow-move-0-'+('after'if post else 'before')
        assert (original/(phase+'-slots.bin')).read_bytes()==struct.pack('<16I',*oslots)
        assert [words((original/(phase+'-record-%02u.bin'%i)).read_bytes())for i in range(16)]==records(oslots,bool(post))
    move,=state['file_flow_calls']['move']
    assert move['arguments']==nmargs and move['normal_return']is True
    assert move['return_stack']==move['entry_stack']+28
    for post,key in enumerate(('before','after')):
        snap=move[key]
        assert snap['global_slots']==symbols and snap['slots']==nslots
        assert snap['table']==state['file_globals']['file_slots']
        assert snap['table_bytes']==state['file_slots_bytes']
        assert snap['cache']==cache(nslots,bool(post))
        assert [words(bytes.fromhex(r))for r in snap['records']]==records(nslots,bool(post))
    entry,=[e for e in events if e['event']=='open_file_entry'];nentry=state['open_file_entry']
    acquire,=[e for e in events if e['event']=='file_record_entry'];nacquire=state['file_record_entry']
    expected=[trap['output_name_pointer'],0x8302,0x180];nexpected=[state['output_name_pointer'],0x8302,0x180]
    assert entry['formals']==expected and nentry['formals']==nexpected
    assert entry['hidden_inputs']==nentry['hidden_inputs']==acquire['hidden_inputs']==nacquire['hidden_inputs']==[]
    assert acquire['stack_pointer']==entry['stack_pointer']-44
    assert trap['esp']==entry['stack_pointer']-(848 if state['function']=='initialize_library_enumerator'else 28 if state['function']=='find_library'else 0 if completed else 76 if mapping else 44)and trap['stack'][0]==(0x40f674 if state['function']=='initialize_library_enumerator'else 0x40ede7 if state['function']=='find_library'else 0x44b906 if completed else 0x40c9cf if mapping else 0x40b84a)
    assert obefore['stack']==acquire['stack_pointer']-44
    if not mapping:assert trap['ebp']==0xffffffff
    current=trap['file_record_formals'];ncurrent=state['file_record_formals']
    for c,e,name in ((current,entry,expected[0]),(ncurrent,nentry,nexpected[0])):
        assert[c[k]for k in ('name','flags','mode')]==[name,0x8302,0x180]
        assert c['initial_created']==1 and c['initial_size']==c['file_access_statistics']==0
        assert c['created_bytes']==1 and c['created'] and c['size_slot']
        assert c['created']<e['stack_pointer']and c['size_slot']+4<=e['stack_pointer']
        assert not c['size_slot']<=c['created']<c['size_slot']+4
    assert current['created']==entry['stack_pointer']-8 and current['size_slot']==entry['stack_pointer']-4
    assert ncurrent['created']==ncurrent['local_created']and ncurrent['initial_mapped_result']==-1
    assert acquire['formals']==expected+[current['created']]
    assert nacquire['formals']==nexpected+[ncurrent['created']]
    lookbefore,lookafter=[e for e in events if e['event']=='file_lookup_observation']
    assert (lookbefore['site'],lookafter['site'])==(0x40b878,0x40b87d)
    assert lookbefore['before']==1 and lookafter['before']==0
    assert lookbefore['name']==lookafter['name']==expected[0]and lookafter['result']==0
    assert lookbefore['stack']==lookbefore['entry_stack']==lookafter['entry_stack']==acquire['stack_pointer']-16
    assert lookafter['stack']==lookbefore['stack']+4
    lookup,=state['file_lookup_calls'];assert lookup['name']==nexpected[0]and lookup['result']==0 and lookup['normal_return']is True and lookup['return_stack']==lookup['entry_stack']+8
    avail_before,avail_after=[e for e in flow if e['kind']=='available']
    assert (avail_before['site'],avail_after['site'])==(0x40b885,0x40b88a)
    assert avail_before['arguments']==avail_after['arguments']==[] and avail_after['result']==oslots[15]
    assert avail_before['stack']==avail_before['entry_stack']==avail_after['entry_stack']==avail_after['stack']==acquire['stack_pointer']-12
    available,=state['file_flow_calls']['available'];assert available['arguments']==[]and available['result']==nslots[15]and available['normal_return']is True and available['return_stack']==available['entry_stack']+4
    assert available['before']==move['before']and available['after']==move['after']
    acq_before,acq_after=[e for e in flow if e['kind']=='acquire']
    assert(acq_before['site'],acq_after['site'])==(0x40b829,0x40b82e)
    assert acq_before['arguments']==acq_after['arguments']==acquire['formals']
    assert acq_before['stack']==acq_before['entry_stack']==acq_after['entry_stack']==acquire['stack_pointer']+4
    assert acq_after['stack']==acq_before['stack']+16
    assert acq_before['created']==0 and acq_after['created']==1 and acq_after['result']==oslots[15]
    nacq=state['file_acquire_return'];assert nacq['arguments']==nacquire['formals']and nacq['entry_stack']==nacquire['stack_pointer']
    assert nacq['before_created']==0 and nacq['after_created']==1 and nacq['result']==nslots[15]and nacq['normal_return']is True and nacq['return_stack']==nacq['entry_stack']+20
    before,after=[e for e in flow if e['kind']=='name']
    assert(before['site'],after['site'])==(0x40b898,0x40b89d)
    assert before['text']==after['text']=='hello.exe'and before['arguments']==after['arguments']==[expected[0]]
    assert before['stack']==before['entry_stack']==after['entry_stack']and after['stack']==before['stack']+4
    name=after['result'];nname=state['file_name_call'];newname=nname['result']
    assert nname['arguments']==[nexpected[0]]and nname['text']=='hello.exe'and nname['normal_return']is True
    assert nname['return_stack']==nname['entry_stack']+8
    assert (original/'file-copied-name-0.bin').read_bytes()==(original/'file-record-name-15.bin').read_bytes()==bytes.fromhex(nname['name_bytes'])==bytes.fromhex(nacq['name_bytes'])==bytes.fromhex(state['file_record_name_bytes'])==b'hello.exe\0'
    assert name and newname and name!=expected[0]and newname!=nexpected[0]
    assert nacq['name']==newname
    assert words((original/'file-acquire-result-0.bin').read_bytes())==records(oslots,True,name)[15]
    assert words(bytes.fromhex(nacq['raw']))==records(nslots,True,newname)[15]
    final_records=records(oslots,True,name);native_records=records(nslots,True,newname)
    if mapping:
        from file_mapping_state import compare_created,mapping_symbol
        assert state['mapped_file_slots_address']==mapping_symbol(str(native_directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
        handle,nhandle=compare_created(original,state,trap,events,name,newname,oslots,nslots)
        if completed:
            from mapping_completion_state import compare as compare_mapping
            view,nview=compare_mapping(original,state,trap,events,oslots,nslots,handle,nhandle,native_directory)
        for pool,value in ((final_records,handle),(native_records,nhandle)):
            last=list(pool[15]);last[4]=value;last[6]=0x80000
            if completed:last[1]=0x1b;last[5]=view if pool is final_records else nview
            pool[15]=tuple(last)
    assert [words((original/('file-record-%02u.bin'%i)).read_bytes())for i in range(16)]==final_records
    assert [words(bytes.fromhex(r))for r in state['file_records']]==native_records
    assert trap['file_cache_lists']=={k:v for k,v in cache(oslots,True).items()if k!='file_newest'}
    assert state['file_cache_lists']=={k:v for k,v in cache(nslots,True).items()if k!='file_newest'}
    assert state['duplicate_calls'][-1]==dict(input=nexpected[0],text='hello.exe',result=newname)
    mapped=trap['mapped_open_formals'];nmapped=state['mapped_open_formals']
    raw=state['mapped_open_entry']
    assert raw['hidden_inputs']==[]and len(raw['raw_formals'])==4
    assert raw['raw_formals']==[nmapped['record'],nmapped['flags'],nmapped['raw_created_word'],nmapped['size']]
    for m,slots,c in ((mapped,oslots,current),(nmapped,nslots,ncurrent)):
        assert m['record']==slots[15]and m['flags']==0x8302 and m['created']==1
        assert 0<=m['raw_created_word']<=0xffffffff and m['raw_created_word']&255==1
        assert m['raw_upper_bits']==m['raw_created_word']>>8
        assert m['size']==c['size_slot']and m['initial_size']==0
    if mapping:
        observed,=[e for e in events if e['event']=='mapped_open_entry']
        assert observed['hidden_inputs']==[]and observed['stack_pointer']==entry['stack_pointer']-44
        assert observed['raw_formals']==[mapped['record'],mapped['flags'],mapped['raw_created_word'],mapped['size']]
    else:assert trap['stack'][1:5]==[mapped['record'],mapped['flags'],mapped['raw_created_word'],mapped['size']]
    if not mapping:
        for metadata,m in ((trap,mapped),(state,nmapped)):
            assert metadata['frontier_inputs']==dict(formals=[m['record'],m['flags'],1,m['size']],hidden_register_inputs=[])
    # Return the newly relevant owned allocations to the existing whole-context
    # disjointness checker: table, all complete records and the copied name.
    ranges=[dict(original_base=a,native_base=b,bytes=40)for a,b in zip(oslots,nslots)]
    if completed:ranges.append(dict(original_base=view,native_base=nview,bytes=0x80000))
    ranges.extend([dict(original_base=struct.unpack('<I',(original/'file-pointers.bin').read_bytes()[:4])[0],native_base=state['file_globals']['file_slots'],bytes=64),dict(original_base=name,native_base=newname,bytes=10)])
    return ranges
