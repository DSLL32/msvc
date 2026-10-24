"""Encountered C1XX parser fields; no unspecified snapshot storage is represented."""
import json,struct
from pathlib import Path

# name, original RVA, scalar/array type, bytes, reset lifetime.
SPECS = [
    ('edit_and_continue', 0xbf48c, 'int', 4, 'bss'),
    ('option_zn', 0xbf4a8, 'int', 4, 'bss'),
    ('include_directories', 0xbf6f0, 'struct CompilerVector *', 4, 'bss'),
    ('source_file', 0xbf6f8, 'char *', 4, 'bss'),
    ('option_zf', 0xc0490, 'int', 4, 'bss'),
    ('version_string', 0xc0558, 'char *', 4, 'bss'),
    ('forced_includes', 0xc0584, 'struct CompilerVector *', 4, 'bss'),
    ('show_includes', 0xc0588, 'int', 4, 'bss'),
    ('defines', 0xc06c4, 'struct CompilerVector *', 4, 'bss'),
    ('prototypes_only', 0xc06d8, 'int', 4, 'bss'),
    ('preprocessed_file', 0xc07ec, 'char *', 4, 'bss'),
    ('browser_file', 0xc0838, 'char *', 4, 'bss'),
    ('option_binl', 0xc0974, 'int', 4, 'bss'),
    ('preprocess_stdout', 0xc0978, 'int', 4, 'bss'),
    ('browser_local_file', 0xc097c, 'char *', 4, 'bss'),
    ('preserve_comments', 0xc0ab0, 'int', 4, 'bss'),
    ('dump_options', 0xc0ac0, 'int', 4, 'bss'),
    ('option_xc', 0xc0ae8, 'int', 4, 'bss'),
    ('warnings_are_errors', 0xc0b10, 'int', 4, 'bss'),
    ('suppress_banner', 0xc0b6c, 'int', 4, 'bss'),
    ('option_zc', 0xc0b74, 'int', 4, 'bss'),
    ('fat', 0xc0b78, 'int', 4, 'bss'),
    ('pch_memory_limit', 0xc1700, 'int', 4, 'bss'),
    ('response_file', 0xc1710, 'char *', 4, 'bss'),
    ('error_file', 0xc5390, 'char *', 4, 'bss'),
    ('argument_allocation', 0xcc398, 'char **', 4, 'initialized'),
    ('packing_stack', 0xcc440, 'struct CompilerPacking *', 4, 'initialized'),
    ('include_aliases', 0xcc444, 'struct CompilerIncludeAliases *', 4, 'initialized'),
    ('warning_levels', 0xccb84, 'unsigned char', 1000, 'initialized'),
    ('lowest_warning', 0xccf6c, 'unsigned', 4, 'initialized'),
    ('highest_warning', 0xccf70, 'unsigned', 4, 'initialized'),
    ('preprocess_only', 0xcd090, 'int', 4, 'initialized'),
    ('language_extensions', 0xcd094, 'int', 4, 'initialized'),
    ('option_zbe', 0xcd098, 'int', 4, 'initialized'),
    ('fastcall_calls', 0xcd0a0, 'int', 4, 'initialized'),
    ('japanese_identifiers', 0xcd0a4, 'int', 4, 'initialized'),
    ('unsigned_char', 0xcd0a8, 'int', 4, 'initialized'),
    ('member_pointer_best_case', 0xcd0ac, 'int', 4, 'initialized'),
    ('member_pointer_general', 0xcd0b0, 'int', 4, 'initialized'),
    ('member_pointer_single', 0xcd0b4, 'int', 4, 'initialized'),
    ('member_pointer_multiple', 0xcd0b8, 'int', 4, 'initialized'),
    ('member_pointer_virtual', 0xcd0bc, 'int', 4, 'initialized'),
    ('vtordisp', 0xcd0c0, 'int', 4, 'initialized'),
    ('dll_runtime', 0xcd0c4, 'int', 4, 'initialized'),
    ('multithread_runtime', 0xcd0c8, 'int', 4, 'initialized'),
    ('pascal_calls', 0xcd0cc, 'int', 4, 'initialized'),
    ('debug_info', 0xcd0d0, 'int', 4, 'initialized'),
    ('pch_debug', 0xcd0d4, 'int', 4, 'initialized'),
    ('preprocess_no_lines', 0xcd0dc, 'int', 4, 'initialized'),
    ('option_pf', 0xcd0e0, 'int', 4, 'initialized'),
    ('option_bu', 0xcd0e4, 'int', 4, 'initialized'),
    ('reproducible', 0xcd0e8, 'int', 4, 'initialized'),
    ('intrinsics', 0xcd0f0, 'int', 4, 'initialized'),
    ('stack_checks', 0xcd0f4, 'int', 4, 'initialized'),
    ('cdecl_calls', 0xcd100, 'int', 4, 'initialized'),
    ('stdcall_calls', 0xcd104, 'int', 4, 'initialized'),
    ('pool_strings', 0xcd108, 'int', 4, 'initialized'),
    ('readonly_strings', 0xcd110, 'int', 4, 'initialized'),
    ('code_based_strings', 0xcd118, 'int', 4, 'initialized'),
    ('processor', 0xcd11c, 'int', 4, 'initialized'),
    ('minimal_rebuild', 0xcd120, 'int', 4, 'initialized'),
    ('inline_level', 0xcd124, 'int', 4, 'initialized'),
    ('exception_handling', 0xcd138, 'int', 4, 'initialized'),
    ('runtime_type_info', 0xcd13c, 'int', 4, 'initialized'),
    ('no_bool', 0xcd140, 'int', 4, 'initialized'),
    ('dependency_output', 0xcd148, 'int', 4, 'initialized'),
    ('incremental_rebuild', 0xcd14c, 'int', 4, 'initialized'),
    ('optimize_speed', 0xcd154, 'int', 4, 'initialized'),
    ('assume_no_alias', 0xcd158, 'int', 4, 'initialized'),
    ('assume_no_global_alias', 0xcd15c, 'int', 4, 'initialized'),
    ('global_optimize', 0xcd160, 'int', 4, 'initialized'),
    ('precise_float', 0xcd164, 'int', 4, 'initialized'),
    ('omit_frame_pointer', 0xcd168, 'int', 4, 'initialized'),
    ('line_records', 0xcd16c, 'int', 4, 'initialized'),
    ('database_path', 0xcd170, 'char', 256, 'initialized'),
    ('pch_link_path', 0xcd270, 'char', 256, 'initialized'),
    ('option_oq', 0xcd474, 'int', 4, 'initialized'),
    ('option_ov', 0xcd47c, 'int', 4, 'initialized'),
    ('incremental_compile', 0xcd48c, 'int', 4, 'initialized'),
    ('debug_types', 0xcd490, 'int', 4, 'initialized'),
    ('function_records', 0xcd498, 'int', 4, 'initialized'),
    ('split_pdbs', 0xcd49c, 'int', 4, 'initialized'),
    ('no_eh_for_new', 0xcd4a0, 'int', 4, 'initialized'),
    ('incremental_rebuild_forced', 0xcd4a4, 'int', 4, 'initialized'),
    ('extern_c_no_throw', 0xcd4a8, 'int', 4, 'initialized'),
    ('pch_use', 0xcd4b0, 'int', 4, 'initialized'),
    ('pch_use_file', 0xcd4b4, 'char *', 4, 'initialized'),
    ('pch_create', 0xcd4b8, 'int', 4, 'initialized'),
    ('pch_create_file', 0xcd4bc, 'char *', 4, 'initialized'),
    ('pch_auto', 0xcd4c0, 'int', 4, 'initialized'),
    ('pch_auto_file', 0xcd4c4, 'char *', 4, 'initialized'),
    ('pch_link_name', 0xcd4c8, 'char *', 4, 'initialized'),
    ('pch_file', 0xcd4cc, 'char *', 4, 'initialized'),
    ('pch_no_preprocess', 0xcd4e4, 'int', 4, 'initialized'),
    ('new_version', 0xcd51c, 'char *', 4, 'initialized'),
    ('object_file', 0xcd530, 'char *', 4, 'initialized'),
    ('database_file', 0xcd534, 'char *', 4, 'initialized'),
    ('warning_level', 0xcd53c, 'int', 4, 'initialized'),
    ('path_separators', 0xcd568, 'char *', 4, 'initialized'),
    ('il_prefix', 0xcd56c, 'char *', 4, 'initialized'),
    ('packing_alignment', 0xcd570, 'int', 4, 'initialized'),
    ('no_expand_inline', 0xcd59c, 'int', 4, 'initialized'),
    ('unrecognized_argument', 0xcd5d8, 'char *', 4, 'initialized'),
    ('debug_old_format', 0xcd5dc, 'int', 4, 'initialized'),
    ('move_pch', 0xcdc24, 'int', 4, 'initialized'),
    ('option_bcmpconv', 0xcdc5c, 'int', 4, 'initialized'),
 ]


def known():
    result={'options.'+name:(0x10400000+rva,width,kind,'frontend_'+('data'if kind=='initialized'else'bss')+'.options.'+name)for name,rva,typ,width,kind in SPECS}
    result['current_pdb_context']=(0x104cb000,4,'external','current_pdb_context')
    return result


def validate_layout(require,observed):
    for kind,key,base in [('initialized','options_data',observed['storage']['first_invocation'][0]+200),('bss','options_bss',observed['bss_address']+1404)]:
        offset=0;layout={}
        for name,rva,typ,width,lifetime in SPECS:
            if lifetime!=kind:continue
            wanted=('unsigned int'if typ=='unsigned'else typ).replace('struct ','')+(' [%d]'%width if width>4 else'')
            layout[name]=[offset,width,wanted]
            require(observed['storage']['options.'+name]==[base+offset,width],'option storage/type alias differs')
            offset+=width
        require(observed[key+'_layout']=={'size':offset,'fields':layout},'option semantic struct contains an unknown field/padding/type')
        require(set(observed[key])==set(layout),'option value inventory differs')
    require(observed['options_data_layout']['size']==1824 and observed['options_bss_layout']['size']==100,'option group size differs')


def gdb_collection():
    # Read two complete, independently checked semantic structs, not one GDB
    # expression per option. No mutable value or address is cached.
    return r'''for option_key,option_expression,option_type in [('options_data','frontend_data.options','CompilerOptionData'),('options_bss','frontend_bss.options','CompilerOptionBss')]:
    option_object=gdb.parse_and_eval(option_expression);option_base=int(option_object.address);option_type_object=gdb.lookup_type(option_type)
    option_raw=gdb.selected_inferior().read_memory(option_base,int(option_type_object.sizeof)).tobytes()
    option_fields=option_type_object.fields();option_values={};option_layout={}
    for option_field in option_fields:
        option_offset=option_field.bitpos//8;option_width=int(option_field.type.sizeof);option_bytes=option_raw[option_offset:option_offset+option_width]
        option_value=list(option_bytes)if option_field.type.code==gdb.TYPE_CODE_ARRAY else struct.unpack('<i'if str(option_field.type)=='int'else'<I',option_bytes)[0]
        option_values[option_field.name]=option_value;option_layout[option_field.name]=[option_offset,option_width,str(option_field.type).replace('struct ','')]
        value['frontend']['storage']['options.'+option_field.name]=[option_base+option_offset,option_width]
    value['frontend'][option_key]=option_values
    value['frontend'][option_key+'_layout']={'size':int(option_type_object.sizeof),'fields':option_layout}
    if option_key=='options_data':
        option_saved=value['frontend']['saved_state'];option_offset_from_data=option_base-int(gdb.parse_and_eval('(unsigned int)&frontend_data'))
        value['frontend']['saved_options_data']=None
        if option_saved:
            option_saved_raw=gdb.selected_inferior().read_memory(option_saved+option_offset_from_data,int(option_type_object.sizeof)).tobytes();option_saved_values={}
            for option_field in option_fields:
                option_offset=option_field.bitpos//8;option_width=int(option_field.type.sizeof);option_bytes=option_saved_raw[option_offset:option_offset+option_width]
                option_saved_values[option_field.name]=list(option_bytes)if option_field.type.code==gdb.TYPE_CODE_ARRAY else struct.unpack('<i'if str(option_field.type)=='int'else'<I',option_bytes)[0]
            value['frontend']['saved_options_data']=option_saved_values
value['frontend']['current_pdb_context']=int(gdb.parse_and_eval('(unsigned int)current_pdb_context'))
value['frontend']['storage']['current_pdb_context']=[int(gdb.parse_and_eval('(unsigned int)&current_pdb_context')),4]
'''


def gdb_seed():
    """Declared data-only restore auxiliary at the true initializer entry."""
    code="python\noption_seed_before={};option_owned=int(gdb.parse_and_eval('(unsigned int)frontend_bss.saved_state'));option_inf=gdb.selected_inferior()\n"
    code+="assert option_owned\n"
    for index,(name,rva,typ,width,kind)in enumerate(SPECS):
        expression=('frontend_data'if kind=='initialized'else'frontend_bss')+'.options.'+name
        code+="option_obj=gdb.parse_and_eval(%r);option_address=int(option_obj.address);option_seed_before[%r]=option_inf.read_memory(option_address,%d).tobytes().hex()\n"%(expression,name,width)
        code+="option_live="+("bytes([0xa5])*%d"%width if width>4 else "struct.pack('<I',"+("option_owned+%d"%(4*(index+1)+(512 if kind=='initialized'else 1024))if '*'in typ else str(101+index))+")")+";option_inf.write_memory(option_address,option_live)\n"
        if kind=='initialized':
            code+="option_offset=option_address-int(gdb.parse_and_eval('(unsigned int)&frontend_data'));option_saved="+("bytes([0x5a])*%d"%width if width>4 else "struct.pack('<I',"+("option_owned+%d"%(4*(index+1))if '*'in typ else str(17+index))+")")+";option_inf.write_memory(option_owned+option_offset,option_saved)\n"
    code+="option_seed_before['current_pdb_context']=int(gdb.parse_and_eval('(unsigned int)current_pdb_context'));gdb.execute('set variable current_pdb_context=&input_pdb_context')\n"
    code+="open(@@SEED_PATH@@,'w').write(json.dumps(option_seed_before,indent=2)+'\\n')\nend\n"
    return code
