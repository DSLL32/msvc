import gdb,json
from pathlib import Path
if exit_status is not None:
    assert entry_input
    info=dict(entry_input,completed=True,exit_status=exit_status,trace=trace)
    Path('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/intreal-native-5/state.json').write_text(json.dumps(info,indent=2)+'\n')
else:
    import gdb,json
    from pathlib import Path
    frame=gdb.newest_frame()
    info={'function':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'signal':gdb.execute('info program',to_string=True),'backtrace':gdb.execute('bt',to_string=True),'trace':trace,'globals':{}}
    pid=gdb.selected_inferior().pid
    actual_args=Path('/proc/%d/cmdline'%pid).read_bytes().rstrip(b'\0').split(b'\0')
    actual_env=Path('/proc/%d/environ'%pid).read_bytes().split(b'\0')
    Path('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/intreal-native-5/native-maps.txt').write_text(Path('/proc/%d/maps'%pid).read_text())
    gdb.execute('info registers eip')
    gdb.execute('info symbol $pc')
    gdb.execute('x/1ib $pc-1')
    gdb.execute('info program')
    info['arguments']=[v.decode()for v in actual_args[1:]]
    flags=[v[14:].decode()for v in actual_env if v.startswith(b'MSC_CMD_FLAGS=')]
    assert not flags,'host compiler flags leaked'
    info['MSC_CMD_FLAGS']=entry_input['MSC_CMD_FLAGS']
    records=[r for r in [{'name': '_c2_math_error', 'native': 'c2_math_error', 'start': 4484380, 'end': 4484383, 'abi': 'cdecl, one pointer parameter', 'kind': 'crt-source'}, {'name': '_c2_print_banner', 'native': 'c2_print_banner', 'start': 4738355, 'end': 4738356, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_opcodes', 'native': 'c2_initialize_opcodes', 'start': 4534439, 'end': 4534440, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_machine', 'native': 'c2_initialize_machine', 'start': 4534522, 'end': 4534523, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_trees', 'native': 'c2_initialize_trees', 'start': 4534572, 'end': 4534573, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_codegen', 'native': 'c2_initialize_codegen', 'start': 4534958, 'end': 4534959, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_registers', 'native': 'c2_initialize_registers', 'start': 4535034, 'end': 4535035, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_optimizer', 'native': 'c2_initialize_optimizer', 'start': 4535045, 'end': 4535046, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_compile_unit', 'native': 'c2_compile_unit', 'start': 4538855, 'end': 4538856, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_finalize_output', 'native': 'c2_finalize_output', 'start': 4541174, 'end': 4541175, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_flush_debug', 'native': 'c2_flush_debug', 'start': 4766214, 'end': 4766215, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_console_handler@4', 'native': 'c2_console_handler', 'start': 4737491, 'end': 4737492, 'pending_extent': True, 'kind': 'application', 'abi': 'stdcall, DWORD control event; RET4'}, {'name': '@c2_report_internal@8', 'native': 'c2_report_internal', 'start': 4746044, 'end': 4746068, 'pending_extent': False, 'kind': 'application', 'abi': 'fastcall, ECX source path EDX line', 'extent_known_for_tail_abi': True, 'extent_evidence': 'cases/analysis/node-size-disassembly.txt: complete486b3c..486b54 reports fatal internal source/line then RET0; still application INT3/no credit'}, {'name': '@c2_system_fatal@12', 'native': 'c2_system_fatal', 'start': 4765320, 'end': 4765358, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX message EDX prefix; stack filename; RET4'}, {'name': '@resolve_diagnostic_location@8', 'native': 'resolve_diagnostic_location', 'start': 4413704, 'end': 4413705, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall ECX location EDX output filename'}, {'name': '@c2_open_pdb@8', 'native': 'c2_open_pdb', 'start': 4765792, 'end': 4765793, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall ECX PDB filename EDX object filename; int result; no stack parameters'}, {'name': '_store_extended', 'native': 'store_extended', 'start': 4609886, 'end': 4609887, 'pending_extent': True, 'kind': 'application', 'abi': 'private ESI C2Real pointer, ST0 incoming80bit value; pop ST0, RET0, preservesintegerregisters', 'state_policy': 'extended-store', 'state': [{'original': '0x499708', 'native': '(unsigned long)c2_onexit_end'}, {'original': '0x4996d4', 'native': '(unsigned long)c2_onexit_begin'}, {'original': '0x49975c', 'native': 'c2_fmode'}, {'original': '0x4980d4', 'native': 'c2_commode'}, {'original': '0x499718', 'native': 'c2_adjust_fdiv'}, {'original': '0x499694', 'native': 'c2_default_math_error'}, {'original': '0x4980d0', 'native': 'c2_new_mode'}, {'original': '0x4980cc', 'native': 'c2_wildcards'}, {'original': '0x7803fa88', 'native': 'recon_fmode'}, {'original': '0x7803fa94', 'native': 'recon_commode'}, {'original': '0x7803f928', 'native': 'recon_adjust_fdiv'}, {'original': '0x7803fa90', 'native': 'recon_crt_app_type'}, {'original': '0x7803f198', 'native': 'recon_crt_newmode'}, {'original': '0x490b7c', 'native': 'c2_il_extended_header'}, {'original': '0x4972f8', 'native': 'c2_il_source_table'}, {'original': '0x497df8', 'native': 'c2_il_symbol_count'}, {'original': '0x491050', 'native': 'c2_il_initial_symbol_count'}, {'original': '0x497f6c', 'native': 'c2_il_target_size'}, {'original': '0x497de8', 'native': 'c2_il_eight_byte_target'}, {'original': '0x48fb68', 'native': 'c2_symbol_hash_count'}, {'original': '0x491040', 'native': 'c2_node_bytes[0]'}, {'original': '0x491044', 'native': 'c2_node_bytes[1]'}, {'original': '0x491048', 'native': 'c2_node_bytes[2]'}, {'original': '0x49104c', 'native': 'c2_node_bytes[3]'}], 'expected_argc': 0, 'expected_argv': ['-G5']}] if r['native']==frame.name()]
    if len(records)==1:
        for view in records[0].get('state',[]):
            info['globals'][view['original']]=int(gdb.parse_and_eval(view['native'])) & 0xffffffff
        if records[0].get('state_policy')in ('command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store'):
            ptr=gdb.parse_and_eval('c2_argv');values=[];i=0
            while int(ptr[i]):
                values.append(ptr[i].string());i+=1
                assert i<4096
            info['command_state']={'argc':int(gdb.parse_and_eval('c2_argc')),'argv':values}
            if records[0]['state_policy']=='environment-flags':
                info['flags']=None if int(gdb.parse_and_eval('flags'))==0 else gdb.parse_and_eval('flags').string()
        if records[0].get('state_policy')in ('alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store'):
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
        if records[0].get('state_policy')in ('symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store'):
            import hashlib
            table=gdb.parse_and_eval('c2_symbol_hash')
            data=gdb.selected_inferior().read_memory(table.address,4096).tobytes()
            info['symbol_hash']={'slots':int(gdb.parse_and_eval('sizeof(c2_symbol_hash)/sizeof(c2_symbol_hash[0])')),'sha256':hashlib.sha256(data).hexdigest()}
        if records[0].get('state_policy')=='allocate-node':
            info['allocate_node']={'kind':int(gdb.parse_and_eval('kind')),'pool':int(gdb.parse_and_eval('pool'))}
        if records[0].get('state_policy')=='node-size':
            caller=frame.older()
            info['node_size']={'kind':int(gdb.parse_and_eval('kind')),'caller_kind':int(caller.read_var('kind')),'pool':int(caller.read_var('pool'))}
        if records[0].get('state_policy')=='pool-allocate':
            caller=frame.older()
            info['pool_allocate']={'pool':int(gdb.parse_and_eval('pool')),'bytes':int(gdb.parse_and_eval('bytes')),
                                   'caller_kind':int(caller.read_var('kind')),'caller_pool':int(caller.read_var('pool'))}
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
        if records[0].get('state_policy')in ('pool-result','real-from-int','extended-store'):
            pools=gdb.parse_and_eval('c2_pools');arena=pools[0];head=arena['blocks']
            assert int(head)
            info['pool_result']={'free':int(gdb.parse_and_eval('c2_free_blocks')),
                'types_offset':int(gdb.parse_and_eval('c2_types'))-int(head),
                'cursor_offset':int(arena['cursor'])-int(head),'remaining':int(arena['remaining']),
                'next':int(head.dereference()['next']),'capacity':int(head.dereference()['capacity']),
                'payload':gdb.selected_inferior().read_memory(int(head)+8,4088).tobytes().hex(),
                'other_pools':[gdb.selected_inferior().read_memory(pools[i].address,12).tobytes().hex()for i in (1,2,3)]}
        if records[0].get('state_policy')in ('header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store'):
            first=gdb.parse_and_eval('c2_input');second=gdb.parse_and_eval('c2_globals_input');iob=gdb.parse_and_eval('&recon_iob[0]')
            info['header_streams']={'input_index':(int(first)-int(iob))//32,'globals_index':(int(second)-int(iob))//32,
                                    'input':gdb.selected_inferior().read_memory(first,32).tobytes().hex(),
                                    'globals':gdb.selected_inferior().read_memory(second,32).tobytes().hex(),
                                    'distinct':int(first)!=int(second),'x87_control':int(gdb.parse_and_eval('$fctrl'))}
            if records[0]['state_policy']in ('il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store'):
                info['header_streams']['active_index']=(int(gdb.parse_and_eval('c2_current_il'))-int(iob))//32
            if records[0]['state_policy']in ('il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store'):
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
    if records and records[0].get('state_policy')in ('wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store'):
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
        model=json.loads(Path('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/intreal-native-5/data-model.json').read_text())
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
    open('/root/.ai-tmp/msvc2/vc5.10/c2/numeric-control-flow/intreal-native-5/state.json','w').write(json.dumps(info,indent=2)+'\n')
