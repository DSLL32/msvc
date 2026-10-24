"""Read just the reached local record and published recursive candidate."""
import gdb

def capture():
    f=gdb.newest_frame();caller=f.older();assert f.name()=='compare_recursive_type'and caller.name()=='intern_type'
    inferior=gdb.selected_inferior()
    def raw(address,size):return bytes(inferior.read_memory(address,size))
    entry=f.read_var('entry');entry_address=int(entry);type_index=int(f.read_var('type'));index=int(f.read_var('index'));record=int(f.read_var('record'))
    assert int(caller.read_var('entry'))==entry_address and int(caller.read_var('type'))==type_index
    table=gdb.parse_and_eval('local_type_records');capacity=int(gdb.parse_and_eval('local_type_capacity'));count=int(gdb.parse_and_eval('local_type_count'));first=int(gdb.parse_and_eval('first_user_type'));relative=type_index-first
    assert capacity==12000 and 0<=relative<count<=capacity and entry_address==int(table+relative)and entry.type.target().sizeof==28
    header=raw(entry_address,28);assert not int(entry['flags'])&0x100,'expanded recursive arrays pending'
    local=dict(table=int(table),capacity=capacity,count=count,first=first,type=type_index,entry=entry_address,bytes=header.hex())
    source=int(entry['data']);module=int(gdb.parse_and_eval('current_module[0].module'));directory=gdb.parse_and_eval('directory_entries');entries=int(gdb.parse_and_eval('directory_entry_count'))
    matches=[(i,directory[i])for i in range(entries)if int(directory[i]['subsection'])==0x121 and int(directory[i]['module'])==module]
    assert len(matches)==1;directory_index,section=matches[0]
    base=int(gdb.parse_and_eval('global_type_data'));requested=int(gdb.parse_and_eval('global_symbol_size'));span=int(section['size'])
    assert int(gdb.parse_and_eval('global_symbol_blocks[0]'))==base and 0<span<=requested<0xffe8 and base+4<=source<=base+span-4
    length=int.from_bytes(raw(source,2),'little')+2;assert 4<=length<=4096 and length<=base+span-source
    local_data=dict(module=module,input=base,requested=requested,defined=span,directory=directory_index,offset=int(section['location']['offset']),record=source,bytes=raw(source,length).hex())
    next_index=int(gdb.parse_and_eval('next_type_index'));relative=index-0x1000
    assert 0<=relative and index<next_index and relative//512<120
    page=gdb.parse_and_eval('type_pages[%d]'%(relative//512));assert int(page)and int(page[relative%512])==record
    storage=gdb.parse_and_eval('type_symbol_storage');block=storage['first'];blocks=int(storage['block_count']);size=int(storage['block_size']);matches=[]
    assert 0<blocks<=64
    for i in range(blocks):
        assert int(block);base=int(block['data']);used=int(block['used']);assert base and 0<=used<=size
        if base<=record<base+used:
            assert base+used-record>=4;length=int.from_bytes(raw(record,2),'little')+2
            assert 4<=length<=4096 and length<=base+used-record
            matches.append(dict(index=index,next=next_index,page=int(page),record=record,block=int(block),data=base,used=used,capacity=size,bytes=raw(record,length).hex()))
        block=block['next']
    assert not int(block)and len(matches)==1;published=matches[0]
    candidate=caller.read_var('candidate');bucket=int(caller.read_var('bucket'));assert bucket==int(entry['hash_value'])and 0<=bucket<4096
    assert int(candidate['data'])==record and int(candidate['index'])==index
    head=gdb.parse_and_eval('((RecursiveTypeEntry **)name_hash_buckets)[%d]'%bucket);hash_count=int(gdb.parse_and_eval('name_hash_counts[%d]'%bucket));cursor=int(gdb.parse_and_eval('recursive_type_cursor'));available=int(gdb.parse_and_eval('recursive_type_slots'))
    assert 0<hash_count<=1024 and 0<=available<1024;arena=cursor-(1024-available)*12;node=head;position=0
    while int(node):
        assert position<hash_count and arena<=int(node)<cursor and (int(node)-arena)%12==0
        if int(node)==int(candidate):break
        node=node['next'];position+=1
    assert int(node)==int(candidate)!=0
    hashed=dict(bucket=bucket,head=int(head),count=hash_count,position=position,cursor=cursor,available=available,arena=arena,node=int(node),bytes=raw(int(node),12).hex())
    return dict(entry=dict(local=local,input=local_data),record=dict(published=published,hash=hashed))
