import gdb,json
from pathlib import Path
if exit_status is not None:
    assert entry_input
    info=dict(entry_input,completed=True,exit_status=exit_status,trace=trace)
    Path('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/filename-record-native/state.json').write_text(json.dumps(info,indent=2)+'\n')
else:
    import gdb,json
    from pathlib import Path
    frame=gdb.newest_frame()
    info={'function':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'signal':gdb.execute('info program',to_string=True),'backtrace':gdb.execute('bt',to_string=True),'trace':trace,'globals':{}}
    pid=gdb.selected_inferior().pid
    actual_args=Path('/proc/%d/cmdline'%pid).read_bytes().rstrip(b'\0').split(b'\0')
    actual_env=Path('/proc/%d/environ'%pid).read_bytes().split(b'\0')
    Path('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/filename-record-native/native-maps.txt').write_text(Path('/proc/%d/maps'%pid).read_text())
    gdb.execute('info registers eip')
    gdb.execute('info symbol $pc')
    gdb.execute('x/1ib $pc-1')
    gdb.execute('info program')
    info['arguments']=[v.decode()for v in actual_args[1:]]
    flags=[v[14:].decode()for v in actual_env if v.startswith(b'MSC_CMD_FLAGS=')]
    assert not flags,'host compiler flags leaked'
    info['MSC_CMD_FLAGS']=entry_input['MSC_CMD_FLAGS']
    records=[r for r in [{'name': '_c2_math_error', 'native': 'c2_math_error', 'start': 4484380, 'end': 4484383, 'abi': 'cdecl, one pointer parameter', 'kind': 'crt-source'}, {'name': '_c2_print_banner', 'native': 'c2_print_banner', 'start': 4738355, 'end': 4738356, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_registers', 'native': 'c2_initialize_registers', 'start': 4535034, 'end': 4535035, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_optimizer', 'native': 'c2_initialize_optimizer', 'start': 4535045, 'end': 4535046, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_compile_unit', 'native': 'c2_compile_unit', 'start': 4538855, 'end': 4538856, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_finalize_output', 'native': 'c2_finalize_output', 'start': 4541174, 'end': 4541175, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_flush_debug', 'native': 'c2_flush_debug', 'start': 4766214, 'end': 4766215, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_console_handler@4', 'native': 'c2_console_handler', 'start': 4737491, 'end': 4737492, 'pending_extent': True, 'kind': 'application', 'abi': 'stdcall, DWORD control event; RET4'}, {'name': '@c2_report_internal@8', 'native': 'c2_report_internal', 'start': 4746044, 'end': 4746068, 'pending_extent': False, 'kind': 'application', 'abi': 'fastcall, ECX source path EDX line', 'extent_known_for_tail_abi': True, 'extent_evidence': 'cases/analysis/node-size-disassembly.txt: complete486b3c..486b54 reports fatal internal source/line then RET0; still application INT3/no credit'}, {'name': '@c2_system_fatal@12', 'native': 'c2_system_fatal', 'start': 4765320, 'end': 4765358, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX message EDX prefix; stack filename; RET4'}, {'name': '@resolve_diagnostic_location@8', 'native': 'resolve_diagnostic_location', 'start': 4413704, 'end': 4413705, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall ECX location EDX output filename'}, {'name': '@c2_open_pdb@8', 'native': 'c2_open_pdb', 'start': 4765792, 'end': 4765793, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall ECX PDB filename EDX object filename; int result; no stack parameters'}, {'name': '@function_retains_symbols@8', 'native': 'function_retains_symbols', 'start': 4347234, 'end': 4347235, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall functionECX conditionalEDX; int result RET0'}, {'name': '@lookup_symbol@8', 'native': 'lookup_symbol', 'start': 4350333, 'end': 4350334, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall scopeECX serialEDX; symbol pointerEAX RET0'}, {'name': '@put_symbol@8', 'native': 'put_symbol', 'start': 4359656, 'end': 4359657, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall scopeECX symbolEDX; pointer result provisional/unused by reachedcaller; RET0'}, {'name': '@queue_symbol_debug@4', 'native': 'queue_symbol_debug', 'start': 4325189, 'end': 4325190, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall symbolECX; RET0'}, {'name': '@update_symbol_storage@4', 'native': 'update_symbol_storage', 'start': 4359139, 'end': 4359140, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall symbolECX; result ignored by reachedcaller; RET0'}, {'name': '@reserve_symbol_storage@4', 'native': 'reserve_symbol_storage', 'start': 4251079, 'end': 4251080, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall symbolECX; scalar result provisional/unused by reachedcaller; RET0'}, {'name': '_input_compact_signed', 'native': 'input_compact_signed', 'start': 4325621, 'end': 4325622, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl no parameters, signed longEAX RET0'}, {'name': '_input_compact_short', 'native': 'input_compact_short', 'start': 4325829, 'end': 4325830, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl no parameters, signed shortAX RET0'}, {'name': '_input_extended_flags', 'native': 'input_extended_flags', 'start': 4763693, 'end': 4763694, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl no parameters, flagsEAX RET0'}, {'name': '@copy_symbol_type@4', 'native': 'copy_symbol_type', 'start': 4359092, 'end': 4359093, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall symbolECX; result ignored by decoder; RET0', 'state_policy': 'symbol-type-input', 'state': [{'original': '0x499708', 'native': '(unsigned long)c2_onexit_end'}, {'original': '0x4996d4', 'native': '(unsigned long)c2_onexit_begin'}, {'original': '0x49975c', 'native': 'c2_fmode'}, {'original': '0x4980d4', 'native': 'c2_commode'}, {'original': '0x499718', 'native': 'c2_adjust_fdiv'}, {'original': '0x499694', 'native': 'c2_default_math_error'}, {'original': '0x4980d0', 'native': 'c2_new_mode'}, {'original': '0x4980cc', 'native': 'c2_wildcards'}, {'original': '0x7803fa88', 'native': 'recon_fmode'}, {'original': '0x7803fa94', 'native': 'recon_commode'}, {'original': '0x7803f928', 'native': 'recon_adjust_fdiv'}, {'original': '0x7803fa90', 'native': 'recon_crt_app_type'}, {'original': '0x7803f198', 'native': 'recon_crt_newmode'}, {'original': '0x490b7c', 'native': 'c2_il_extended_header'}, {'original': '0x4972f8', 'native': 'c2_il_source_table'}, {'original': '0x497df8', 'native': 'c2_il_symbol_count'}, {'original': '0x491050', 'native': 'c2_il_initial_symbol_count'}, {'original': '0x497f6c', 'native': 'c2_il_target_size'}, {'original': '0x497de8', 'native': 'c2_il_eight_byte_target'}, {'original': '0x48fb68', 'native': '(unsigned long)global_scope.previous'}, {'original': '0x491040', 'native': 'c2_node_bytes[0]'}, {'original': '0x491044', 'native': 'c2_node_bytes[1]'}, {'original': '0x491048', 'native': 'c2_node_bytes[2]'}, {'original': '0x49104c', 'native': 'c2_node_bytes[3]'}, {'original': '0x491074', 'native': 'selected_save_register_mask'}, {'original': '0x495f34', 'native': 'call_clobbered_register_mask'}, {'original': '0x49745c', 'native': 'stack_alignment'}, {'original': '0x4973e8', 'native': '(unsigned long)current_debug_scope'}, {'original': '0x490b84', 'native': '(unsigned long)current_symbol_list_scope'}, {'original': '0x4973d4', 'native': '(unsigned long)pending_functions'}, {'original': '0x4973f0', 'native': '(unsigned long)pending_insertion'}, {'original': '0x4972fc', 'native': '(unsigned long)global_special_symbols'}, {'original': '0x49d634', 'native': 'current_source_file'}, {'original': '0x49739c', 'native': 'last_section_serial'}, {'original': '0x4973b0', 'native': 'il_option_flags'}, {'original': '0x4973a0', 'native': '(unsigned long)auxiliary_prefix'}, {'original': '0x497390', 'native': '(unsigned long)aux_expression_file'}, {'original': '0x497380', 'native': '(unsigned long)aux_symbol_file'}, {'original': '0x497c6c', 'native': 'c2_constant_strings'}, {'original': '0x4973c0', 'native': 'c2_omit_frame_pointer'}, {'original': '0x495f1c', 'native': '(unsigned long)current_symbol'}], 'expected_argc': 0, 'expected_argv': ['-G5']}, {'name': '@lookup_global_symbol@4', 'native': 'lookup_global_symbol', 'start': 4322387, 'end': 4322388, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall serialECX; symbol*EAX RET0'}, {'name': '@clone_alias_symbol@4', 'native': 'clone_alias_symbol', 'start': 4553883, 'end': 4553884, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall symbolECX; result ignored by decoder; RET0'}, {'name': '@read_qualified_symbol@8', 'native': 'read_qualified_symbol', 'start': 4745531, 'end': 4745532, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall symbolECX poolEDX; result ignored by decoder; RET0'}, {'name': '_compiler_warning', 'native': 'compiler_warning', 'start': 4508034, 'end': 4508035, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl level,code,varargs; caller cleanup20 at reached callsite'}, {'name': '@read_module_directive@4', 'native': 'read_module_directive', 'start': 4562073, 'end': 4562074, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall signed kindECX; result ignored by decoder; RET0'}, {'name': '@register_source_file@4', 'native': 'register_source_file', 'start': 4766336, 'end': 4766337, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall char*nameECX; tested intEAX RET0'}, {'name': '@lookup_source_file@8', 'native': 'lookup_source_file', 'start': 4766384, 'end': 4766385, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall nameECX unsigned*bytesEDX; char*EAX RET0'}, {'name': '@uppercase_symbol_name@4', 'native': 'uppercase_symbol_name', 'start': 4745583, 'end': 4745584, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall mutable char*nameECX; void/result ignored by caller; RET0'}] if r['native']==frame.name()]
    if len(records)==1:
        for view in records[0].get('state',[]):
            info['globals'][view['original']]=int(gdb.parse_and_eval(view['native'])) & 0xffffffff
        if records[0].get('state_policy')in ('command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            ptr=gdb.parse_and_eval('c2_argv');values=[];i=0
            while int(ptr[i]):
                values.append(ptr[i].string());i+=1
                assert i<4096
            info['command_state']={'argc':int(gdb.parse_and_eval('c2_argc')),'argv':values}
            if records[0]['state_policy']=='environment-flags':
                info['flags']=None if int(gdb.parse_and_eval('flags'))==0 else gdb.parse_and_eval('flags').string()
        if records[0].get('state_policy')in ('alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            vector=gdb.parse_and_eval('recon_crt_environment');index=0;origin=None
            while int(vector[index]):
                entry=vector[index];prefix=gdb.selected_inferior().read_memory(entry,14).tobytes()
                if prefix==b'MSC_CMD_FLAGS=':origin=int(entry)+14;break
                index+=1
            assert origin is not None
            alignment=gdb.parse_and_eval('alignment'if records[0]['state_policy']=='alignment-option'else'c2_alignment_options');current=gdb.parse_and_eval('c2_current_option');operand=gdb.parse_and_eval('c2_option_operand')
            array=gdb.parse_and_eval('environment_arguments');argv=gdb.parse_and_eval('c2_argv')
            info['alignment_option']={'alignment':None if not int(alignment)else alignment.string(),'current':current.string(),
                                      'current_offset':int(current)-origin,'operand':operand.string(),'operand_offset':int(operand)-origin,
                                      'argv_offset':int(argv)-int(array.address),'argv_value_alias':int(argv[0])==int(current)}
        if records[0].get('state_policy')in ('symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            import hashlib
            table=gdb.parse_and_eval('global_scope.buckets')
            data=gdb.selected_inferior().read_memory(table.address,4096).tobytes()
            info['symbol_hash']={'slots':int(gdb.parse_and_eval('sizeof(global_scope.buckets)/sizeof(global_scope.buckets[0])')),'sha256':hashlib.sha256(data).hexdigest()}
            if records[0]['state_policy']in ('section-linked','section-defaults'):
                section=gdb.parse_and_eval('section');values=[int(table[i])for i in range(1024)]
                info['symbol_hash']['section_slot']=5
                info['symbol_hash']['section_alias']=values[5]==int(section)
                altered=bytearray(data);altered[20:24]=bytes(4)
                info['symbol_hash']['normalized_sha256']=hashlib.sha256(altered).hexdigest()
                info['symbol_hash'].pop('sha256')

        if records[0].get('state_policy')=='symbol-type-input':
            symbol=gdb.parse_and_eval('symbol');head=int(gdb.parse_and_eval('c2_pools[0].blocks'));file=gdb.parse_and_eval('filenames');caller=frame.older()
            template=bytearray(gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes());template[24:28]=bytes(4)
            info['symbol_type_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
                'symbol_offset':int(symbol)-head,'caller_symbol_alias':int(symbol)==int(caller.read_var('symbol')),
                'caller_kind':int(caller.read_var('kind')),'retain_locals':int(caller.read_var('retain_locals')),
                'symbol_bytes':gdb.selected_inferior().read_memory(symbol,110).tobytes().hex(),
                'filename_offset':int(file)-head,'filename_serial':int(file['serial']),
                'filename_name_offset':int(file['name'])-head,'filename_next_null':not int(file['next']),
                'filename_name_alias':int(file['name'])==int(gdb.parse_and_eval('symbol_template.symbol.name')),
                'filename_size':int(gdb.parse_and_eval('sizeof(C2Filename)')),
                'template_bytes':template.hex(),
                'input_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
                'decorated_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&decorated_name'),2059).tobytes().hex()}
            gdb.write('C2-SYMBOL-TYPE-INPUT '+json.dumps(info['symbol_type_input'],sort_keys=True)+'\n')
        if records[0].get('state_policy')=='filename-record':
            symbol=gdb.parse_and_eval('symbol');head=int(gdb.parse_and_eval('c2_pools[0].blocks'))
            template=bytearray(gdb.selected_inferior().read_memory(symbol,64).tobytes());template[24:28]=bytes(4)
            info['filename_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
                'symbol_alias':int(symbol)==int(gdb.parse_and_eval('&symbol_template.symbol')),
                'name_pool_offset':int(symbol['name'])-head,'name':symbol['name'].string(),
                'template_bytes':template.hex(),
                'input_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
                'decorated_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&decorated_name'),2059).tobytes().hex(),
                'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
            gdb.write('C2-FILENAME-INPUT '+json.dumps(info['filename_input'],sort_keys=True)+'\n')
        if records[0].get('state_policy')=='symbol-name-copy':
            caller=frame.older();name=gdb.parse_and_eval('name')
            info['symbol_copy_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
                'name_alias':int(name)==int(gdb.parse_and_eval('&decorated_name[1]')),
                'pool':int(gdb.parse_and_eval('pool')),'caller_pool':int(caller.read_var('pool')),
                'caller_symbol_alias':int(caller.read_var('symbol'))==int(gdb.parse_and_eval('&symbol_template.symbol')),
                'template_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes().hex(),
                'input_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
                'decorated_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&decorated_name'),2059).tobytes().hex(),
                'workspace_size':int(gdb.parse_and_eval('sizeof(decorated_name)')),
                'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
            gdb.write('C2-SYMBOL-COPY-INPUT '+json.dumps(info['symbol_copy_input'],sort_keys=True)+'\n')
        if records[0].get('state_policy')=='symbol-name-intern':
            caller=frame.older();symbol=gdb.parse_and_eval('symbol');name=gdb.parse_and_eval('name')
            info['symbol_intern_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
                'symbol_alias':int(symbol)==int(gdb.parse_and_eval('&symbol_template.symbol')),
                'name_alias':int(name)==int(gdb.parse_and_eval('&symbol_name[0]')),
                'pool':int(gdb.parse_and_eval('pool')),'kind':int(caller.read_var('kind')),
                'retain_locals':int(caller.read_var('retain_locals')),
                'template_bytes':gdb.selected_inferior().read_memory(symbol,64).tobytes().hex(),
                'name_bytes':gdb.selected_inferior().read_memory(name,2058).tobytes().hex(),
                'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
            gdb.write('C2-SYMBOL-INTERN-INPUT '+json.dumps(info['symbol_intern_input'],sort_keys=True)+'\n')
        if records[0].get('state_policy')=='symbol-name-read':
            caller=frame.older();buffer=gdb.parse_and_eval('buffer')
            info['symbol_name_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
                'buffer_alias':int(buffer)==int(gdb.parse_and_eval('&symbol_name[0]')),
                'capacity':int(gdb.parse_and_eval('capacity')),'kind':int(caller.read_var('kind')),
                'retain_locals':int(caller.read_var('retain_locals')),
                'template_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes().hex(),
                'name_bytes':gdb.selected_inferior().read_memory(buffer,2058).tobytes().hex(),
                'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
            gdb.write('C2-SYMBOL-NAME-INPUT '+json.dumps(info['symbol_name_input'],sort_keys=True)+'\n')
        if records[0].get('state_policy')=='symbol-serial-read':
            caller=frame.older()
            arguments=[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument]
            info['symbol_serial_input']={'formals':arguments,'kind':int(caller.read_var('kind')),
                'retain_locals':int(caller.read_var('retain_locals')),
                'template_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes().hex(),
                'name_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
                'source_template_size':int(gdb.parse_and_eval('sizeof(symbol_template)')),
                'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
            gdb.write('C2-SYMBOL-SERIAL-INPUT '+json.dumps(info['symbol_serial_input'],sort_keys=True)+'\n')
        if records[0].get('state_policy')=='symbol-record':
            caller=frame.older();table=caller.read_var('table')
            info['symbol_record_input']={'retain_locals':int(gdb.parse_and_eval('retain_locals')),
                'caller_null_scope':not int(caller.read_var('scope')),'caller_retention':int(caller.read_var('retain_locals')),
                'global_scope_alias':int(table)==int(gdb.parse_and_eval('&global_scope')),
                'scope_bytes':int(gdb.parse_and_eval('sizeof(C2ScopeHash)')),
                'bucket_offset':int(gdb.parse_and_eval('&global_scope.buckets'))-int(gdb.parse_and_eval('&global_scope'))}
        if records[0].get('state_policy')=='global-symbols':
            info['global_symbols_input']={'null_scope':not int(gdb.parse_and_eval('scope')),'active_globals_alias':int(gdb.parse_and_eval('c2_current_il'))==int(gdb.parse_and_eval('c2_globals_input'))}
        if records[0].get('state_policy')in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            import hashlib
            table=gdb.parse_and_eval('global_scope.buckets');head=int(gdb.parse_and_eval('c2_pools[0].blocks'))
            nonnull={i:int(table[i])-head for i in range(1024)if int(table[i])}
            info['symbol_hash']={'slots':1024,'section_offsets':nonnull}
        if records[0].get('state_policy')=='allocate-node':
            info['allocate_node']={'kind':int(gdb.parse_and_eval('kind')),'pool':int(gdb.parse_and_eval('pool'))}
        if records[0].get('state_policy')=='node-size':
            caller=frame.older()
            info['node_size']={'kind':int(gdb.parse_and_eval('kind')),'caller_kind':int(caller.read_var('kind')),'pool':int(caller.read_var('pool'))}
        if records[0].get('state_policy')=='pool-allocate':
            caller=frame.older()
            info['pool_allocate']={'pool':int(gdb.parse_and_eval('pool')),'bytes':int(gdb.parse_and_eval('bytes')),
                                   'caller_kind':int(caller.read_var('kind')),'caller_pool':int(caller.read_var('pool'))}
        if records[0].get('state_policy')in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            head=gdb.parse_and_eval('section_head');section=head;rows=[];addresses=[]
            while int(section):
                assert len(rows)<5
                raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes())
                name=int(section['symbol']['name']);class_name=int(section['class_name']);context=int(section['symbol']['association']['context']);nxt=int(section['next'])
                for at in (24,36,50,54):raw[at:at+4]=bytes(4)
                rows.append({'bytes':raw.hex(),'name':section['symbol']['name'].string(),'class_name':section['class_name'].string(),
                    'name_pointer':name,'class_pointer':class_name,'context_alias':context==int(gdb.parse_and_eval('section_context')),
                    'pool_offset':int(section)-int(gdb.parse_and_eval('c2_pools[0].blocks'))})
                addresses.append(int(section));section=section['next']
            assert len(rows)==5
            info['sections']={'rows':rows,'tail_alias':int(gdb.parse_and_eval('section_tail'))==addresses[-1],
                'outputs':[addresses.index(int(gdb.parse_and_eval(name)))if int(gdb.parse_and_eval(name))in addresses else None for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')],
                'output_nulls':[not int(gdb.parse_and_eval(name))for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')],
                'context_identity_alias':int(gdb.parse_and_eval('section_context'))==int(gdb.parse_and_eval('&section_context_identity')),
                'default_alignment':int(gdb.parse_and_eval('default_section_alignment'))}
        if records[0].get('state_policy')=='section-symbol':
            symbol=gdb.parse_and_eval('symbol');section=symbol.cast(gdb.lookup_type('C2Section').pointer())
            raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes());raw[24:28]=bytes(4);raw[54:58]=bytes(4)
            info['section_symbol']={'local':int(gdb.parse_and_eval('local')),'bytes':raw.hex(),
                'name':symbol['name'].string(),'class_name':section['class_name'].string(),
                'name_alias':int(symbol['name'])==int(gdb.parse_and_eval('code_section_name')),
                'pool_offset':int(symbol)-int(gdb.parse_and_eval('c2_pools[0].blocks')),
                'initial_sections':[int(gdb.parse_and_eval(name))for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')]}
        if records[0].get('state_policy')=='section-linked':
            section=gdb.parse_and_eval('section');symbol=section.cast(gdb.lookup_type('C2Symbol').pointer())
            raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes());raw[24:28]=bytes(4);raw[54:58]=bytes(4)
            info['section_linked']={'bytes':raw.hex(),
                'name':symbol['name'].string(),'class_name':section['class_name'].string(),
                'name_alias':int(symbol['name'])==int(gdb.parse_and_eval('code_section_name')),
                'pool_offset':int(symbol)-int(gdb.parse_and_eval('c2_pools[0].blocks')),
                'initial_sections':[int(gdb.parse_and_eval(name))for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')]}
        if records[0].get('state_policy')=='section-defaults':
            section=gdb.parse_and_eval('section');symbol=section.cast(gdb.lookup_type('C2Symbol').pointer())
            raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes());raw[24:28]=bytes(4);raw[54:58]=bytes(4)
            info['section_defaults']={'bytes':raw.hex(),
                'name':symbol['name'].string(),'class_name':section['class_name'].string(),
                'name_alias':int(symbol['name'])==int(gdb.parse_and_eval('code_section_name')),
                'pool_offset':int(symbol)-int(gdb.parse_and_eval('c2_pools[0].blocks')),
                'code_section_alias':int(gdb.parse_and_eval('code_section'))==int(section),
                'other_sections':[int(gdb.parse_and_eval(name))for name in ('data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')],
                'head_alias':int(gdb.parse_and_eval('section_head'))==int(section),'tail_alias':int(gdb.parse_and_eval('section_tail'))==int(section),
                'context_identity_alias':int(gdb.parse_and_eval('section_context'))==int(gdb.parse_and_eval('&section_context_identity'))}
        if records[0].get('state_policy')=='section-constructor':
            name=gdb.parse_and_eval('name');class_name=gdb.parse_and_eval('class_name')
            info['section_constructor']={'name':name.string(),'class_name':class_name.string(),'serial':int(gdb.parse_and_eval('serial')),
                'name_alias':int(name)==int(gdb.parse_and_eval('code_section_name')),
                'initial_sections':[int(gdb.parse_and_eval(item))for item in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')]}
        if records[0].get('state_policy')=='real-from-int':
            source=gdb.parse_and_eval('input');destination=gdb.parse_and_eval('output')
            info['real_from_int']={'value':int(source.dereference()),'output_is_zero':int(destination)==int(gdb.parse_and_eval('&c2_real_zero')),
                'caller_local_alias':int(source)==int(frame.older().read_var('value').address),
                'records':[gdb.selected_inferior().read_memory(gdb.parse_and_eval('&'+name),12).tobytes().hex()for name in ('c2_real_zero','c2_real_one','c2_real_two')]}
        if records[0].get('state_policy')=='extended-store':
            destination=int(gdb.parse_and_eval('$esi'))
            info['extended_store']={'destination_is_zero':destination==int(gdb.parse_and_eval('&c2_real_zero')),
                'destination':gdb.selected_inferior().read_memory(destination,12).tobytes().hex(),
                'control':int(gdb.parse_and_eval('$fctrl')),'status':int(gdb.parse_and_eval('$fstat')),'tag':int(gdb.parse_and_eval('$ftag')),
                'instruction':int(gdb.parse_and_eval('$fioff'))&0xffffffff,'data_pointer':int(gdb.parse_and_eval('$fooff'))&0xffffffff,
                'source_alias':(int(gdb.parse_and_eval('$fooff'))&0xffffffff)==int(frame.older().read_var('input')),
                'pointer':destination,'entry':store_abi_entry,
                'registers':[gdb.selected_frame().read_register('st%d'%i).bytes.hex()for i in range(8)]}
            gdb.write('C2-STORE-ABI-STOP '+json.dumps(info['extended_store'],sort_keys=True)+'\n')
        if records[0].get('state_policy')=='extended-results':
            info['extended_results']={'control':int(gdb.parse_and_eval('$fctrl')),'status':int(gdb.parse_and_eval('$fstat')),'tag':int(gdb.parse_and_eval('$ftag')),
                'records':[gdb.selected_inferior().read_memory(gdb.parse_and_eval('&'+name),12).tobytes().hex()for name in ('c2_real_zero','c2_real_one','c2_real_two')],
                'entries':store_abi_entries,'steps':store_steps}
            gdb.write('C2-EXTENDED-RESULTS '+json.dumps(info['extended_results'],sort_keys=True)+'\n')
        if records[0].get('state_policy')in ('intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            info['real_results']={'control':int(gdb.parse_and_eval('$fctrl')),'status':int(gdb.parse_and_eval('$fstat')),'tag':int(gdb.parse_and_eval('$ftag')),
                'records':[gdb.selected_inferior().read_memory(gdb.parse_and_eval('&'+name),12).tobytes().hex()for name in ('c2_real_zero','c2_real_one','c2_real_two')]}
        if records[0].get('state_policy')in ('pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            pools=gdb.parse_and_eval('c2_pools');arena=pools[0];head=arena['blocks']
            assert int(head)
            info['pool_result']={'free':int(gdb.parse_and_eval('c2_free_blocks')),
                'types_offset':int(gdb.parse_and_eval('c2_types'))-int(head),
                'cursor_offset':int(arena['cursor'])-int(head),'remaining':int(arena['remaining']),
                'next':int(head.dereference()['next']),'capacity':int(head.dereference()['capacity']),
                'payload':gdb.selected_inferior().read_memory(int(head)+8,4088).tobytes().hex(),
                'other_pools':[gdb.selected_inferior().read_memory(pools[i].address,12).tobytes().hex()for i in (1,2,3)]}
        if records[0].get('state_policy')in ('section-symbol','section-linked','section-defaults'):
            payload=bytearray.fromhex(info['pool_result']['payload']);payload[88:92]=bytes(4);payload[118:122]=bytes(4)
            info['pool_result']['payload']=payload.hex()
        if records[0].get('state_policy')in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            payload=bytearray.fromhex(info['pool_result']['payload'])
            for index in range(5):
                for field in (24,36,50,54):payload[64+index*88+field:68+index*88+field]=bytes(4)
            if records[0]['state_policy']=='symbol-type-input':payload[532:536]=bytes(4)
            info['pool_result']['payload']=payload.hex()
        if records[0].get('state_policy')in ('header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
            first=gdb.parse_and_eval('c2_input');second=gdb.parse_and_eval('c2_globals_input');iob=gdb.parse_and_eval('&recon_iob[0]')
            info['header_streams']={'input_index':(int(first)-int(iob))//32,'globals_index':(int(second)-int(iob))//32,
                                    'input':gdb.selected_inferior().read_memory(first,32).tobytes().hex(),
                                    'globals':gdb.selected_inferior().read_memory(second,32).tobytes().hex(),
                                    'distinct':int(first)!=int(second),'x87_control':int(gdb.parse_and_eval('$fctrl'))}
            if records[0]['state_policy']in ('il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
                info['header_streams']['active_index']=(int(gdb.parse_and_eval('c2_current_il'))-int(iob))//32
            if records[0]['state_policy']in ('il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
                stream=second.dereference();valid=int(stream['_cnt'])+int(stream['_ptr'])-int(stream['_base'])
                assert 0<valid<=int(stream['_bufsiz'])<=4096
                info['header_streams']['valid_buffer']=gdb.selected_inferior().read_memory(stream['_base'],valid).tobytes().hex()
                if records[0]['state_policy']=='il-long-read':info['il_word_result']=int(frame.older().read_var('machine'))

        if records[0].get('state_policy')=='intermediate-open':
            base=gdb.parse_and_eval('base')
            info['intermediate_open']={'base':base.string(),'suffix':gdb.parse_and_eval('suffix').string(),
                                       'mode':gdb.parse_and_eval('mode').string(),'description':gdb.parse_and_eval('description').string(),
                                       'base_alias':int(base)==int(gdb.parse_and_eval('c2_intermediate_base')),'x87_control':int(gdb.parse_and_eval('$fctrl'))}
        if records[0].get('state_policy')=='integer-option':
            vector=gdb.parse_and_eval('recon_crt_environment');index=0;origin=None
            while int(vector[index]):
                entry=vector[index];prefix=gdb.selected_inferior().read_memory(entry,14).tobytes()
                if prefix==b'MSC_CMD_FLAGS=':origin=int(entry)+14;break
                index+=1
            assert origin is not None
            value=gdb.parse_and_eval('value');current=gdb.parse_and_eval('c2_current_option');operand=gdb.parse_and_eval('c2_option_operand')
            array=gdb.parse_and_eval('environment_arguments');argv=gdb.parse_and_eval('c2_argv')
            info['integer_option']={'text':value.string(),'input_offset':int(value)-origin,'current':current.string(),
                                    'current_offset':int(current)-origin,'operand_alias':int(operand)==int(value),
                                    'argv_offset':int(argv)-int(array.address),'argv_value_alias':int(argv[0])==int(value),
                                    'mb_cur_max':int(gdb.parse_and_eval('recon_mb_cur_max')),'character_flags':int(gdb.parse_and_eval('recon_pctype[49]'))}
        if records[0].get('state_policy')=='environment-copy':
            flags=frame.older().read_var('flags');vector=gdb.parse_and_eval('recon_crt_environment');index=0;identity=None
            while int(vector[index]):
                if vector[index].string().startswith('MSC_CMD_FLAGS='):identity=int(vector[index])+14;break
                index+=1
            assert identity is not None
            info['environment_copy']={'flags':flags.string(),'environment_alias':int(flags)==identity}
        if records[0].get('state_policy')=='format-integer':
            output=gdb.parse_and_eval('buffer')
            origin=frame.older().read_var('buffer')
            info['integer_format']={'value':int(gdb.parse_and_eval('value')),'base':int(gdb.parse_and_eval('base')),
                                    'output_offset':int(output)-int(origin),'prefix':gdb.selected_inferior().read_memory(output-13,13).tobytes().hex()}
        if records[0].get('state_policy')in ('format-prefix','format-prefix-args'):
            prefix_args=records[0]['state_policy']=='format-prefix-args'
            arguments=gdb.parse_and_eval('(int *)arguments'if prefix_args else '(int *)((char *)&format+4)')
            buffer=gdb.parse_and_eval('buffer')
            caller=frame.older().older()if prefix_args else frame.older()
            origin=caller.read_var('text').address
            info['format_prefix']={'format':gdb.parse_and_eval('format').string(),'character':int(arguments[0]),'number':int(arguments[1]),
                                   'output_offset':int(buffer)-int(origin),'prefix':gdb.selected_inferior().read_memory(buffer-11,12).tobytes().hex()}
        if records[0].get('state_policy')in ('diagnostic-emit','cached-message'):
            info['message_cache']={'initialized':int(gdb.parse_and_eval('messages_initialized')),
                                  'cache':[[int(gdb.parse_and_eval('messages[%d].id'%i)),gdb.parse_and_eval('messages[%d].text'%i).string()if int(gdb.parse_and_eval('messages[%d].text'%i))else None]for i in range(7)],
                                  'bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('message_buffer'),384).tobytes().hex()}
            if records[0]['state_policy']=='diagnostic-emit':
                arguments=gdb.parse_and_eval('(char **)arguments')
                info['diagnostic']={'category':int(gdb.parse_and_eval('category')),'message':int(gdb.parse_and_eval('message')),
                                    'format':gdb.parse_and_eval('format').string(),'format_alias':int(gdb.parse_and_eval('format == message_buffer')),
                                    'argument':arguments[0].string(),'phase':arguments[1].string(),
                                    'argument_alias':int(arguments[0]==gdb.parse_and_eval('c2_unexpected_argument'))}
            else:
                info['cached_message_id']=int(gdb.parse_and_eval('message'))
        if records[0].get('state_policy')in ('message-load','message-unescape'):
            info['message_load']={'message':int(gdb.parse_and_eval('message'))if records[0]['state_policy']=='message-load'else 0,'buffer':int(gdb.parse_and_eval('buffer')),
                                  'capacity':int(gdb.parse_and_eval('capacity'))if records[0]['state_policy']=='message-load'else 384,'allow_missing':int(gdb.parse_and_eval('allow_missing'))if records[0]['state_policy']=='message-load'else 1,
                                  'initialized':int(gdb.parse_and_eval('messages_initialized')),
                                  'cache':[[int(gdb.parse_and_eval('messages[%d].id'%i)),int(gdb.parse_and_eval('messages[%d].text'%i))]for i in range(7)],
                                  'bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('message_buffer'),384).tobytes().hex()}
        if records[0].get('state_policy')=='match-option':
            info['matcher']={'table':int(gdb.parse_and_eval('table')),'input':gdb.parse_and_eval('input').string(),
                             'next':int(gdb.parse_and_eval('next')),'mode':int(gdb.parse_and_eval('mode')),
                             'record_size':int(gdb.parse_and_eval('sizeof(C2Option)')),
                             'storage_records':int(gdb.parse_and_eval('sizeof(c2_options)/sizeof(C2Option)'))}
            gdb.write('C2-MATCHER table=0x%x next=0x%x mode=%d\n'%(info['matcher']['table'],info['matcher']['next'],info['matcher']['mode']))
        if records[0].get('state_policy')=='exit-callback':
            info['callback']=int(gdb.parse_and_eval('callback'))
            gdb.write('C2-CALLBACK 0x%x\n'%info['callback'])
        if records[0].get('state_policy')=='main-arguments-and-initial-state':
            def vector(expression):
                pointer=gdb.parse_and_eval(expression);result=[];i=0
                while int(pointer[i]):
                    result.append(pointer[i].string());i+=1
                    assert i<4096
                return result
            info['main']={'argc':int(gdb.parse_and_eval('argc')),'argv':vector('argv'),'environment':vector('environment'),
                          'initenv_alias':int(gdb.parse_and_eval('recon_crt_initenv == environment')),
                          'x87_control':int(gdb.parse_and_eval('$fctrl'))}
    if records and records[0].get('state_policy')in ('wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
        if records[0]['state_policy']=='wildcard-match':
            info['wildcard']={'pattern':gdb.parse_and_eval('pattern').string(),'input':gdb.parse_and_eval('input').string(),
                          'kind':int(gdb.parse_and_eval('kind')),'current_alias':int(gdb.parse_and_eval('c2_current_option == input'))}
        if records[0]['state_policy']=='apply-option':
            info['apply']={'option':int(gdb.parse_and_eval('option')),'value':gdb.parse_and_eval('value').string(),
                           'mode':int(gdb.parse_and_eval('mode')),'current_alias':int(gdb.parse_and_eval('c2_current_option == value'))}
        if records[0]['state_policy']=='message-initialize':
            info['message_initialization']={'current':gdb.parse_and_eval('c2_current_option').string(),
                                           'current_alias':int(gdb.parse_and_eval('c2_current_option == c2_unexpected_argument')),
                                           'current_pointer':int(gdb.parse_and_eval('c2_current_option'))}
        if records[0]['state_policy']=='message-lookup':
            info['lookup']={'message':int(gdb.parse_and_eval('message')),'allow_missing':int(gdb.parse_and_eval('allow_missing')),
                            'current':gdb.parse_and_eval('c2_current_option').string(),
                            'current_alias':int(gdb.parse_and_eval('c2_current_option == c2_unexpected_argument')),
                            'current_pointer':int(gdb.parse_and_eval('c2_current_option'))}
        if records[0]['state_policy']=='fatal-option':
            arguments=gdb.parse_and_eval('(char **)((char *)&message+4)')
            info['fatal']={'message':int(gdb.parse_and_eval('message')),'argument':arguments[0].string(),'phase':arguments[1].string(),
                           'current_alias':int(arguments[0]==gdb.parse_and_eval('c2_current_option')),
                           'unexpected_alias':int(arguments[0]==gdb.parse_and_eval('c2_unexpected_argument')),
                           'argument_pointer':int(arguments[0])}
        model=json.loads(Path('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/filename-record-native/data-model.json').read_text())
        table=gdb.parse_and_eval('c2_options');options=[];selectors={};objects={}
        for i in range(len(model['options'])+1):
            row=table[i]
            options.append({'pattern':row['pattern'].string()if int(row['pattern'])else None,'value':int(row['value']),
                            'result':int(row['result']),'kind':int(row['kind']),'reserved':int(row['reserved'])})
        for item in model['selectors']:
            rows=[];table=gdb.parse_and_eval(item['native'])
            for i in range(len(item['entries'])+1):
                row=table[i];rows.append({'selector':int(row['selector']),'kind':int(row['kind']),'value':int(row['value'])})
            selectors[item['native']]=rows
        for item in model['objects']:
            if item['kind']=='selector':continue
            value=gdb.parse_and_eval(item['native'])
            objects[item['native']]=(value.string()if int(value)else None)if item['kind']=='string'else int(value)&0xffffffff
        info['data_state']={'options':options,'selectors':selectors,'objects':objects}
    if records and records[0].get('state_policy')in ('intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input'):
        model=json.loads(Path('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/filename-record-native/intrinsic-model.json').read_text())
        descriptions=gdb.parse_and_eval('intrinsics');expected={r['number']:r for r in model['rows']};texts={r['original']:bytes.fromhex(r['bytes'])for r in model['texts']}
        rows=[]
        for i in range(344):
            description=descriptions[i]
            row={key:int(description[key])for key in ('result_type','reserved_type','flags','argument_signature','reserved_flags','commands','runtime_name')}
            for field in ('commands','runtime_name'):
                size=len(texts[expected[i][field]])+1
                row[field+'_bytes']=gdb.selected_inferior().read_memory(row[field],size).tobytes().hex()
            rows.append(row)
        info['intrinsic_table']=rows
    open('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/filename-record-native/state.json','w').write(json.dumps(info,indent=2)+'\n')
