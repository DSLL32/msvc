"""Typed reached-data view after this case's global IL has been consumed.

This walks symbol/section/filename data, not machine control flow. It preserves
all4096 arena bytes, normalizing only documented pointer fields. Unknown kinds,
pools, out-of-arena pointers or string identities fail rather than disappear.
"""
import struct

ROOTS = {
    'pools': (0x48e020, 'c2_pools'), 'free_blocks': (0x48e000, 'c2_free_blocks'),
    'types': (0x497fd8, 'c2_types'), 'scope': (0x48fb68, 'global_scope'),
    'section_head': (0x497dd0, 'section_head'), 'section_tail': (0x497db8, 'section_tail'),
    'current_function':(0x48fb48,'current_function'),'code_generation_state':(0x491064,'code_generation_state'),
    'function_input_flags':(0x490be4,'function_input_flags'),'function_analysis_available':(0x48fb3c,'function_analysis_available'),
    'selected_symbol':(0x495f1c,'current_symbol'),'volatile_limit':(0x499734,'volatile_pass_limit'),
    'filenames': (0x4973b8, 'filenames'), 'pending': (0x4973d4, 'pending_functions'),
    'insertion': (0x4973f0, 'pending_insertion'), 'context': (0x497f58, 'section_context'),
    'template': (0x497308, 'symbol_template'), 'input_name': (0x496a00, 'symbol_name'),
    'decorated_name': (0x497460, 'decorated_name'), 'source_pool': (0x48e050, 'c2_pools[4]'),
    'output_directives': (0x4980c0, 'output_directives'),
    'object_output': (0x497e08, 'object_output'),
    'object_output_open': (0x497e1c, 'object_output_open'),
    'incremental_data': (0x49e31c, 'incremental_data'),
    'directive_section': (0x4973fc, 'directive_section'),
    'fpo_section': (0x497dc0, 'fpo_section'),
    'writer_allocator': (0x41eb8a, 'allocate_output_bytes'),
    'writer_error': (0x48511c, 'writer_callback_48511c'),
    'writer_clock': (0x45477a, 'writer_callback_45477a'),
}


COFF_GLOBALS = {
    'pending_debug_symbols':(0x497458,4),'c2_zero_alignment':(0x490b74,4),
    'global_initializers':(0x497d98,4),'global_records_processing':(0x497df0,4),
    'output_allocate': (0x497d88, 4), 'output_error_handler': (0x4999f4, 4),
    'output_clock_handler': (0x497e00, 4), 'coff_new_output': (0x497da0, 4),
    'record_source_line':(0x48e004,4),'record_line_delta':(0x48e008,2),
    'assembly_output_active': (0x497da4,4), 'assembly_comment_character': (0x49e69c,1),
    'c2_assembly_enabled': (0x497f98,4), 'c2_mixed_assembly': (0x497f94,4),
    'coff_machine': (0x497e04, 2), 'coff_source_name': (0x4999e8, 4),
    'coff_output': (0x497e14, 4), 'coff_pending04': (0x4999ec, 4),
    'coff_debug': (0x497ccd, 1), 'coff_table': (0x497d74, 4),
    'coff_storage': (0x497d70, 4), 'coff_symbol_buffer': (0x497d48, 4),
    'coff_symbol_capacity': (0x497d50, 4), 'coff_default_flags': (0x497cb0, 2),
    'coff_flags': (0x497d9c, 2), 'coff_pending_e18': (0x497e18, 2),
    'coff_section_count': (0x497d78, 2), 'coff_current_section': (0x497d6c, 4),
    'coff_pending_c90': (0x497c90, 4), 'coff_output_list': (0x497d44, 4),
    'coff_current_list': (0x497d40,4), 'coff_free_chunks': (0x497c70,4),
    'coff_free_blocks_508': (0x497ce8,4), 'coff_free_blocks_308': (0x497f5c,4),
    'coff_pending_499690': (0x499690,4),
    'coff_symbol_total': (0x497d4c,4), 'coff_next_symbol': (0x497d54,4),
    'coff_directive_option': (0x497dac,4),
    'function_option_23': (0x48fb60,4),
    'coff_reuse_index': (0x49d5ec,4), 'coff_reuse_output': (0x497da8,4),
    'coff_reserved_symbols': (0x497d5c,4), 'coff_previous_source': (0x497f68,4),
}
ROOTS.update({key:(value[0],key)for key,value in COFF_GLOBALS.items()})
ROOTS['coff_reuse_sections']=(0x49e6e0,'coff_reuse_sections')
ROOTS['coff_section_buckets']=(0x497d00,'coff_section_buckets')

REGISTER_ROOTS = {'function_register_blocks': (4782220, 'function_register_blocks'), 'next_function_register_block': (4782760, 'next_function_register_block'), 'end_function_register_blocks': (4782756, 'end_function_register_blocks'), 'next_function_register_number': (4782736, 'next_function_register_number'), 'first_function_register_block': (4776664, 'first_function_register_block'), 'last_function_register_block': (4782752, 'last_function_register_block'), 'tracked_register_block': (4776656, 'tracked_register_block'), 'secondary_register_block': (4776652, 'secondary_register_block'), 'plain_register_block': (4776648, 'plain_register_block'), 'default_register_block': (4776636, 'default_register_block'), 'extra_register_block': (4776644, 'extra_register_block'), 'free_plain_register_nodes': (4776628, 'free_plain_register_nodes'), 'free_tracked_register_nodes': (4776632, 'free_tracked_register_nodes'), 'pending_debug_symbols': (4813912, 'pending_debug_symbols'), 'secondary_register_set': (4776640, 'secondary_register_set'), 'tracked_register_set': (4776660, 'tracked_register_set'), 'empty_function_register': (4813836, 'empty_function_register')}
ROOTS.update(REGISTER_ROOTS)


def register_pool_state(read, head, count):
    """Known allocator-written spans plus encountered register-block fields."""
    assert count == 4, 'new natural register allocation shape needs explicit proof'
    chain=[]; current=head
    while current:
        assert current not in chain and len(chain)<count+1, 'cyclic/extra function pool arena'
        data=read(current,4096)
        assert len(data)==4096 and struct.unpack_from('<I',data,4)[0]==4088
        chain.append(current);current=struct.unpack_from('<I',data)[0]
    assert len(chain)==count+1, 'function pool arena count differs'
    bases=list(reversed(chain));blocks=[]
    identity=lambda index:0x50000000+index*0x10000
    for index,base in enumerate(bases[1:],1):
        raw=bytearray(read(base,4096));expected=bytearray(4096)
        pointers={0:(bases[index-1],identity(index-1)),8:(bases[index+1]+8 if index<count else 0,identity(index+1)+8 if index<count else 0),12:(base+20,identity(index)+20),16:(base+2580,identity(index)+2580)}
        for offset,(actual,logical)in pointers.items():
            assert struct.unpack_from('<I',raw,offset)[0]==actual, 'register block/pool header identity differs'
            struct.pack_into('<I',raw,offset,logical);struct.pack_into('<I',expected,offset,logical)
        struct.pack_into('<I',expected,4,4088)
        for row in range(32):
            value=(index-1)*32+row
            assert struct.unpack_from('<I',raw,48+80*row)[0]==value, 'register row number differs'
            struct.pack_into('<I',expected,48+80*row,value)
        assert raw==expected, 'allocator-defined register arena bytes or own zeroed tails differ'
        blocks.append({'index':index,'register_count':32,'first_number':(index-1)*32,'allocator_bytes':raw.hex()})
    return bases,{'arena_count':count+1,'register_blocks':count,'blocks':blocks,'byte_scope':'Each exact4096B arena: real allocator writes two headerDWORDs and zeroes4088B. Only encountered pointer/header/number fields are interpreted; other zero bytes have allocator-writer provenance, no semantic/default/layout credit.'}


def complete_globals(read, addresses, directives=False, writer=False, coff=False, initialized_output=False, list_pending=False, list_constructed=False, source_pending=False, symbols_complete=False, append_pending=False, append_complete=False, section_symbols_pending=False, section_symbols_complete=False, storage_complete=False, section_recorded=False, section_selected=False, output_bytes_written=False, output_directives_complete=False, function_selected=False, register_directory=False, register_blocks=0):
    payload=b'-defaultlib:LIBC -defaultlib:OLDNAMES 'if output_directives_complete else b'-defaultlib:LIBC'if output_bytes_written else b''
    written=len(payload);writes=6 if output_directives_complete else 2 if output_bytes_written else 0
    def u32(address): return struct.unpack('<I', read(address, 4))[0]
    pool_words = struct.unpack('<12I', read(addresses['pools'], 48))
    head, cursor, remaining = pool_words[:3]
    assert head and cursor - head == (2068 if symbols_complete else 1892) and remaining == (2028 if symbols_complete else 2204)
    assert pool_words[6:] == (0,) * 6 and read(addresses['source_pool'], 12) == bytes(12)
    if not directives: assert pool_words[3:6] == (0,) * 3
    assert u32(addresses['free_blocks']) == 0 and u32(addresses['types']) == head + 8
    block = bytearray(read(head, 4096))
    assert block[:8] == struct.pack('<II', 0, 4088)
    context = u32(addresses['context'])
    assert context and not head <= context < head + 4096
    strings, string_pointers = [], {}

    def arena_pointer(pointer):
        if not pointer: return 0
        assert head + 8 <= pointer < cursor, 'pointer outside allocated arena'
        return 0x10000000 + pointer - head

    def string(pointer):
        assert pointer
        value = bytearray()
        for i in range(2059):
            byte = read(pointer + i, 1)
            if byte == b'\0': break
            value += byte
        else: raise AssertionError('unbounded reached string')
        text = value.decode('ascii')
        if head <= pointer < cursor:
            return arena_pointer(pointer), text
        if pointer not in string_pointers:
            string_pointers[pointer] = 0x20000000 + len(strings)
            strings.append(text)
        return string_pointers[pointer], text

    def patch(at, value): struct.pack_into('<I', block, at - head, value)
    scope = bytearray(read(addresses['scope'], 4100))
    assert scope[:4] == bytes(4)
    objects = {}
    for bucket in range(1024):
        pointer = struct.unpack_from('<I', scope, 4 + bucket * 4)[0]
        struct.pack_into('<I', scope, 4 + bucket * 4, arena_pointer(pointer))
        chain = set()
        while pointer:
            assert pointer not in chain and pointer not in objects, 'duplicate/cyclic symbol'
            chain.add(pointer)
            kind = read(pointer + 4, 1)[0]
            assert kind in (4, 9), 'new symbol kind needs an explicit data model'
            size = 86 if kind == 9 or not (u32(pointer + 54) & 0x4000) else 110
            assert pointer + size <= cursor and u32(pointer + 40) & 1023 == bucket
            objects[pointer] = (kind, size)
            pointer = u32(pointer)
    assert len(objects) == (19 if symbols_complete else 17)
    inventory = []
    for pointer in sorted(objects):
        kind, size = objects[pointer]
        patch(pointer, arena_pointer(u32(pointer)))
        name_pointer, name = string(u32(pointer + 24)); patch(pointer + 24, name_pointer)
        row = dict(offset=pointer-head, kind=kind, size=size, serial=u32(pointer+40), name=name)
        if kind == 9:
            class_pointer, class_name = string(u32(pointer+54)); patch(pointer+54, class_pointer)
            patch(pointer+50, arena_pointer(u32(pointer+50)))
            combine = read(pointer+67, 1)[0]
            if combine == 7:
                group = u32(pointer+36)
                assert read(group,12) == bytes(12), 'COMDAT group reserved/count state differs'
                patch(pointer+36, arena_pointer(group))
                patch(pointer+74, arena_pointer(u32(pointer+74)))
                patch(pointer+78, arena_pointer(u32(pointer+78)))
            elif combine == 0:
                assert symbols_complete and class_name in ('DRECTVE','DEBFPO') and u32(pointer+36) == 0
            else:
                assert combine == 2 and u32(pointer+36) == context
                patch(pointer+36, 0xf0000001)
            row['class_name'] = class_name
        elif size == 110:
            patch(pointer+102, arena_pointer(u32(pointer+102)))
        inventory.append(row)
    expected = [
        (72,9,86,5,'_TEXT'),(160,9,86,2,'_DATA'),(248,9,86,3,'CONST'),
        (336,9,86,4,'_BSS'),(424,9,86,9,'_TLS'),
        (548,4,110,85,'_mix_unsigned'),(676,9,86,146,'_mix_unsigned'),
        (776,4,110,92,'_mix_signed'),(900,9,86,147,'_mix_signed'),
        (1000,4,110,100,'_nested_loops'),(1128,9,86,148,'_nested_loops'),
        (1228,4,110,122,'_pointer_aliasing'),(1360,9,86,149,'_pointer_aliasing'),
        (1460,4,110,135,'_floating_point'),(1588,9,86,150,'_floating_point'),
        (1688,4,86,80,'_floating_call'),(1792,4,86,79,'_observe')]
    if symbols_complete:expected += [(1892,9,86,151,'.drectve'),(1980,9,86,152,'.debug$F')]
    if output_bytes_written or output_directives_complete:assert u32(head+1892+12)==written,'directive section byte count differs'
    assert [(r['offset'],r['kind'],r['size'],r['serial'],r['name'])for r in inventory] == expected
    sections, pointer = [], u32(addresses['section_head'])
    while pointer:
        assert len(sections) < (12 if symbols_complete else 10) and objects[pointer][0] == 9
        sections.append(pointer-head); pointer = u32(pointer+50)
    assert sections == [72,160,248,336,424,676,900,1128,1360,1588]+([1892,1980]if symbols_complete else [])
    assert u32(addresses['section_tail']) == head+(1980 if symbols_complete else 1588)
    pending, pointer = [], u32(addresses['pending'])
    while pointer:
        assert len(pending) < 5 and objects[pointer] == (4,110)
        pending.append((pointer-head,u32(pointer+86)));pointer=u32(pointer+102)
    expected_pending=[(548,1132),(776,1288),(1000,1453),(1228,1910),(1460,2413)]
    assert pending == (expected_pending[1:] if function_selected else expected_pending)
    assert u32(addresses['insertion']) == head+1330
    pointer = u32(addresses['filenames'])
    assert pointer == head+536 and u32(pointer) == 86 and u32(pointer+8) == 0
    name_pointer, name = string(u32(pointer+4)); assert name == 'numeric-control-flow.c'
    patch(pointer+4,name_pointer);patch(pointer+8,0)
    template = bytearray(read(addresses['template'],64))
    pointer = struct.unpack_from('<I',template,24)[0]
    struct.pack_into('<I',template,24,string(pointer)[0])
    # All unaddressed padding, reserved fields, string bytes and unused arena
    # tail remain in this normalized4096-byte view and must match exactly.
    result = dict(pool_cursor=2068 if symbols_complete else 1892,pool_remaining=2028 if symbols_complete else 2204,pool_bytes=block.hex(),scope_bytes=scope.hex(),
        objects=inventory,external_strings=strings,sections=sections,pending=pending,
        insertion_offset=1330,filename_offset=536,template_bytes=template.hex(),
        input_bytes=read(addresses['input_name'],2058).hex(),
        decorated_bytes=read(addresses['decorated_name'],2059).hex())

    if function_selected:
        assert u32(addresses['selected_symbol'])==head+548,'selected function is not first actual pending symbol'
        assert u32(head+548+106)==0x2a and u32(head+548+102)==head+776,'selected function consumedflag/next ownership differs'
        assert u32(addresses['volatile_limit'])==128,'controller volatile pass limit differs'
        result['function_selection']={'offset':548,'serial':85,'additional_flags':0x2a,'pending_root_offset':776,'volatile_limit':128}
        result['function_setup']={key:u32(addresses[key])for key in ['current_function','code_generation_state','function_input_flags','function_analysis_available']}
        assert result['function_setup']==dict(current_function=0,code_generation_state=0,function_input_flags=0,function_analysis_available=0),'setup writes occurred before the actual first reset-child trap'
    if directives:
        directive_head, directive_cursor, directive_remaining = pool_words[3:6]
        assert directive_head and directive_head != head
        if register_blocks:
            pool_bases,pool_state=register_pool_state(read,directive_head,register_blocks)
            assert all(base!=head for base in pool_bases)
            assert directive_cursor==pool_bases[-1]+2580 and directive_remaining==1516
            directive_head=pool_bases[0]
            expected_cursor=2580
            result['function_pools']=pool_state
        else:
            expected_cursor=4032 if register_directory else 32
            assert directive_cursor == directive_head + expected_cursor and directive_remaining == 4096-expected_cursor
        assert u32(addresses['output_directives']) == directive_head + 8
        data = bytearray(read(directive_head, 4096))
        assert data[:8] == struct.pack('<II', 0, 4088)
        nodes = []
        for offset, next_offset, expected_text in ((8, 20, 'LIBC'), (20, 0, 'OLDNAMES')):
            next_pointer, kind, value = struct.unpack_from('<III', data, offset)
            assert next_pointer == (directive_head + next_offset if next_offset else 0)
            assert kind == 4
            normalized, text = string(value)
            assert text == expected_text
            struct.pack_into('<III', data, offset, 0x30000000 + next_offset if next_offset else 0, kind, normalized)
            nodes.append(dict(offset=offset, next_offset=next_offset, kind=kind, value=text))
        # The accepted allocator writes both header DWORDs and zeroes all4088
        # payload bytes; append writes only kind/value and the prior link.
        # The unwritten second link and remaining4064 bytes retain that zeroing.
        if register_blocks:
            for index in range(register_blocks):
                offset=32+4*index
                assert struct.unpack_from('<I',data,offset)[0]==pool_bases[index+1]+8, 'register directory slot identity differs'
                struct.pack_into('<I',data,offset,0x50000000+(index+1)*0x10000+8)
        result['directives'] = dict(head_offset=8, cursor_offset=expected_cursor, remaining=directive_remaining,
            nodes=nodes, pool_bytes=data.hex())
    if register_directory:
        assert directives and function_selected
        assert u32(addresses['function_register_blocks'])==directive_head+32
        assert u32(addresses['next_function_register_block'])==directive_head+32+4*register_blocks
        assert u32(addresses['end_function_register_blocks'])==directive_head+4032
        pending={name:u32(addresses[name])for name in REGISTER_ROOTS if name not in ('function_register_blocks','next_function_register_block','end_function_register_blocks')}
        if register_blocks:
            actual={'first_function_register_block':1,'last_function_register_block':4,'tracked_register_block':1,'secondary_register_block':2,'plain_register_block':3,'default_register_block':4}
            for name,index in actual.items():
                assert pending[name]==pool_bases[index]+8, 'register global block alias differs'
                pending[name]=index
            assert pending['next_function_register_number']==128
            assert all(value==0 for name,value in pending.items()if name not in actual and name!='next_function_register_number'), 'unexecuted register-set or later node constructor changed state'
        else:
            assert all(v==0 for v in pending.values()),'unexecuted register constructors or reset state differ'
        offset=32+4*register_blocks
        assert read(directive_head+offset,4096-offset)==bytes(4096-offset),'unused pointer directory or own untouched tail differs'
        result['function_registers']={'directory_offset':32,'cursor_offset':offset,'end_offset':4032,'capacity':1000,'allocation_bytes':4000,'pending':pending}
    if writer:
        assert directives
        later_pools = struct.unpack('<9I', read(addresses['pools'] + 60, 36))
        assert later_pools[:6] == (0,) * 6, 'unencountered pools5/6 changed'
        writer_head, writer_cursor, writer_remaining = later_pools[6:]
        assert writer_head and writer_head not in (head, directive_head)
        expected_cursor=(3356 if section_recorded else 3348 if storage_complete else 3340 if section_symbols_complete else 3316 if section_symbols_pending else 1908 if append_pending or append_complete else 1884 if symbols_complete else 1872 if source_pending else 1852 if list_constructed else 1344 if list_pending else 1320)if initialized_output else 64
        expected_remaining=4096-expected_cursor
        assert writer_cursor == writer_head + expected_cursor and writer_remaining == expected_remaining
        data = bytearray(read(writer_head, 4096))
        assert data[:8] == struct.pack('<II', 0, 4088)
        assert struct.unpack_from('<HHI', data, 8) == (1, 0x14c, 0xffffffff)
        source, output = struct.unpack_from('<II', data, 16)
        assert source == writer_head + 40
        assert read(source, 23) == b'numeric-control-flow.c\0'
        assert output == u32(addresses['object_output']) and output
        assert u32(addresses['object_output_open']) == 1
        assert u32(addresses['incremental_data']) == 0
        assert u32(addresses['directive_section']) == (head+1892 if symbols_complete else 0)
        assert u32(addresses['fpo_section']) == (head+1980 if symbols_complete else 0)
        targets = []
        for offset, key, normalized in ((24,'writer_allocator',0x50000001),
                (28,'writer_error',0x50000002),(32,'writer_clock',0x50000003)):
            assert struct.unpack_from('<I', data, offset)[0] == addresses[key]
            struct.pack_into('<I', data, offset, normalized)
            targets.append(key)
        assert data[36] == 0
        struct.pack_into('<II', data, 16, 0x60000028, 0x40000006)
        if initialized_output:
            assert struct.unpack_from('<IH2x6I',data,64)==(1 if append_complete else 0,300,1 if append_complete else 0,0,0,0,writer_head+96,writer_head+96)
            struct.pack_into('<II',data,88,0x60000060,0x60000060)
            if append_complete:
                assert struct.unpack_from('<II',data,96)==(0,writer_head+1884),'first generic table item differs'
                struct.pack_into('<I',data,100,0x6000075c)
                if storage_complete:
                    assert data[104:1304]==bytes(1200),'remaining page initialized bytes differ'
                    assert struct.unpack_from('<4I',data,1304)==(writer_head+3340,writer_head+3340,1,1),'storage header/count/index differ'
                    struct.pack_into('<II',data,1304,0x60000d0c,0x60000d0c)
                else:
                    assert data[104:1320]==bytes(1216),'remaining page/storage initialized bytes differ'
            else:
                assert data[96:1320]==bytes(1224),'new page/storage initial bytes differ'
        if list_constructed:
            assert struct.unpack_from('<6I',data,1320)==(1,0,0,0,0,writer_head+1344)
            assert struct.unpack_from('<I',data,1344)[0]==writer_head+1344
            assert data[1348:1852]==bytes(504),'new chunk tail differs from allocator zeroing'
            struct.pack_into('<II',data,1340,0x60000540,0x60000540)
        if symbols_complete:
            assert struct.unpack_from('<3I',data,1872)==(5,0,0)
        if source_pending:
            assert struct.unpack_from('<5I',data,1852)==(1,0,writer_head+1864,writer_head+40,0)
            struct.pack_into('<II',data,1860,0x60000748,0x60000028)
        result['writer'] = dict(offset=8, cursor_offset=expected_cursor, remaining=expected_remaining,
            source_offset=40, source='numeric-control-flow.c', format=1,
            machine=0x14c, pending04=0xffffffff, debug=0,
            file_global_alias=True, output_open=1, callback_identities=targets,
            incremental_data_null=True, output_sections_null=not symbols_complete, pool_bytes=data.hex())
    if coff:
        assert writer
        expected = dict(output_allocate=addresses['writer_allocator'],
            output_error_handler=addresses['writer_error'], output_clock_handler=addresses['writer_clock'],
            coff_new_output=1, coff_machine=0x14c, coff_source_name=writer_head+40,
            coff_output=output, coff_pending04=0xffffffff, coff_debug=0, coff_pending_c90=1, function_option_23=1, coff_reuse_index=0xffffffff,coff_section_count=1 if storage_complete else 0)
        if section_selected:expected.update(coff_flags=1,coff_current_section=writer_head+1884,coff_current_list=writer_head+1968)
        if initialized_output:
            allocation=u32(addresses['coff_symbol_buffer'])
            assert allocation and all(not start<=allocation<start+4096 for start in (head,directive_head,writer_head))
            expected.update(coff_table=writer_head+64,coff_storage=writer_head+1304,coff_symbol_buffer=allocation,coff_symbol_capacity=1023)
        if list_constructed:expected['coff_output_list']=writer_head+1320
        if symbols_complete:expected.update(coff_symbol_total=5 if section_symbols_complete else 3,coff_next_symbol=5 if section_symbols_complete else 3,coff_previous_source=writer_head+1864)
        copied = {}
        for key, (_, size) in COFF_GLOBALS.items():
            actual = int.from_bytes(read(addresses[key],size),'little')
            assert actual == expected.get(key,0), 'COFF copied/default global differs: '+key
            copied[key] = actual
        for key, normalized in (('output_allocate',0x50000001),
                ('output_error_handler',0x50000002),('output_clock_handler',0x50000003),
                ('coff_source_name',0x60000028),('coff_output',0x40000006)):
            copied[key] = normalized
        if section_selected:copied.update(coff_current_section=0x6000075c,coff_current_list=0x600007b0)
        if initialized_output:
            copied.update(coff_table=0x60000040,coff_storage=0x60000518,coff_symbol_buffer=0x70000001)
            result['coff_allocations']={'table_offset':64,'table_bytes':32,'page_offset':96,'page_bytes':1208,'storage_offset':1304,'storage_bytes':16,'symbol_buffer':{'kind':'malloc','requested_bytes':4096,'payload':'UNMODELED; unread, no content/default credit'}}
        if list_pending:
            assert read(writer_head+1320,24)==bytes(24),'pending constructor list allocation is not allocator-zeroed'
            result['coff_allocations']['pending_list']={'offset':1320,'bytes':24,'contents':'allocator zeroing only; constructor fields await child return'}
        if list_constructed:
            copied['coff_output_list']=0x60000528
            result['coff_allocations']['list']={'offset':1320,'bytes':24,'buffered':1,'count_only':0,'completed_bytes':0,'used_bytes':0,'write_count':0,'last_offset':1344}
            result['coff_allocations']['chunk']={'offset':1344,'allocation_bytes':508,'next_self_alias':True,'modeled_prefix_bytes':4,'tail':'allocator zeroing only; no payload/layout interpretation'}
        if source_pending:
            result['coff_allocations']['source_record']={'offset':1852,'bytes':12,'kind':1,'unwritten04':'allocator zeroing only','source_offset':1864}
            result['coff_allocations']['source_info']={'offset':1864,'bytes':8,'name_offset':40,'next_source':0}
        if symbols_complete:
            symbol_offsets=[1852,1872,1872]+([3316,3328]if section_symbols_complete else [])
            assert struct.unpack('<%dI'%len(symbol_offsets),read(allocation,len(symbol_offsets)*4))==tuple(writer_head+at for at in symbol_offsets)
            copied['coff_previous_source']=0x60000748
            result['coff_allocations']['auxiliary_record']={'offset':1872,'bytes':12,'kind':5,'payload':'allocator zeroing only; unused for source auxiliary'}
            result['coff_allocations']['symbol_buffer']['initialized_entries']=symbol_offsets
            result['coff_allocations']['symbol_buffer']['payload']='First20B only initialized entries; remaining4076B UNMODELED/unread'if section_symbols_complete else'First12B only initialized entries; remaining4084B UNMODELED/unread'
        assert read(addresses['coff_reuse_sections'],32)==bytes(32),'eight typed reuse slots differ from loader-defined NULL initialization'
        result['coff_reuse_sections']=[None]*8
        if append_pending:
            expected_record=struct.pack('<IIIIHHBBH',0,0,0xa00,0,0,8,0,0,0)
            assert data[1884:1908]==expected_record,'new24B section fields/unwritten allocator bytes differ'
            result['coff_allocations']['pending_section']={'offset':1884,'bytes':24,'data_null':True,'name_null':True,'characteristics':0xa00,'base_offset':0,'index':0,'flags':8,'selection':0,'pending21':'allocator zeroing only; uninterpreted','association':0}
        if append_complete:
            name=u32(head+1892+24)
            expected_record=struct.pack('<IIIIHHBBH',writer_head+1908 if section_symbols_pending else 0,name,0xa00,0,1,11 if storage_complete else 8,0,0,0)
            assert data[1884:1908]==expected_record,'inserted section fields/borrowed name/index differ'
            assert u32(head+1892+70)==1,'actual creator section index DWORD differs'
            name_id,name_text=string(name)
            assert name_text=='.drectve'
            struct.pack_into('<I',data,1888,name_id)
            if section_symbols_pending:
                assert struct.unpack_from('<7I',data,1908)==(writer_head+1908,writer_head+1884,3 if section_symbols_complete else 0,writer_head+1968,writer_head+2500,writer_head+3008,0),'new60B record initialized pointers differ'
                assert data[1936:1956]==bytes(20),'uninterpreted record gap differs from allocator zeroing'
                assert struct.unpack_from('<HH',data,1956)==(1 if storage_complete else 0,0),'data storageindex/flags differ'
                assert data[1960:1968]==bytes(8),'unmodeled record tail differs from allocator zeroing'
                for offset,value in ((1908,0x60000774),(1912,0x6000075c),(1920,0x600007b0),(1924,0x600009c4),(1928,0x60000bc0)):
                    struct.pack_into('<I',data,offset,value)
                assert struct.unpack_from('<6I',data,1968)==(1,0,0,written,writes,writer_head+1992),'new per-section output list differs'
                assert struct.unpack_from('<I',data,1992)[0]==writer_head+1992
                assert data[1996:1996+len(payload)]==payload,'written output chunk payload differs'
                assert data[1996+len(payload):2500]==bytes(504-len(payload)),'per-section unwritten payload/tail differs from allocator zeroing'
                struct.pack_into('<II',data,1988,0x600007c8,0x600007c8)
                for offset,size in ((2500,508),(3008,308)):
                    assert struct.unpack_from('<II',data,offset)==(0,writer_head+offset),'relocation/line block prefix differs'
                    assert data[offset+8:offset+size]==bytes(size-8),'block uninterpreted tail differs from allocator zeroing'
                    struct.pack_into('<I',data,offset+4,0x60000000+offset)
                struct.pack_into('<I',data,1884,0x60000774)
            if section_symbols_complete:
                assert struct.unpack_from('<3I',data,3316)==(2,0,writer_head+1908),'new section COFF record differs'
                assert struct.unpack_from('<3I',data,3328)==(5,0,0),'new section auxiliary record differs'
                struct.pack_into('<I',data,3324,0x60000774)
            if storage_complete:
                assert struct.unpack_from('<II',data,3340)==(0,writer_head+1908),'storage entry next/data alias differs'
                struct.pack_into('<I',data,3344,0x60000774)
            if section_recorded:
                assert struct.unpack_from('<II',data,3348)==(0,head+1892),'new section hash-link next/section pointer differ'
                struct.pack_into('<I',data,3352,0x10000764)
            result['writer']['pool_bytes']=data.hex()
            result['coff_allocations']['inserted_section']={'offset':1884,'bytes':24,'data_null':not section_symbols_pending,'name':name_text,'name_section_alias':True,'characteristics':0xa00,'base_offset':0,'index':1,'flags':11 if storage_complete else 8,'selection':0,'pending21':'allocator zeroing only; uninterpreted','association':0}
            result['coff_allocations']['table_state']={'count':1,'used':1,'capacity':300,'first_offset':96,'last_offset':96,'first_item_offset':1884,'remaining_items':'allocator zeroing only'}
        if section_symbols_pending:
            result['coff_allocations']['inserted_section']['data_offset']=1908
            result['coff_allocations']['section_data']={'offset':1908,'allocation_bytes':60,'modeled_prefix_bytes':52,'next_self_alias':True,'section_offset':1884,'symbol_index':3 if section_symbols_complete else 0,'list_offset':1968,'relocation_offset':2500,'line_offset':3008,'size':0,'storage_index':1 if storage_complete else 0,'flags':0,'gap28_47':'allocator zeroing only; no field interpretation','tail52_59':'allocator zeroing only; no layout credit'}
            result['coff_allocations']['section_list']={'offset':1968,'bytes':24,'buffered':1,'count_only':0,'completed_bytes':0,'used_bytes':written,'write_count':writes,'last_offset':1992}
            result['coff_allocations']['section_chunk']={'offset':1992,'allocation_bytes':508,'next_self_alias':True,'modeled_prefix_bytes':504 if output_bytes_written or output_directives_complete else 4,'payload':payload.hex()if output_bytes_written or output_directives_complete else None,'tail':'unwritten payload/tail allocator zeroing only; last4B no layout interpretation'}
            result['coff_allocations']['section_blocks']=[{'offset':at,'allocation_bytes':size,'modeled_prefix_bytes':8,'count':0,'last_self_alias':True,'tail':'allocator zeroing only; no payload/layout interpretation'}for at,size in ((2500,508),(3008,308))]
        if section_symbols_complete:
            result['coff_allocations']['section_symbol_record']={'offset':3316,'bytes':12,'kind':2,'unwritten04':'allocator zeroing only','data_offset':1908,'symbol_index':3}
            result['coff_allocations']['section_auxiliary_record']={'offset':3328,'bytes':12,'kind':5,'payload':'allocator zeroing only; notyetpopulated'}
        if storage_complete:
            result['coff_allocations']['storage_state']={'offset':1304,'bytes':16,'first_offset':3340,'last_offset':3340,'count':1,'next_index':1}
            result['coff_allocations']['storage_entry']={'offset':3340,'bytes':8,'next_null':True,'data_offset':1908}
        bucket_words=struct.unpack('<16I',read(addresses['coff_section_buckets'],64))
        expected_buckets=tuple(writer_head+3348 if section_recorded and i==7 else 0 for i in range(16))
        assert bucket_words==expected_buckets,'complete16-entry section bucket ownership differs'
        result['coff_section_buckets']=[3348 if section_recorded and i==7 else None for i in range(16)]
        if section_recorded:
            result['coff_allocations']['section_link']={'offset':3348,'bytes':8,'next_null':True,'section_offset':1892,'serial':151,'bucket':7}
        result['coff'] = copied
    return result
