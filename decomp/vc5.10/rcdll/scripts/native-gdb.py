"""Loaded only by native GDB; raw trap-derived frontier, no Wine dependency."""
import gdb
import hashlib
import json
import os
import sys
from pathlib import Path
output=Path(os.environ['RCDLL_NATIVE_SNAPSHOT'])
config_path=Path(os.environ['RCDLL_NATIVE_CONFIG'])
sys.path.insert(0,str(Path(os.environ['RCDLL_NATIVE_SCRIPT']).parent))
from protocol import read_configuration,logical_name
config=read_configuration(config_path)
def capture():
    try:
        frame=gdb.newest_frame()
        raw_name=frame.name();name=logical_name(raw_name)
        matches=[f for f in config['functions'] if f['name']==name]
        pc=int(gdb.parse_and_eval('$eip'))
        instruction=bytes(gdb.selected_inferior().read_memory(pc-1,1))
        complete=name=='fixture_complete'
        if not complete:
            assert instruction==b'\xcc','stop is not an executed INT3'
            assert len(matches)==1,'trap has no unique indexed logical function'
        f=dict(start=None) if complete else matches[0]
        gdb.execute('printf "RCDLL_PC=%08x\\n", $pc')
        gdb.execute('bt')
        gdb.execute('info address '+raw_name)
        registers={n:int(gdb.parse_and_eval('$'+n))&0xffffffff for n in ['eip','esp','ebp','eax','ecx','edx','ebx','esi','edi']}
        arguments={'result':int(frame.read_var('result'))} if complete else {}
        if name in ['_DllMainCRTStartup','DllMain','_CRT_INIT']:
            arguments={n:int(frame.read_var(n))&0xffffffff for n in ['instance','reason','reserved']}
        globals={}
        for item in json.loads((config_path.parent/'data.json').read_text()):
            value=gdb.parse_and_eval(item['native_expression'])
            globals[item['name']]=bytes(gdb.selected_inferior().read_memory(int(value.address),item['bytes'])).hex()
        identity=gdb.parse_and_eval('fixture_registered_identity').dereference()
        module=dict(token=int(gdb.parse_and_eval('fixture_module'))&0xffffffff,
            name=identity['name'].string(),path=identity['path'].string(),sha256=identity['sha256'].string(),flags=int(identity['flags']))
        exit_base=int(gdb.parse_and_eval('dll_exit_base'))&0xffffffff
        exit_end=int(gdb.parse_and_eval('dll_exit_end'))&0xffffffff
        exit_array=dict(base=exit_base,end=exit_end)
        if exit_base:exit_array['first']=bytes(gdb.selected_inferior().read_memory(exit_base,4)).hex()
        if name=='RC':
            arguments={n:int(frame.read_var(n))&0xffffffff for n in ['window','unused','diagnostic','resource','count']}
            vector=frame.read_var('arguments');arguments['argv']=[vector[n].string()for n in range(arguments['count'])]
            arguments['terminator']=int(vector[arguments['count']])
            arguments['host_callback']=int(gdb.parse_and_eval('(void *)fixture_diagnostic'))&0xffffffff
        if name=='main':
            arguments['count']=int(frame.read_var('count'))
            vector=frame.read_var('arguments');arguments['argv']=[vector[n].string()for n in range(arguments['count'])]
            arguments['terminator']=int(vector[arguments['count']])
            arguments['host_callback']=int(gdb.parse_and_eval('(void *)fixture_diagnostic'))&0xffffffff
        if name in ['rc_read_string','rc_expect_end']:
            cursor=int(gdb.parse_and_eval('rc_source_cursor'))
            arguments['reader']=dict(formals=[],caller=frame.older().name(),cursor=cursor,current_word=int(gdb.parse_and_eval('*rc_source_cursor')),header_mode=int(gdb.parse_and_eval('rc_header_mode')),output_active=int(gdb.parse_and_eval('rc_cpp_active')),in_directive=int(gdb.parse_and_eval('rc_in_directive')),token_kind=int(gdb.parse_and_eval('rc_token_kind')),token_value=int(gdb.parse_and_eval('rc_token_value')))
            if name=='rc_read_string':assert frame.older().name()=='rc_next_token'
            else:
                reads=json.loads((output.parent/'string-reads.json').read_text());assert len(reads)==1
                arguments['reader']=reads[0];arguments['string_reads']=reads
                word=frame.read_var('word');raw=bytearray()
                for offset in range(257):
                    value=bytes(gdb.selected_inferior().read_memory(int(word)+offset*2,2));raw.extend(value)
                    if value==b'\0\0':break
                else:raise AssertionError('unterminated expect-end word')
                arguments['expect_end']=dict(pointer=int(word),bytes=raw.hex(),formal_bytes=word.type.sizeof,current_word=int(gdb.parse_and_eval('*rc_source_cursor')),cursor=int(gdb.parse_and_eval('rc_source_cursor')),output_active=int(gdb.parse_and_eval('rc_cpp_active')),in_directive=int(gdb.parse_and_eval('rc_in_directive')))
                arguments['string_current']=dict(pointer=int(gdb.parse_and_eval('&rc_string_buffer')),payload=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_string_buffer')),4090)).hex(),extra=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_token_extra')),2)).hex())
            from importlib.machinery import SourceFileLoader
            source_observer=SourceFileLoader('native_preprocess',str(Path(os.environ['RCDLL_NATIVE_SCRIPT']).parent/'native-preprocess.py')).load_module()
            source_parent=frame.older()
            while source_parent and source_parent.name()!='rc_open_source':source_parent=source_parent.older()
            assert source_parent
            source_path=source_parent.read_var('path');arguments['source_open']=dict(path=int(source_path),first=int(source_parent.read_var('first')),path_bytes=source_observer.wide_bytes(int(source_path)),formal_bytes=dict(path=source_path.type.sizeof,first=source_parent.read_var('first').type.sizeof))
            file=gdb.parse_and_eval('rc_source_file').dereference();descriptor=int(file['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            arguments['source_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text()
            arguments['interned_types']=json.loads((output.parent/'interned-types.json').read_text())
            node=arguments['interned_types'][0]['node'];arguments['integer_node']=dict(pointer=node,payload=bytes(gdb.selected_inferior().read_memory(node,20)).hex())
            buckets=gdb.parse_and_eval('rc_type_buckets');arguments['type_buckets']=dict(address=int(buckets.address),bytes=buckets.type.sizeof,count=256,pointer_bytes=buckets[0].type.sizeof,payload=bytes(gdb.selected_inferior().read_memory(int(buckets.address),buckets.type.sizeof)).hex())
        if name in ['rc_scan_comment','rc_match_logical_character','rc_skip_block_comment','rc_push_condition','rc_parse_define','rc_read_macro_replacement','rc_check_macro_space','rc_find_macro_parameter','rc_evaluate_condition','rc_parse_conditional_expression','rc_parse_logical_or']:
            depth=int(gdb.parse_and_eval('rc_source_depth'));assert depth==1
            current=gdb.parse_and_eval('rc_source_frames[1]');parent=gdb.parse_and_eval('rc_source_frames[0]');file=current['file'].dereference();base=int(file['_base']);size=int(file['_bufsiz']);descriptor=int(file['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            arguments['comment']=dict(formals=[],caller=frame.older().name(),depth=depth,cursor=int(gdb.parse_and_eval('rc_source_cursor')),current_word=int(gdb.parse_and_eval('*rc_source_cursor')),encoding=int(gdb.parse_and_eval('rc_source_encoding')),frame=bytes(gdb.selected_inferior().read_memory(int(current.address),36)).hex(),parent_frame=bytes(gdb.selected_inferior().read_memory(int(parent.address),36)).hex(),source_block=bytes(gdb.selected_inferior().read_memory(int(current['buffer'])-8,8206)).hex(),source_file=bytes(gdb.selected_inferior().read_memory(int(current['file']),32)).hex(),file_buffer=bytes(gdb.selected_inferior().read_memory(base,size)).hex(),file_host_path=os.readlink('/proc/%d/fd/%d'%(gdb.selected_inferior().pid,handle)),file_fdinfo=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text(),marker=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_source_marker')),514)).hex(),work_buffer=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_string_buffer')),4090)).hex())
            if name=='rc_match_logical_character':
                character=frame.read_var('character');arguments['character_match']=dict(character=int(character),formal_bytes=character.type.sizeof,stack_slot_bytes=4,comparison_bits=32)
                arguments['comment']['formals']=[dict(name='character',value=int(character),bytes=character.type.sizeof)]
            arguments['include_opens']=json.loads((output.parent/'include-opens.json').read_text())
            if name=='rc_skip_block_comment':arguments['character_matches']=json.loads((output.parent/'character-matches.json').read_text())
            if name in ['rc_push_condition','rc_parse_define','rc_read_macro_replacement','rc_check_macro_space','rc_find_macro_parameter']:
                parent=frame.older();parent=parent.older()if name in ['rc_read_macro_replacement','rc_check_macro_space','rc_find_macro_parameter']else parent;parent=parent.older()if name in ['rc_check_macro_space','rc_find_macro_parameter']else parent;arguments['block_comments']=json.loads((output.parent/'block-comments.json').read_text());arguments['condition_entry']=dict(formals=[],nesting=int(gdb.parse_and_eval('rc_cpp_nesting_level')),state=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_condition_state')),128)).hex(),caller=parent.name(),directive=int(parent.read_var('directive')),word=int(parent.read_var('word')),macro=int(gdb.parse_and_eval('&rc_macro_name')),macro_bytes=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_macro_name')),1024)).hex())
                if name=='rc_push_condition':arguments['condition_entry']['condition']=int(parent.read_var('condition'))
                else:arguments['condition_pushes']=json.loads((output.parent/'condition-pushes.json').read_text())
                if name in ['rc_read_macro_replacement','rc_check_macro_space','rc_find_macro_parameter']:
                    arguments['define_parsers']=json.loads((output.parent/'define-parsers.json').read_text());arguments['macro_replacement']=dict(formals=[],caller=(frame.older().older()if name in ['rc_check_macro_space','rc_find_macro_parameter']else frame.older()).name(),parameter_count=int(gdb.parse_and_eval('rc_macro_parameter_count')),pointer=int(gdb.parse_and_eval('rc_macro_replacement')),buffer=int(gdb.parse_and_eval('&rc_string_buffer')),buffer_bytes=gdb.parse_and_eval('rc_string_buffer').type.sizeof,saved_parameters=int((frame.older().older()if name in ['rc_check_macro_space','rc_find_macro_parameter']else frame.older()).read_var('parameters')))
        if name=='rc_check_macro_space':
            cursor=frame.read_var('cursor');parent=frame.older();arguments['macro_space']=dict(cursor=int(cursor),formal_bytes=cursor.type.sizeof,stack_slot_bytes=4,caller=parent.name(),out=int(parent.read_var('out')),character=int(parent.read_var('character')),character_bytes=parent.read_var('character').type.sizeof,stringify=int(parent.read_var('stringify')),charize=int(parent.read_var('charize')))
        if name=='rc_find_macro_parameter':
            pointer=frame.read_var('name');raw=bytearray()
            for offset in range(2045):
                word=bytes(gdb.selected_inferior().read_memory(int(pointer)+2*offset,2));raw.extend(word)
                if word==b'\0\0':break
            else:raise AssertionError('unterminated macro parameter name')
            parent=frame.older();arguments['macro_parameter']=dict(pointer=int(pointer),formal_bytes=pointer.type.sizeof,stack_slot_bytes=4,bytes=raw.hex(),caller=parent.name(),start=int(parent.read_var('start')),out=int(parent.read_var('out')),character=int(parent.read_var('character')),stringify=int(parent.read_var('stringify')),charize=int(parent.read_var('charize')))
            arguments['comment']['formals']=[dict(name='name',value=int(pointer),bytes=pointer.type.sizeof)]
            arguments['macro_space_checks']=json.loads((output.parent/'macro-space-checks.json').read_text())
        if name in ['rc_evaluate_condition','rc_parse_conditional_expression','rc_parse_logical_or']:
            parent=frame.older();
            while parent.name()!='rc_handle_directive':parent=parent.older()
            arguments['block_comments']=json.loads((output.parent/'block-comments.json').read_text());arguments['condition_entry']=dict(formals=[],nesting=int(gdb.parse_and_eval('rc_cpp_nesting_level')),state=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_condition_state')),128)).hex(),caller=parent.name(),directive=int(parent.read_var('directive')),word=int(parent.read_var('word')),macro=int(gdb.parse_and_eval('&rc_macro_name')),macro_bytes=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_macro_name')),1024)).hex())
            arguments['macro_replacement']=dict(parameter_count=int(gdb.parse_and_eval('rc_macro_parameter_count')),pointer=int(gdb.parse_and_eval('rc_macro_replacement')),buffer=int(gdb.parse_and_eval('&rc_string_buffer')),buffer_bytes=gdb.parse_and_eval('rc_string_buffer').type.sizeof)
            arguments['expression_entry']=dict(formals=[],saved_output=int(parent.read_var('saved_output')),output_active=int(gdb.parse_and_eval('rc_cpp_active')),in_expression=int(gdb.parse_and_eval('rc_in_condition_expression')),in_directive=int(gdb.parse_and_eval('rc_in_directive')))
            arguments['parameter_lookups']=json.loads((output.parent/'parameter-lookups.json').read_text());arguments['define_parsers']=json.loads((output.parent/'define-parsers.json').read_text())
        if name in ['rc_parse_conditional_expression','rc_parse_logical_or']:
            arguments['condition_evaluation']=dict(formals=[],caller=frame.older().name(),parentheses=int(gdb.parse_and_eval('rc_expression_parentheses')),token_kind=int(gdb.parse_and_eval('rc_token_kind')))
        if name=='rc_search_include_directories':
            listing=gdb.parse_and_eval('rc_cpp_includes');marker=gdb.parse_and_eval('rc_source_marker')
            arguments['include_search']=dict(formals=[],caller=frame.older().name(),cursor=int(gdb.parse_and_eval('rc_source_cursor')),current_word=int(gdb.parse_and_eval('*rc_source_cursor')),macro=int(gdb.parse_and_eval('&rc_macro_name')),macro_bytes=1024,macro_current=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_macro_name')),1024)).hex(),marker=int(marker.address),marker_current=bytes(gdb.selected_inferior().read_memory(int(marker.address),marker.type.sizeof)).hex(),marker_bytes=marker.type.sizeof,token_value=int(gdb.parse_and_eval('rc_token_value')),token_kind=int(gdb.parse_and_eval('rc_token_kind')),header_mode=int(gdb.parse_and_eval('rc_header_mode')),output_active=int(gdb.parse_and_eval('rc_cpp_active')),in_directive=int(gdb.parse_and_eval('rc_in_directive')),include_count=int(listing['remaining']),directories=[],path_characters=int(gdb.parse_and_eval('rc_cpp_path_characters')),number_buffer=int(gdb.parse_and_eval('rc_number_buffer')))
            for index in range(100):
                pointer=listing['items'][index]
                if int(pointer):
                    raw=bytearray()
                    for offset in range(257):
                        word=bytes(gdb.selected_inferior().read_memory(int(pointer)+2*offset,2));raw.extend(word)
                        if word==b'\0\0':break
                    else:raise AssertionError('unterminated include path')
                    arguments['include_search']['directories'].append(dict(index=index,pointer=int(pointer),bytes=raw.hex()))
            storage=gdb.parse_and_eval('rc_string_buffer')
            arguments['include_search']['string_storage']=dict(address=int(storage.address),bytes=storage.type.sizeof,payload=bytes(gdb.selected_inferior().read_memory(int(storage.address),storage.type.sizeof)).hex(),number_offset=int(gdb.parse_and_eval('rc_number_buffer'))-int(storage.address))
            arguments['angle_names']=json.loads((output.parent/'angle-names.json').read_text());arguments['include_copies']=json.loads((output.parent/'include-copies.json').read_text())
            file=gdb.parse_and_eval('rc_source_file').dereference();descriptor=int(file['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            arguments['source_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text()
        if name=='rc_parse_angle_name':
            arguments['angle_name']=dict(formals=[],caller=frame.older().name(),cursor=int(gdb.parse_and_eval('rc_source_cursor')),current_word=int(gdb.parse_and_eval('*rc_source_cursor')),macro=int(gdb.parse_and_eval('&rc_macro_name')),macro_bytes=1024,macro_before=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_macro_name')),1024)).hex(),marker=int(gdb.parse_and_eval('&rc_source_marker')),marker_before=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_source_marker')),2)).hex(),header_mode=int(gdb.parse_and_eval('rc_header_mode')),output_active=int(gdb.parse_and_eval('rc_cpp_active')),in_directive=int(gdb.parse_and_eval('rc_in_directive')),token_kind=int(gdb.parse_and_eval('rc_token_kind')))
            arguments['type_comparisons']=json.loads((output.parent/'type-comparisons.json').read_text())
            file=gdb.parse_and_eval('rc_source_file').dereference();descriptor=int(file['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            arguments['source_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text()
        if name=='rc_equal_type':
            parent=frame.older();assert parent.name()=='rc_intern_type'
            left=frame.read_var('left');right=frame.read_var('right');owner=parent.read_var('entry');buckets=gdb.parse_and_eval('rc_type_buckets')
            arguments['type_compare']=dict(left=int(left),right=int(right),formal_bytes=dict(left=left.type.sizeof,right=right.type.sizeof),object_bytes=dict(left=left.dereference().type.sizeof,right=right.dereference().type.sizeof),left_bytes=bytes(gdb.selected_inferior().read_memory(int(left),20)).hex(),right_bytes=bytes(gdb.selected_inferior().read_memory(int(right),20)).hex(),hash=int(parent.read_var('hash')),entry=int(owner),entry_bytes=owner.dereference().type.sizeof,right_member=int(owner.dereference()['type'].address),buckets_address=int(buckets.address),buckets_bytes=buckets.type.sizeof,buckets=bytes(gdb.selected_inferior().read_memory(int(buckets.address),1024)).hex())
            arguments['expected_ends']=json.loads((output.parent/'expected-ends.json').read_text())
            file=gdb.parse_and_eval('rc_source_file').dereference();descriptor=int(file['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            arguments['source_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text()
        jump=None
        if any(f['name']=='main' and not f.get('stub')for f in config['functions']):
            cursor=gdb.parse_and_eval('jump_contexts')
            legacy=int(gdb.parse_and_eval('&rc_abort_context'))
            while int(cursor):
                entry=cursor.dereference()
                if int(entry['legacy'])==legacy:
                    jump=dict(legacy=legacy,opaque_setjmp3=int(entry['opaque_setjmp3']),host_bytes=bytes(gdb.selected_inferior().read_memory(int(entry['host'].address),entry['host'].type.sizeof)).hex())
                    break
                cursor=entry['next']
        vector=[]
        for i in range(int(gdb.parse_and_eval('fixture_argument_count'))):
            pointer=int(gdb.parse_and_eval('fixture_argument_vector[%d]'%i))&0xffffffff
            storage=int(gdb.parse_and_eval('fixture_argument_storage[%d]'%i))&0xffffffff
            size=int(gdb.parse_and_eval('fixture_argument_lengths[%d]'%i))
            raw=bytes(gdb.selected_inferior().read_memory(storage,size)).hex()
            vector.append(dict(pointer=pointer,storage=storage,bytes=raw,text=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string()))
        if name=='rc_release':
            pointer=int(frame.read_var('block'))&0xffffffff
            arguments['block']=pointer
            arguments['payload']=bytes(gdb.selected_inferior().read_memory(pointer-8,528)).hex()
            arguments['include']=gdb.parse_and_eval('rc_include_paths').string()
            pointer=int(frame.older().read_var('new_paths'))&0xffffffff
            arguments['new_paths']=pointer
            arguments['new_payload']=bytes(gdb.selected_inferior().read_memory(pointer-8,528)).hex()
        if name=='rc_allocate':arguments['bytes']=int(frame.read_var('bytes'))
        if name=='rc_is_option':arguments['value']=int(frame.read_var('value'))
        if name=='rc_emit':
            message=frame.read_var('message');length=len(message.string())+1
            arguments['message']=bytes(gdb.selected_inferior().read_memory(int(message),length)).hex()
        if name=='rc_message':arguments['id']=int(frame.read_var('id'))
        if name=='rc_prepare_input':arguments['path']=frame.read_var('path').string()
        if name in ['rc_preprocess','rc_widen_arguments','rc_preprocessor_main','rc_next_argument','rc_scan_options','rc_apply_option','rc_match_option','rc_option_number','rc_open_preprocessor','rc_set_character_token','rc_hash_macro_name','rc_set_character_class','rc_define_macro','rc_find_macro','rc_copy_macro_text','rc_open_source','rc_refill_source','rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type','rc_read_string','rc_expect_end','rc_equal_type','rc_parse_angle_name','rc_search_include_directories','rc_scan_comment','rc_match_logical_character','rc_skip_block_comment','rc_push_condition','rc_parse_define','rc_read_macro_replacement','rc_check_macro_space','rc_find_macro_parameter','rc_evaluate_condition','rc_parse_conditional_expression','rc_parse_logical_or']:
            caller=frame
            if name in ['rc_next_argument','rc_scan_options','rc_apply_option','rc_match_option','rc_option_number','rc_open_preprocessor','rc_set_character_token','rc_hash_macro_name','rc_set_character_class','rc_define_macro','rc_find_macro','rc_copy_macro_text','rc_open_source','rc_refill_source','rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type','rc_read_string','rc_expect_end','rc_equal_type','rc_parse_angle_name','rc_search_include_directories','rc_scan_comment','rc_match_logical_character','rc_skip_block_comment','rc_push_condition','rc_parse_define','rc_read_macro_replacement','rc_check_macro_space','rc_find_macro_parameter','rc_evaluate_condition','rc_parse_conditional_expression','rc_parse_logical_or']:
                while caller and caller.name()!='rc_preprocessor_main':caller=caller.older()
                assert caller,'preprocessor main caller missing'
            count=int(caller.read_var('count'));preprocess_vector=caller.read_var('arguments');assert 0<count<=256
            wide=name not in ['rc_preprocess','rc_widen_arguments']
            arguments.update(count=count,argv=[]if wide else[preprocess_vector[i].string()for i in range(count)],filename_pointer=int(gdb.parse_and_eval('rc_filename')),prepared_lines=int(gdb.parse_and_eval('rc_prepared_lines')),current_line=int(gdb.parse_and_eval('rc_preparing_line')),filename=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('rc_filename')),514)).hex())
            if name=='rc_preprocess':arguments['unused']=int(frame.read_var('unused'))
            if wide:
                wide_vector=[]
                for index in range(count):
                    pointer=int(preprocess_vector[index]);data=bytearray()
                    for offset in range(4177):
                        word=bytes(gdb.selected_inferior().read_memory(pointer+2*offset,2));data.extend(word)
                        if word==b'\0\0':break
                    else:raise AssertionError('unterminated wide argument')
                    wide_vector.append(dict(pointer=pointer,bytes=data.hex()))
                arguments.update(wide_vector=wide_vector,wide_terminator=int(preprocess_vector[count]),vector_pointer=int(preprocess_vector))
                arguments['argv']=[bytes.fromhex(value['bytes']).decode('utf-16le')[:-1]for value in wide_vector]
            frees=json.loads((output.parent/'locals.json').read_text());assert frees[-1]['operation']=='free'
            # Revisit the actual FILE pointer observed at the first line read;
            # descriptor numbers and CRT array slots need not equal Wine's.
            file=frees[0]['input_file']
            arguments['closed_input']=bytes(gdb.selected_inferior().read_memory(file,32)).hex()
            if name in ['rc_next_argument','rc_scan_options','rc_apply_option','rc_match_option','rc_option_number','rc_open_preprocessor','rc_set_character_token','rc_hash_macro_name','rc_set_character_class','rc_define_macro','rc_find_macro','rc_copy_macro_text','rc_open_source','rc_refill_source','rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type','rc_read_string','rc_expect_end','rc_equal_type','rc_parse_angle_name','rc_search_include_directories','rc_scan_comment','rc_match_logical_character','rc_skip_block_comment','rc_push_condition','rc_parse_define','rc_read_macro_replacement','rc_check_macro_space','rc_find_macro_parameter','rc_evaluate_condition','rc_parse_conditional_expression','rc_parse_logical_or']:
                from importlib.machinery import SourceFileLoader
                observer=SourceFileLoader('native_preprocess',str(Path(os.environ['RCDLL_NATIVE_SCRIPT']).parent/'native-preprocess.py')).load_module()
                arguments['preprocessor']=observer.capture_preprocessor(config_path.parent/'preprocess-data.json')
                if name=='rc_scan_options':
                    arguments['scan']={field:int(frame.read_var(field))&0xffffffff for field in ['options','argument','next','duplicate']}
                    arguments['scan']['argument_bytes']=observer.wide_bytes(int(frame.read_var('argument')))
                    arguments['scan']['getter']=int(gdb.parse_and_eval('(void *)rc_next_argument'))&0xffffffff
                if name=='rc_apply_option':
                    arguments['apply']={field:int(frame.read_var(field))&0xffffffff for field in ['option','operand','duplicate']}
                    arguments['apply']['operand_bytes']=observer.wide_bytes(int(frame.read_var('operand')))
                if name=='rc_option_number':
                    parent=frame.older();assert parent.name()=='rc_apply_option'
                    arguments['number']=dict(operand=int(frame.read_var('operand')),operand_bytes=observer.wide_bytes(int(frame.read_var('operand'))),parent={field:int(parent.read_var(field))&0xffffffff for field in ['option','operand','duplicate']})
                if name=='rc_open_preprocessor':
                    arguments['open']={field:int(frame.read_var(field))&0xffffffff for field in ['input','output','defines','undefines']}
                    arguments['conversion']=json.loads((output.parent/'numeric-conversion.json').read_text())
                if name in ['rc_refill_source','rc_duplicate_source_name','rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                    source_parent=frame.older()
                    if name=='rc_intern_type':
                        assert source_parent.name()=='rc_store_integer';source_parent=source_parent.older()
                    if name=='rc_digit_value':
                        assert source_parent.name()=='rc_parse_integer_value';source_parent=source_parent.older()
                    if name in ['rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        assert source_parent.name()=='rc_read_number';source_parent=source_parent.older()
                    if name in ['rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        assert source_parent.name()=='rc_next_token';source_parent=source_parent.older()
                    if name=='rc_character_class':
                        assert source_parent.name()=='rc_read_identifier';source_parent=source_parent.older()
                    if name in ['rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        assert source_parent.name()=='rc_handle_directive';source_parent=source_parent.older()
                    if name in ['rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        assert source_parent.name()=='rc_scan_line_start';source_parent=source_parent.older()
                    if name=='rc_character_token':
                        assert source_parent.name()=='rc_quote_source_filename';source_parent=source_parent.older()
                    if name in ['rc_write_preprocessed','rc_quote_source_filename','rc_character_token']:
                        assert source_parent.name()=='rc_emit_source_line';source_parent=source_parent.older()
                    assert source_parent.name()=='rc_open_source'
                    setup=source_parent.older();assert setup.name()=='rc_open_preprocessor'
                    arguments['open']={field:int(setup.read_var(field))&0xffffffff for field in ['input','output','defines','undefines']}
                    arguments['setup_iteration']=int(setup.read_var('i'))
                    arguments['source_open']={field:int(source_parent.read_var(field))for field in ['path','first']}
                    arguments['source_open']['path_bytes']=observer.wide_bytes(int(source_parent.read_var('path')))
                    arguments['source_open']['formal_bytes']={field:source_parent.read_var(field).type.sizeof for field in ['path','first']}
                    arguments['macro_copies']=json.loads((output.parent/'macro-copies.json').read_text())
                    arguments['source_entry']=json.loads((output.parent/'source-entry.json').read_text())
                    if name=='rc_duplicate_source_name':
                        arguments['duplicate']={field:int(frame.read_var(field))for field in ['path']}
                        arguments['duplicate']['path_bytes']=observer.wide_bytes(int(frame.read_var('path')))
                        arguments['duplicate']['formal_bytes']=dict(path=frame.read_var('path').type.sizeof)
                        arguments['refills']=json.loads((output.parent/'refills.json').read_text())
                    if name in ['rc_emit_source_line','rc_write_preprocessed','rc_quote_source_filename','rc_character_token','rc_list_macro','rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        arguments['refills']=json.loads((output.parent/'refills.json').read_text())
                        arguments['source_name_copies']=json.loads((output.parent/'source-name-copies.json').read_text())
                    if name=='rc_write_preprocessed':
                        arguments['write']={field:int(frame.read_var(field))for field in ['data','size','count','file']}
                        arguments['write']['formal_bytes']={field:frame.read_var(field).type.sizeof for field in ['data','size','count','file']}
                        arguments['write']['payload']=bytes(gdb.selected_inferior().read_memory(arguments['write']['data'],arguments['write']['size']*arguments['write']['count'])).hex()
                        arguments['source_formats']=json.loads((output.parent/'source-line-format.json').read_text())
                    if name in ['rc_quote_source_filename','rc_character_token']:
                        quote_frame=frame if name=='rc_quote_source_filename'else frame.older()
                        arguments['quote']={field:int(quote_frame.read_var(field))for field in ['destination','path']}
                        arguments['quote']['path_bytes']=observer.wide_bytes(arguments['quote']['path'])
                        arguments['quote']['destination_bytes']=bytes(gdb.selected_inferior().read_memory(arguments['quote']['destination'],1024)).hex()
                        arguments['quote']['formal_bytes']={field:quote_frame.read_var(field).type.sizeof for field in ['destination','path']}
                        arguments['source_formats']=json.loads((output.parent/'source-line-format.json').read_text())
                        arguments['output_writes']=json.loads((output.parent/'output-writes.json').read_text())
                        output_file=gdb.parse_and_eval('rc_cpp_output_file').dereference();output_descriptor=int(output_file['_file']);output_handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%output_descriptor))-1
                        arguments['output_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,output_handle)).read_text()
                    if name=='rc_character_token':
                        character=int(frame.read_var('character'));assert character<=256
                        arguments['token']=dict(character=character,formal_bytes=frame.read_var('character').type.sizeof,stack_slot_bytes=4,table_entry=int(gdb.parse_and_eval('rc_character_tokens[%d]'%character)),quote_cursor=int(frame.older().read_var('cursor')),quote_source=int(frame.older().read_var('path')))
                    if name=='rc_list_macro':
                        macro=frame.read_var('macro');marker=source_parent.read_var('marker')
                        arguments['marker']=dict(pointer=int(macro),object=int(marker.address),object_bytes=marker.type.sizeof,record=bytes(gdb.selected_inferior().read_memory(int(macro),16)).hex(),name=int(macro.dereference()['name']),name_bytes=observer.wide_bytes(int(macro.dereference()['name'])),text=int(macro.dereference()['text']),text_bytes=observer.wide_bytes(int(macro.dereference()['text'])),formal_bytes=macro.type.sizeof,parent=source_parent.name(),frame_pointer=int(source_parent.read_register('ebp')),marker_global=int(gdb.parse_and_eval('&rc_source_marker')),marker_global_bytes=gdb.parse_and_eval('rc_source_marker').type.sizeof)
                        arguments['output_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,int(gdb.parse_and_eval('recon_osfhnd[4]'))-1)).read_text()
                        arguments['refills']=json.loads((output.parent/'refills.json').read_text())
                        arguments['source_name_copies']=json.loads((output.parent/'source-name-copies.json').read_text())
                        arguments['source_formats']=json.loads((output.parent/'source-line-format.json').read_text())
                        arguments['output_writes']=json.loads((output.parent/'output-writes.json').read_text())
                    if name in ['rc_scan_line_start','rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        arguments['scan_line']=dict(formals=[],cursor=int(gdb.parse_and_eval('rc_source_cursor')),current_line=int(gdb.parse_and_eval('rc_preparing_line')))
                        arguments['macro_listings']=json.loads((output.parent/'macro-listings.json').read_text())
                        arguments['output_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,int(gdb.parse_and_eval('recon_osfhnd[4]'))-1)).read_text()
                        arguments['source_formats']=json.loads((output.parent/'source-line-format.json').read_text())
                        arguments['output_writes']=json.loads((output.parent/'output-writes.json').read_text())
                    if name in ['rc_handle_directive','rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        scanner=frame.older()
                        if name=='rc_intern_type':scanner=scanner.older()
                        if name=='rc_digit_value':scanner=scanner.older()
                        if name in ['rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:scanner=scanner.older()
                        if name in ['rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:scanner=scanner.older()
                        if name=='rc_character_class':scanner=scanner.older()
                        if name in ['rc_read_nonspace','rc_read_identifier','rc_character_class','rc_directive_code','rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:scanner=scanner.older()
                        arguments['directive']=dict(formals=[],scanner_character=int(scanner.read_var('character')),scanner_character_bytes=scanner.read_var('character').type.sizeof)
                    if name=='rc_read_nonspace':arguments['nonspace']=dict(formals=[])
                    if name=='rc_read_identifier':
                        arguments['identifier']=dict(character=int(frame.read_var('character')),formal_bytes=frame.read_var('character').type.sizeof,stack_slot_bytes=4,parent_character=int(frame.older().read_var('character')))
                    if name=='rc_character_class':
                        parent=frame.older();character=int(frame.read_var('character'));cursor=int(parent.read_var('cursor'))
                        arguments['identifier']=dict(character=int(parent.read_var('character')),formal_bytes=parent.read_var('character').type.sizeof,stack_slot_bytes=4,parent_character=int(parent.older().read_var('character')))
                        arguments['class']=dict(character=character,formal_bytes=frame.read_var('character').type.sizeof,stack_slot_bytes=4,table_entry=int(gdb.parse_and_eval('rc_character_classes[%d]'%character)),identifier_cursor=cursor,hash=int(parent.read_var('hash')))
                    if name=='rc_directive_code':
                        arguments['directive_word']=dict(pointer=int(frame.read_var('word')),bytes=observer.wide_bytes(int(frame.read_var('word'))),formal_bytes=frame.read_var('word').type.sizeof)
                        arguments['class_reads']=json.loads((output.parent/'class-reads.json').read_text())
                    if name in ['rc_next_token','rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        arguments['tokenizer']=dict(formals=[],previous_kind=int(gdb.parse_and_eval('rc_token_kind')),previous_value=int(gdb.parse_and_eval('rc_token_value')),header_mode=int(gdb.parse_and_eval('rc_header_mode')),condition_mode=int(gdb.parse_and_eval('rc_in_condition_expression')))
                        arguments['class_reads']=json.loads((output.parent/'class-reads.json').read_text())
                        arguments['directive_lookups']=json.loads((output.parent/'directive-lookups.json').read_text())
                    if name in ['rc_read_number','rc_get_logical_character','rc_parse_integer_value','rc_digit_value','rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                        number=frame if name=='rc_read_number'else frame.older()
                        if name in ['rc_digit_value','rc_intern_type']:number=number.older()
                        parent=number.older();assert parent.name()=='rc_next_token'
                        arguments['numeric_token']=dict(first=int(number.read_var('first')),formal_bytes=number.read_var('first').type.sizeof,stack_slot_bytes=4,parent_character=int(parent.read_var('character')),parent_class=int(parent.read_var('dispatch')),buffer_active=int(gdb.parse_and_eval('rc_number_buffer_active')))
                        if name=='rc_get_logical_character':arguments['logical_character']=dict(formals=[],begin=int(number.read_var('begin')),cursor=int(number.read_var('cursor')),radix=int(number.read_var('radix')),token=int(number.read_var('token')))
                        if name in ['rc_parse_integer_value','rc_digit_value']:
                            conversion=frame if name=='rc_parse_integer_value'else frame.older()
                            arguments['number_state']=dict(begin=int(number.read_var('begin')),cursor=int(number.read_var('cursor')),radix=int(number.read_var('radix')),token=int(number.read_var('token')))
                            arguments['integer_conversion']=dict(text=int(conversion.read_var('text')),text_bytes=observer.wide_bytes(int(conversion.read_var('text'))),radix=int(conversion.read_var('radix')),formal_bytes={field:conversion.read_var(field).type.sizeof for field in ['text','radix']})
                            arguments['logical_reads']=json.loads((output.parent/'logical-reads.json').read_text())
                            if name=='rc_digit_value':arguments['digit']=dict(character=int(frame.read_var('character')),formal_bytes=frame.read_var('character').type.sizeof,parent_cursor=int(conversion.read_var('cursor')),accumulator=int(conversion.read_var('value')),previous=int(conversion.read_var('previous')))
                        if name in ['rc_decimal_integer_kind','rc_store_integer','rc_intern_type']:
                            conversions=json.loads((output.parent/'integer-conversions.json').read_text());digits=json.loads((output.parent/'digit-values.json').read_text());assert len(conversions)==len(digits)==1
                            arguments['number_state']=dict(begin=int(number.read_var('begin')),cursor=int(number.read_var('cursor')),radix=int(number.read_var('radix')),token=int(number.read_var('token')))
                            arguments['integer_conversion']={key:conversions[0][key]for key in ['text','text_bytes','radix','formal_bytes']}
                            arguments['logical_reads']=json.loads((output.parent/'logical-reads.json').read_text())
                            arguments['digit']={key:digits[0][key]for key in ['character','formal_bytes','parent_cursor','accumulator','previous']}
                            arguments['integer_returns']=conversions;arguments['digit_values']=digits
                            if name=='rc_decimal_integer_kind':arguments['integer_kind']=dict(value=int(frame.read_var('value')),formal_bytes=frame.read_var('value').type.sizeof,parent_value=int(number.read_var('value')[0]),parent_object_bytes=number.read_var('value').type.sizeof,initialized_bytes=4)
                            else:
                                kinds=json.loads((output.parent/'integer-kinds.json').read_text());assert len(kinds)==1
                                arguments['integer_kind']=dict(value=kinds[0]['value'],formal_bytes=kinds[0]['formal_bytes'],parent_value=int(number.read_var('value')[0]),parent_object_bytes=number.read_var('value').type.sizeof,initialized_bytes=4)
                                arguments['integer_kinds']=kinds
                                store_frame=frame.older()if name=='rc_intern_type'else frame
                                value=store_frame.read_var('value');token=store_frame.read_var('token');owner=number.read_var('value')
                                arguments['store']=dict(token=int(token),value=int(value),formal_bytes=dict(token=token.type.sizeof,value=value.type.sizeof),payload=bytes(gdb.selected_inferior().read_memory(int(value),4)).hex(),owner=int(owner.address),object_bytes=owner.type.sizeof,initialized_bytes=4)

                    if name=='rc_intern_type':
                        store_frame=frame.older();node=store_frame.read_var('node');type_arg=frame.read_var('type')
                        buckets=gdb.parse_and_eval('rc_type_buckets')
                        arguments['type_buckets']=dict(address=int(buckets.address),bytes=buckets.type.sizeof,count=256,pointer_bytes=buckets[0].type.sizeof,payload=bytes(gdb.selected_inferior().read_memory(int(buckets.address),buckets.type.sizeof)).hex())
                        arguments['intern_type']=dict(pointer=int(type_arg),formal_bytes=type_arg.type.sizeof,object_bytes=type_arg.dereference().type.sizeof,payload=bytes(gdb.selected_inferior().read_memory(int(type_arg),20)).hex(),parent_type=int(store_frame.read_var('type')),node=int(node),node_bytes=bytes(gdb.selected_inferior().read_memory(int(node),20)).hex(),type_code=int(store_frame.read_var('type_code')),parent=store_frame.name())

                    file=gdb.parse_and_eval('rc_source_file').dereference();descriptor=int(file['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
                    arguments['source_host_path']=os.readlink('/proc/%d/fd/%d'%(gdb.selected_inferior().pid,handle))
                    arguments['source_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text()
                if name=='rc_open_source':
                    parent=frame.older();assert parent.name()=='rc_open_preprocessor'
                    arguments['open']={field:int(parent.read_var(field))&0xffffffff for field in ['input','output','defines','undefines']}
                    arguments['setup_iteration']=int(parent.read_var('i'))
                    arguments['source_open']={field:int(frame.read_var(field))for field in ['path','first']}
                    arguments['source_open']['path_bytes']=observer.wide_bytes(int(frame.read_var('path')))
                    arguments['source_open']['formal_bytes']={field:frame.read_var(field).type.sizeof for field in ['path','first']}
                    arguments['macro_copies']=json.loads((output.parent/'macro-copies.json').read_text())
                if name=='rc_copy_macro_text':
                    arguments['copy']={field:int(frame.read_var(field))for field in ['value','length']}
                    arguments['copy']['value_bytes']=observer.wide_bytes(int(frame.read_var('value')))
                    arguments['copy']['formal_bytes']={field:frame.read_var(field).type.sizeof for field in ['value','length']}
                    macro=frame.older().read_var('macro');arguments['pending_macro']=dict(pointer=int(macro),bytes=bytes(gdb.selected_inferior().read_memory(int(macro)-8,24)).hex(),record_bytes=macro.dereference().type.sizeof)
                if name in ['rc_find_macro','rc_copy_macro_text']:
                    parent=frame.older();assert parent.name()=='rc_define_macro'
                    setup=parent.older();assert setup.name()=='rc_open_preprocessor'
                    arguments['open']={field:int(setup.read_var(field))&0xffffffff for field in ['input','output','defines','undefines']}
                    arguments['setup_iteration']=int(setup.read_var('i'))
                    arguments['definition']={field:int(parent.read_var(field))for field in ['value','length','parameters']}
                    arguments['definition']['value_bytes']=observer.wide_bytes(int(parent.read_var('value')))if int(parent.read_var('value'))else None
                    arguments['definition']['formal_bytes']={field:parent.read_var(field).type.sizeof for field in ['value','length','parameters']}
                if name=='rc_define_macro':
                    parent=frame.older();assert parent.name()=='rc_open_preprocessor'
                    arguments['open']={field:int(parent.read_var(field))&0xffffffff for field in ['input','output','defines','undefines']}
                    arguments['setup_iteration']=int(parent.read_var('i'))
                    arguments['definition']={field:int(frame.read_var(field))for field in ['value','length','parameters']}
                    arguments['definition']['value_bytes']=observer.wide_bytes(int(frame.read_var('value')))if int(frame.read_var('value'))else None
                    arguments['definition']['formal_bytes']={field:frame.read_var(field).type.sizeof for field in ['value','length','parameters']}
                if name=='rc_hash_macro_name':
                    parent=frame.older();assert parent.name()=='rc_open_preprocessor'
                    arguments['open']={field:int(parent.read_var(field))&0xffffffff for field in ['input','output','defines','undefines']}
                    arguments['macro_name']=dict(pointer=int(frame.read_var('name')),bytes=observer.wide_bytes(int(frame.read_var('name'))))
                    arguments['setup_iteration']=int(parent.read_var('i'))
                if name in ['rc_set_character_token','rc_set_character_class']:
                    parent=frame.older();assert parent.name()=='rc_open_preprocessor'
                    arguments['open']={field:int(parent.read_var(field))&0xffffffff for field in ['input','output','defines','undefines']}
                    fields=['character','token'if name=='rc_set_character_token'else'flags']
                    arguments['character']={field:int(frame.read_var(field))for field in fields}
                    arguments['character']['formal_bytes']={field:frame.read_var(field).type.sizeof for field in fields}
        if name=='rc_notify_progress':
            parent=frame.older();assert parent.name()=='rc_prepare_input'
            arguments.update(kind=int(frame.read_var('kind')),value=int(frame.read_var('value')),current_line=int(gdb.parse_and_eval('rc_preparing_line')),towupper=json.loads((output.parent/'towupper.json').read_text()),prefix_results=json.loads((output.parent/'prefix-results.json').read_text()),line_format=json.loads((output.parent/'line-format.json').read_text()))
            for label,variable,reading in [('input','input',True),('output','output',False)]:
                file=parent.read_var(variable);state=file.dereference();start=int(state['_base']);size=int(state['_ptr'])-start+(int(state['_cnt'])if reading else 0);assert 0<=size<=4096
                arguments[label+'_state']=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex()
                arguments[label+'_buffer']=bytes(gdb.selected_inferior().read_memory(start,size)).hex()
            pointer=int(parent.read_var('line').address);line=bytearray()
            for index in range(4177):
                word=bytes(gdb.selected_inferior().read_memory(pointer+2*index,2));line.extend(word)
                if word==b'\0\0':break
            else:raise AssertionError('unterminated progress source line')
            arguments['line']=line.hex()
        if name=='rc_prefix_equal':
            def wide(pointer):
                result=bytearray()
                for i in range(4177):
                    word=bytes(gdb.selected_inferior().read_memory(int(pointer)+2*i,2));result.extend(word)
                    if word==b'\0\0':return result.hex()
                raise AssertionError('unterminated native wide string')
            parent=frame.older();assert parent.name()=='rc_prepare_input'
            file=parent.read_var('input');state=file.dereference();start=int(state['_base']);size=int(state['_ptr'])-start+int(state['_cnt']);assert 0<size<=4096
            arguments.update(file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),file_buffer=bytes(gdb.selected_inferior().read_memory(start,size)).hex())
            arguments.update(prefix=wide(frame.read_var('prefix')),text=wide(frame.read_var('text')),current_line=int(gdb.parse_and_eval('rc_preparing_line')),locals=json.loads((output.parent/'locals.json').read_text()),ctype=json.loads((output.parent/'ctype.json').read_text()))
        if name=='rc_read_wide_line':
            file=frame.read_var('file')
            arguments.update(capacity=int(frame.read_var('capacity')),wide=int(frame.read_var('wide')),file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),cpinfo=json.loads((output.parent/'cpinfo.json').read_text()),unicode_results=json.loads((output.parent/'unicode-results.json').read_text()))
        if name=='rc_is_text_unicode':
            sample=frame.read_var('sample');size=int(frame.read_var('bytes'));flags=frame.read_var('flags')
            parent=frame.older();assert parent.name()=='rc_detect_encoding'
            file=parent.read_var('file');state=file.dereference();start=int(state['_base']);length=int(state['_ptr'])-start+int(state['_cnt']);assert 0<=length<=4096
            arguments.update(sample=bytes(gdb.selected_inferior().read_memory(int(sample),size+2)).hex(),bytes=size,flags=int(flags.dereference()),position=int(parent.read_var('position')),file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),file_buffer=bytes(gdb.selected_inferior().read_memory(start,length)).hex())
        if name=='rc_detect_encoding':
            file=frame.read_var('file');parent=frame.older();assert parent.name()=='rc_prepare_input'
            output_file=parent.read_var('output');state=output_file.dereference()
            start=int(state['_base']);length=int(state['_ptr'])-start;assert 0<=length<=8196
            arguments.update(input_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),output_state=bytes(gdb.selected_inferior().read_memory(int(output_file),32)).hex(),output_buffer=bytes(gdb.selected_inferior().read_memory(start,length)).hex(),write_returns=json.loads((output.parent/'writes.json').read_text()))
        if name=='rc_write_bytes':
            file=frame.read_var('file');data=frame.read_var('data');size=int(frame.read_var('bytes'))
            assert 0<size<=8196
            arguments.update(file=int(file),data=int(data),bytes=size,payload=bytes(gdb.selected_inferior().read_memory(int(data),size)).hex(),file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex())
            parent=frame.older();assert parent.name()=='rc_prepare_input'
            sp=registers['esp'];bp=int(parent.read_register('ebp'))&0xffffffff
            gdb.execute('printf "RCDLL_PARENT_FRAME=%08x %08x %08x\\n", '+str(int(parent.pc())&0xffffffff)+', '+str(sp)+', '+str(bp))
            objects=[]
            for field,size in [('escaped',520),('directive',8196),('line',8354)]:
                value=parent.read_var(field);address=int(value.address)&0xffffffff
                assert value.type.sizeof==size and sp<=address<address+size<=bp
                objects.append(dict(name=field,address=address,bytes=size))
                gdb.execute('printf "RCDLL_FRAME_OBJECT '+field+'=%08x %u\\n", '+str(address)+', '+str(size))
            maps=Path('/proc/%d/maps'%gdb.selected_inferior().pid).read_text().splitlines()
            stack=[line for line in maps if line.endswith('[stack]')];assert len(stack)==1
            low,high=[int(value,16)for value in stack[0].split()[0].split('-')];assert low<=sp<bp<high
            ordered=sorted(objects,key=lambda value:value['address'])
            assert all(a['address']+a['bytes']<=b['address']for a,b in zip(ordered,ordered[1:]))
            descriptor=int(file.dereference()['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            arguments.update(parent_frame=dict(caller=parent.name(),pc=int(parent.pc())&0xffffffff,esp=sp,ebp=bp,objects=objects,stack_mapping=stack[0]),file_host_path=os.readlink('/proc/%d/fd/%d'%(gdb.selected_inferior().pid,handle)),file_fdinfo=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text(),filename=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('rc_filename')),514)).hex())
        if name=='rc_format':
            arguments['capacity']=int(frame.read_var('capacity'))
            template=frame.read_var('format').cast(gdb.lookup_type('char').pointer())
            arguments['template']=template.string()
            base=int(gdb.parse_and_eval('$ebp'))
            version=int.from_bytes(bytes(gdb.selected_inferior().read_memory(base+20,4)),'little')
            arguments['version']=gdb.Value(version).cast(gdb.lookup_type('char').pointer()).string()
            arguments['build']=int.from_bytes(bytes(gdb.selected_inferior().read_memory(base+24,4)),'little')
            arguments['output']=int(frame.read_var('output'))&0xffffffff
            arguments['expected_output']=int(gdb.parse_and_eval('&rc_error_text'))&0xffffffff
        module['thread_calls_disabled']=bool(gdb.parse_and_eval('fixture_thread_calls_disabled'))
        heap_identity=None
        token=int(gdb.parse_and_eval('rc_heap'))&0xffffffff
        if token:
            cursor=gdb.parse_and_eval('native_heaps')
            while int(cursor):
                if int(cursor)==token:
                    item=cursor.dereference();blocks=[];block=item['blocks']
                    while int(block):
                        record=block.dereference();size=int(record['requested_size'])
                        blocks.append(dict(owner=int(record['owner'])&0xffffffff,payload=int(record['payload'])&0xffffffff,bytes=size,flags=int(record['flags']),contents=bytes(gdb.selected_inferior().read_memory(int(record['payload']),size)).hex()))
                        block=record['next']
                    heap_identity=dict(handle=token,options=int(item['options']),initial=int(item['initial_size']),maximum=int(item['maximum_size']),blocks=blocks)
                    break
                cursor=cursor.dereference()['next']
        heap=json.loads((output.parent/'heap.json').read_text()) if (output.parent/'heap.json').exists() else []
        environment=[];environment_vector=gdb.parse_and_eval('fixture_target_environment')
        if int(environment_vector):
            for index in range(16385):
                if not int(environment_vector[index]):break
                environment.append(environment_vector[index].string())
            else:raise AssertionError('unterminated target environment')
        case_inputs=dict(environment=environment,host_cwd=gdb.parse_and_eval('fixture_host_cwd').string(),include=gdb.parse_and_eval('fixture_input_include').string()if int(gdb.parse_and_eval('fixture_input_include'))else None,codepage=gdb.parse_and_eval('fixture_input_codepage').string()if int(gdb.parse_and_eval('fixture_input_codepage'))else None)
        rich={}
        if name=='rc_prepare_input':
            rich['buffers']={}
            for field,size in [('rc_output_path',260),('rc_input_path',260),('rc_input_name',260),('rc_preprocessed_path',260),('rc_notice_text',520)]:
                value=gdb.parse_and_eval(field)
                rich['buffers'][field]=bytes(gdb.selected_inferior().read_memory(int(value.address),size)).hex()
            rich['temporaries']={field:gdb.parse_and_eval(field).string()for field in ['rc_input_temporary','rc_output_temporary']}
            count=int(frame.older().read_var('argument_count'));assert 0<count<=256
            rich['preprocessor_argv']=[gdb.parse_and_eval('rc_preprocessor_arguments[%d]'%index).string()for index in range(count)]
            file=gdb.parse_and_eval('rc_output_file');rich['output_file']=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex()
            descriptor=int(file.dereference()['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            rich['output_file_host_path']=os.readlink('/proc/%d/fd/%d'%(gdb.selected_inferior().pid,handle))
            rich['output_file_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text()
        result=dict(rich=rich,case_inputs=case_inputs,export_vector=vector,heap_identity=heap_identity,heap=heap,diagnostic_callback=int(gdb.parse_and_eval("(void *)fixture_diagnostic"))&0xffffffff,jump=jump,exit_array=exit_array,allocations=json.loads((output.parent/'allocations.json').read_text()),module=module,globals=globals,kind='completion' if complete else 'frontier',function=raw_name,rva=f['start'],raw_pc=pc,executed_byte=instruction.hex(),registers=registers,arguments=arguments,config_sha256=hashlib.sha256(config_path.read_bytes()).hexdigest())
        result['crt_errors']=dict(errno=int(gdb.parse_and_eval('*(int *)&errno')),last_error=int(gdb.parse_and_eval('last_error')))
    except Exception as error:
        import traceback
        result=dict(kind='error',error=str(error),traceback=traceback.format_exc())
    output.write_text(json.dumps(result,indent=2)+'\n')

if gdb.selected_inferior().pid:
    capture()
else:
    assert output.exists() and json.loads(output.read_text())["kind"]=="completion", "inferior exited without a completion state"
