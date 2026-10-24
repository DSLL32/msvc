"""Initialized public-writer prologue state before its first unknown helper."""


def validate(config,data,original,native):
    padding=bool(config.get('observe_public_padding'));name_hash=bool(config.get('observe_name_hash'))
    complete=bool(config.get('observe_name_hash_complete'))
    if complete:
        assert original['address']==0x40ede0 and original['stack'][0]==0x40d68f and native['frontier_caller']=='write_publics','public writer addresshash child differs'
    elif name_hash:
        assert original['address']==0x410010 and original['stack'][0]==0x40ebc0 and native['frontier_caller']=='build_symbol_name_hash','public writer next stream-init differs'
    elif padding:
        assert original['address']==0x40e920 and original['stack'][0]==0x40d66a and native['frontier_caller']=='write_publics','public writer next namehash child differs'
    else:assert original['address']==0x40f960 and original['stack'][0]==0x40d62f and native['frontier_caller']=='write_publics','public writer first real child differs'
    if not padding:assert native['parameters']=={'buckets':dict(null=False,global_name='type_table_global'),'reserve':4},'public writer child argument schema differs'
    rows={row['name']:row for row in data}
    def root(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def actual(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    if name_hash:assert original['name_hash_entry']['stack'][6]==root('type_table_global') and original['name_hash_entry']['stack'][5]==64,'public writer actual original namehash inputs differ'
    else:assert (original['stack'][6]==root('type_table_global') and original['stack'][5]==64) if padding else (original['stack'][1]==root('type_table_global') and original['stack'][2]==4),'public writer actual original bucket input/reserve differs'
    parent=native['public_writer_frame']
    assert parent['owner']=='write_publics' and parent['caller']=='flush_packed_file','public writer parent identities differ'
    shift=3 if complete else 32 if name_hash else 4 if padding else 0
    assert original['stack'][24+shift]==0x406c5a and original['stack'][25+shift]==root('directory_entries')+58*12 and original['stack'][26+shift]==5056,'public writer original parent arguments/stack differ'
    assert parent['entry']==actual('directory_entries')+58*12 and parent['offset']==5056,'public writer native parent arguments differ'
    expected={'name_method':dict(offset=0,width=2),'address_method':dict(offset=2,width=2),'symbol_bytes':dict(offset=4,width=4),'name_bytes':dict(offset=8,width=4),'address_bytes':dict(offset=12,width=4)}
    assert parent['header_layout']==expected,'public writer actual header fields differ'
    assert parent['header_address'] and parent['padding_address'] and (parent['header_address']+16<=parent['padding_address'] or parent['padding_address']+4<=parent['header_address']),'public writer initialized objects overlap'
    # Canonical prologue stores the DWORD padding at child ESP+28, then the
    # 16-byte header at ESP+32. The following 48B stream contents are excluded from typed comparison;
    # the original fixed 128B raw stack transport still retains their own bits.
    stack=original['memory'][original['esp']]
    expected_header=(b'\x0a\0\0\0'+(2116).to_bytes(4,'little')+(564).to_bytes(4,'little')+bytes(4)) if complete else bytes(4)+(2116 if padding else 0).to_bytes(4,'little')+bytes(8)
    begin=40 if complete else 156 if name_hash else 44 if padding else 28
    assert stack[begin:begin+20]==bytes(4)+expected_header and bytes.fromhex(parent['header'])==expected_header and parent['padding']==0,'public writer initialized header/padding differs'
    streams=parent['stream_objects'] if name_hash else parent['uninitialized_streams'];assert all(set(row)=={'local','address','width','first_offset'} for row in streams),'public writer unknown stream contents admitted'
    assert [{k:row[k] for k in ('local','width','first_offset')} for row in streams]==[dict(local=name,width=24,first_offset=20) for name in ('names','addresses')],'public writer actual stream types differ'
    objects=[(parent['header_address'],16),(parent['padding_address'],4)]+[(row['address'],row['width']) for row in streams]
    assert all(address and size and all(max(address,other)>=min(address+size,other+length) for other,length in objects[index+1:]) for index,(address,size) in enumerate(objects)),'public writer automatic object identities overlap'
    assert parent['uninitialized_stream_bytes_uncompared']==sum(row['width'] for row in parent['uninitialized_streams'])==(24 if name_hash else 48) and (not name_hash or parent['uninitialized_streams']==streams[1:]),'public writer unknown stream state admitted'
    assert root('output_used')==actual('output_used')==5056 and root('directory_count')==actual('directory_count')==58,'public writer modified output before first helper'
    return dict(parent_original_return_pc=0x406c5a,child_original_return_pc=0x40d68f if complete else 0x40ebc0 if name_hash else 0x40d66a if padding else 0x40d62f,bucket_input='type_table_global(public buckets)',reserve=4,initialized_header_bytes=16,initialized_padding_bytes=4,uninitialized_stream_bytes_uncompared=24 if name_hash else 48,output_prefix_unchanged_bytes=5056,next_consumer='build_symbol_address_hash' if complete else 'fill_type_arena' if name_hash else 'build_symbol_name_hash' if padding else 'insert_symbol_page_padding(buckets,reserve4)')
