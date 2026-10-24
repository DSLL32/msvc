break convert_compact_data_member_pointer
python
import gdb, json
exit_codes = []
gdb.events.exited.connect(lambda event: exit_codes.append(getattr(event, "exit_code", None)))
end
run > /root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1225-convert_compact_data_member_pointer-24/native-raw/stdout.bin 2> /root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1225-convert_compact_data_member_pointer-24/native-raw/stderr.bin
python
if exit_codes:
    assert len(exit_codes) == 1 and exit_codes[0] is not None, "missing normal exit status"
    result = {"buffers": [], "states": [], "function": "exit:" + str(exit_codes[0]), "signal": 0, "exit_code": exit_codes[0]}
else:
    gdb.execute("bt")
    result = {"buffers": [], "states": [], "function": gdb.newest_frame().name(), "signal": int(gdb.parse_and_eval("$_siginfo.si_signo"))}
    block = gdb.newest_frame().block()
    arguments = []
    while block is not None:
        arguments += [symbol.name for symbol in block if symbol.is_argument]
        if block.function is not None: break
        block = block.superblock
    result["declared_arguments"] = sorted(arguments)
    result['raw_expression'] = int(gdb.parse_and_eval('(unsigned)expression'))
    result['raw_type'] = int(gdb.parse_and_eval('(unsigned)type'))
    result['raw_path'] = int(gdb.parse_and_eval('(unsigned)path'))
    result['raw_forward'] = int(gdb.parse_and_eval('(unsigned)forward'))
    result['raw_displacement'] = int(gdb.parse_and_eval('(unsigned)displacement'))
    result['raw_virtual_base'] = int(gdb.parse_and_eval('(unsigned)virtual_base'))
    result["caller_locals"] = {}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'convert_member_pointer_representation'
    owner_value = owner.read_var('expression')
    result["caller_locals"]['caller_expression'] = {"function": owner.name(), "local": 'expression', "fields": [], "value": int(owner_value)}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'convert_member_pointer_representation'
    owner_value = owner.read_var('type')
    result["caller_locals"]['caller_type'] = {"function": owner.name(), "local": 'type', "fields": [], "value": int(owner_value)}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'convert_member_pointer_representation'
    owner_value = owner.read_var('source_model')
    result["caller_locals"]['caller_source_model'] = {"function": owner.name(), "local": 'source_model', "fields": [], "value": int(owner_value)}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'convert_member_pointer_representation'
    owner_value = owner.read_var('target_model')
    result["caller_locals"]['caller_target_model'] = {"function": owner.name(), "local": 'target_model', "fields": [], "value": int(owner_value)}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'convert_member_pointer_representation'
    owner_value = owner.read_var('data_member')
    result["caller_locals"]['caller_data_member'] = {"function": owner.name(), "local": 'data_member', "fields": [], "value": int(owner_value)}
    stream = gdb.parse_and_eval("expression_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else "")
    stream = gdb.parse_and_eval("symbol_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else "")
    stream = gdb.parse_and_eval("line_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else "")
    stream = gdb.parse_and_eval("global_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else "")
    stream = gdb.parse_and_eval("debug_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else "")
    value = int(gdb.parse_and_eval("*(unsigned int *)&compiler_stack"))
    result["states"].append(int(value != 0) if True else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&initializer_record_stack"))
    result["states"].append(int(value != 0) if True else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&macro_expansion_active"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&captured_class_error_count"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&current_class_context"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&declaration_control"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&token_allocation_flags"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&cpp_parser_flags"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&next_symbol_index"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&ir_record_sequence"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&pending_constructor_initializers"))
    result["states"].append(int(value != 0) if True else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&initializing_symbol"))
    result["states"].append(int(value != 0) if True else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&initializing_reference"))
    result["states"].append(int(value != 0) if False else value)
    value = int(gdb.parse_and_eval("*(unsigned int *)&member_initializer_active"))
    result["states"].append(int(value != 0) if False else value)
open('/root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1225-convert_compact_data_member_pointer-24/native-raw/checkpoint.json',"w").write(json.dumps(result, indent=2)+"\n")
end
