"""Instruction-boundary observers for arguments/results, never function vertices."""

def plan(accepted,case="help"):
    result=[]
    if '_mainCRTStartup' in accepted:
        result += [(0x65320,'observe:initerm_c'),(0x6535b,'observe:initerm_cpp')]
    for suffix,address in [('12',0x424e70),('8',0x431d80),('4',0x431df0)]:
        if '_allocate_buffer'+suffix in accepted:
            result += [(address+5-0x400000,'observe:new_call_'+suffix),(address+10-0x400000,'observe:new_return_'+suffix)]
    for suffix,address in [('12',0x424ea0),('8',0x431db0),('4',0x431e20),('timing',0x444ea0)]:
        name='_register_'+('timing_cleanup' if suffix=='timing' else 'buffer'+suffix+'_cleanup')
        if name in accepted:
            result += [(address+5-0x400000,'observe:atexit_call_'+suffix),(address+10-0x400000,'observe:atexit_return_'+suffix)]
    if '_main' in accepted:
        result += [(0xded2,'observe:console_call'),(0xded8,'observe:console_return')]
    if '_initialize_file_cache@16' in accepted:
        result += [(0xb568,'observe:module_call'),(0xb56e,'observe:module_return'),
                   (0xb578,'observe:proc_call'),(0xb57e,'observe:proc_return')]
    if '_initialize_memory' in accepted:
        result += [(0x4f571,'observe:system_before'),(0x4f577,'observe:system_after'),
                   (0x4f586,'observe:memory_before'),(0x4f58c,'observe:memory_after')]
    if '_parse_command_line@16' in accepted:result.append((0xde93,'observe:main_frame'))
    if '_is_libpath@4' in accepted:result += [(0x5c76b,'observe:mbs_call'),(0x5c771,'observe:mbs_return')]
    if '_create_memory_arena@4' in accepted:result += [(0x4f659,'observe:virtual_call'),(0x4f65f,'observe:virtual_return')]
    if '_parse_switch@8' in accepted:result += [(0xd149,'observe:parser_alloc_call'),(0xd14e,'observe:parser_alloc_return')]
    if '_duplicate_argument@4' in accepted:result.append((0x451ab,'observe:duplicate_return'))
    if '_duplicate_link_string@4' in accepted:
        result.extend([(0xe3f3,'observe:link_name_module_call'),(0xe3f8,'observe:link_name_module_return'),(0xe400,'observe:link_name_library_call'),(0xe405,'observe:link_name_library_return')])
    if '_allocate_virtual_memory@16'in accepted:
        result.extend([(0x4f5c5,'observe:pool_virtual_call'),(0x4f5cb,'observe:pool_virtual_return'),(0xe36e,'observe:section_alloc_call_0'),(0xe373,'observe:section_alloc_return_0'),(0xe386,'observe:section_alloc_call_1'),(0xe38b,'observe:section_alloc_return_1')])
    if '_adjust_output_section_flags@12'in accepted:
        result.extend([(0xf15a,'observe:section_flags_call_lookup'),(0xf15f,'observe:section_flags_return_lookup'),(0xefbc,'observe:section_flags_call_create'),(0xefc1,'observe:section_flags_return_create')])
    if '_initialize_section_enumerator@8'in accepted:
        result.extend([(0xf16e,'observe:section_enum_call'),(0xf173,'observe:section_enum_return')])
    if '_next_output_section@4'in accepted:
        result.extend([(0xf178,'observe:section_next_call'),(0xf17d,'observe:section_next_return'),(0xf1c8,'observe:section_next_continue_call'),(0xf1cd,'observe:section_next_continue_return')])
    if '_end_section_enumerator'in accepted:
        result.extend([(0xf1d6,'observe:section_end_call'),(0xf1db,'observe:section_end_return')])
    if '_resolve_output_section@4'in accepted:
        result.extend([(0xefac,'observe:section_lookup_call'),(0xefb1,'observe:section_lookup_return'),(0xefe2,'observe:section_name_call'),(0xefe7,'observe:section_name_return'),(0xf219,'observe:section_resolve_call'),(0xf21e,'observe:section_resolve_return')])
    if '_find_contribution_group@8'in accepted:
        result.extend([(0xe67e,'observe:group_resolve_call'),(0xe683,'observe:group_resolve_return')])
    if '_initialize_group_enumerator@8'in accepted:
        result.extend([(0xe689,'observe:group_enum_call'),(0xe68e,'observe:group_enum_return')])
    if '_next_contribution_group@4'in accepted:
        result.extend([(0xe70c,'observe:group_find_call'),(0xe711,'observe:group_find_return'),(0xe73c,'observe:group_name_call'),(0xe741,'observe:group_name_return'),(0xe693,'observe:group_next_call'),(0xe698,'observe:group_next_return'),(0xe6da,'observe:group_next_continue_call'),(0xe6df,'observe:group_next_continue_return'),(0xe6ec,'observe:group_end_call'),(0xe6f1,'observe:group_end_return'),(0x42fa1,'observe:sections_initialize_call'),(0x42fa6,'observe:sections_initialize_return')])
    if '_check_output_input_collision@8'in accepted:
        result.extend([(0x4b8cb,'observe:collision_call'),(0x4b8d0,'observe:collision_return'),
                       (0x61862,'observe:collision_fullpath_output_call'),(0x61864,'observe:collision_fullpath_output_return'),
                       (0x6189d,'observe:collision_fullpath_input_call'),(0x6189f,'observe:collision_fullpath_input_return'),
                       (0x618c4,'observe:collision_compare_call'),(0x618c6,'observe:collision_compare_return')])
    if '_find_cached_file@4'in accepted:
        result.extend([(0xb878,'observe:file_lookup_call'),(0xb87d,'observe:file_lookup_return')])
    if '_move_file_record@24'in accepted:
        result.extend([(0xb951,'observe:file_move_call'),(0xb956,'observe:file_move_return'),
                       (0xb885,'observe:file_available_call'),(0xb88a,'observe:file_available_return'),
                       (0xb829,'observe:file_acquire_call'),(0xb82e,'observe:file_acquire_return'),
                       (0xb898,'observe:file_name_call'),(0xb89d,'observe:file_name_return')])
    if '_initialize_library_enumerator@8'in accepted:result.append((0xf674,'observe:library_enum_return'))
    if '_next_library@4'in accepted:result.extend([(0xf67e,'observe:library_next_return'),(0xede7,'observe:library_find_return'),(0x4b906,'observe:library_create_return'),(0xee4b,'observe:library_array_call'),(0xee50,'observe:library_array_return')])
    if 'LinkContext::LoadInputArgument'in accepted:result.extend([(0x49a88,'observe:input_open_call'),(0x49a8d,'observe:input_open_return'),(0x4b94f,'observe:load_input_call'),(0x4b954,'observe:load_input_return')])
    if '_mapped_read@12'in accepted:result.extend([(0x5cd75,'observe:archive_read_call'),(0x5cd7a,'observe:archive_read_return'),(0x498e0,'observe:archive_predicate_call'),(0x498e5,'observe:archive_predicate_return')])
    if '_seek_mapped_file@16'in accepted:
        result.extend([(0xc0a8,'observe:seek_current_call'),(0xc0ad,'observe:seek_current_return'),(0xc0c1,'observe:seek_end_call'),(0xc0c6,'observe:seek_end_return'),(0xc0dd,'observe:seek_restore_call'),(0xc0e2,'observe:seek_restore_return'),(0x4e653,'observe:seek_reset_call'),(0x4e658,'observe:seek_reset_return'),(0xc061,'observe:mapped_seek_call'),(0xc066,'observe:mapped_seek_return'),(0x4e629,'observe:file_length_call'),(0x4e62e,'observe:file_length_return')])
    if '_read_big_endian_dword@4'in accepted:
        result.extend([(0x4e5ea,'observe:big_endian_read_call'),(0x4e5ef,'observe:big_endian_read_return')])
    if '_byte_swap_dword@4'in accepted:
        result.extend([(0x4e5f9,'observe:byte_swap_return'),(0x4e65e,'observe:big_endian_return'),(0x4994e,'observe:mac_resource_return')])
    if '_close_timing' in accepted:result.append((0x44eb6,'observe:timing_end'))
    for suffix,address in [('4',0x431e30),('8',0x431dc0),('12',0x424eb0)]:
        if '_free_buffer'+suffix in accepted:result += [(address+10-0x400000,'observe:delete_call_'+suffix),(address+15-0x400000,'observe:delete_return_'+suffix)]
    if case!="help" and "_parse_argument@8" in accepted:
        result += [(0x5c7ab,"observe:mbspbrk_call"),(0x5c7b1,"observe:mbspbrk_return")]
    if case!="help" and "_canonicalize_argument@8" in accepted:
        result.append((0x5c7db,"observe:canonicalize_return"))
    if case!="help" and "_search_library_paths@8" in accepted:
        result += [(0x5c6f2,"observe:library_search_call"),(0x5c6f7,"observe:library_search_return"),
                   (0x5c706,"observe:searchenv_call"),(0x5c70c,"observe:searchenv_return")]
    if case!="help" and "_release_memory@4" in accepted:
        result += [(0x4fe35,"observe:free_call"),(0x4fe3b,"observe:free_return")]
    if '_open_mapped_file@16'in accepted:
        result.extend([(0xc914,'observe:mapped_create_call'),(0xc91a,'observe:mapped_create_return')])
    if '_initialize_file_mapping@12'in accepted:
        result.extend([(0xb845,'observe:mapped_open_call'),(0xc9ca,'observe:mapping_function_call'),(0xc9cf,'observe:mapping_function_return'),(0xb84a,'observe:mapped_open_return'),(0x4b8e5,'observe:output_open_return'),(0xcd44,'observe:mapping_create_call'),(0xcd4a,'observe:mapping_create_return'),(0xcd95,'observe:mapping_view_call'),(0xcd9b,'observe:mapping_view_return'),(0xcdc9,'observe:mapping_close_call'),(0xcdcf,'observe:mapping_close_return')])
    return result
