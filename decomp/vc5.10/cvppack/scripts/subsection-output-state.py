"""Actual58 copy calls: sorted live input history, rewritten offsets, new buffer."""
import struct


def validate(config,data,original,native):
    rows={row['name']:row for row in data}
    def root(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def other(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    public_prefix=bool(config.get('observe_public_writer_prefix'))
    if public_prefix:
        assert original['address']==0x40f960 and original['stack'][0]==0x40d62f and native['frontier_caller']=='write_publics','subsection next publicwriter child differs'
    else:assert original['address']==0x40d600 and original['stack'][0]==0x406c5a and native['frontier_caller']=='flush_packed_file','subsection next publics consumer differs'
    if not public_prefix:assert native['parameters']=={'entry':dict(root='directory_entries',index=58,extent=12),'offset':5056},'subsection publics actual output-pointer/scalar ABI differs'
    assert root('directory_count')==other('directory_count')==58,'subsection final row count differs'
    old=original['subsection_calls'];new=native['subsection_calls'];assert len(old)==len(new)==58,'subsection complete call inventory differs'
    before=original['directory_sort_complete']['bytes'];actual_before=bytes.fromhex(native['directory_sort_complete']['bytes'])
    assert len(before)==len(actual_before)==840,'subsection sorted-input history incomplete'
    padding=original['memory'][rows['output_padding']['original']];assert padding==bytes.fromhex(native['globals']['output_padding'])==bytes(4),'subsection contextual zero-padding differs'
    def source(state,address,size,hexadecimal):
        for retired in state['main_free_calls']+state['source_family_frees']:
            assert not retired['address'] or not size or max(address,retired['address'])>=min(address+size,retired['address']+retired['size']),'subsection source overlaps a retired generation'
        contents=[]
        for base,payload in state['memory'].items():
            base=int(base);payload=bytes.fromhex(payload) if hexadecimal else payload
            if base<=address and address+size<=base+len(payload):contents.append(payload[address-base:address-base+size])
        assert contents and all(value==contents[0] for value in contents),'subsection source lacks complete consistent live initialization'
        return contents[0]
    output=bytearray(b'NB11'+bytes(4));converted=bytearray();input_bytes=0;alignment_bytes=0
    for index,(a,b) in enumerate(zip(old,new)):
        left=before[index*12:index*12+12];right=actual_before[index*12:index*12+12]
        kind,module,address,size=struct.unpack('<HHII',left);actual_kind,actual_module,actual_address,actual_size=struct.unpack('<HHII',right)
        assert (kind,module,size)==(actual_kind,actual_module,actual_size) and kind in (0x120,0x125,0x127) and size>0,'subsection source row kind/module/extent differs'
        assert a['sequence']==b['sequence']==a['index']==b['index']==index and a['return_pc']==a['return_pc_after']==0x406c17,'subsection actual index/caller sequence differs'
        assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4 and b['formal_inventory']=={'index':'unsigned int'},'subsection actual cdecl/formal ABI differs'
        assert len(a['before_nonvolatile'])==len(b['before_nonvolatile'])==4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'subsection ordinary-return registers differ'
        assert a['before_row']==left and bytes.fromhex(b['before_row'])==right and a['input_address']==address and b['input_address']==actual_address,'subsection actual source pointer/row alias differs'
        payload=source(original,address,size,False);actual_payload=source(native,actual_address,size,True)
        assert a['input']==bytes.fromhex(b['input'])==payload==actual_payload and len(payload)==size,'subsection complete initialized source input differs'
        offset=len(output);assert a['before_used']==b['before_used']==offset,'subsection actual append position differs'
        output.extend(payload);count=4-(size&3)
        if count!=4:output.extend(padding[:count]);alignment_bytes+=count
        assert a['padding']==b['padding']==count and a['after_used']==b['after_used']==len(output),'subsection actual padding/output count differs'
        row=struct.pack('<HHII',kind,module,offset,size);assert a['after_row']==bytes.fromhex(b['after_row'])==row,'subsection rewritten relative row differs'
        converted.extend(row);input_bytes+=size
    assert input_bytes==5002 and alignment_bytes==46 and len(output)==5056,'subsection unmodeled source/align layout'
    for state,expected in [(original,bytes(converted)+before[696:]),(native,bytes(converted)+actual_before[696:])]:
        current=state['memory'][root('directory_entries')] if state is original else bytes.fromhex(state['memory'][str(other('directory_entries'))])
        assert len(current)==840 and current==expected,'subsection current directory offsets/tail differ from actual calls'
    assert original['output_buffer_state']['bytes']==bytes.fromhex(native['output_buffer_state']['bytes'])==bytes(output),'subsection complete initialized output buffer differs'
    assert root('output_padding_size')==other('output_padding_size')==4,'subsection final shared padding count differs'
    return dict(calls=58,actual_complete_cdecl_nonvolatile_source_inputs=True,source_initialized_bytes=input_bytes,alignment_bytes=alignment_bytes,initialized_output_prefix_bytes=len(output),own_unwritten_capacity_bytes=65512-len(output),rewritten_directory_rows=58,unchanged_initialized_tail_bytes=144,unwritten_directory_spare_bytes=72,sorted_pointer_history_separate_from_current_relative_offsets=True,next_consumer='insert_symbol_page_padding' if public_prefix else 'write_publics(spare entry58,offset5056)')
