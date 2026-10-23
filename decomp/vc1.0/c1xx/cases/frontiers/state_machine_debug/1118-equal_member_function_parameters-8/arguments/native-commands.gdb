break equal_member_function_parameters
python
import gdb, json
exit_codes = []
gdb.events.exited.connect(lambda event: exit_codes.append(getattr(event, "exit_code", None)))
end
run > /root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1118-equal_member_function_parameters-8/native-raw/stdout.bin 2> /root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1118-equal_member_function_parameters-8/native-raw/stderr.bin
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
    result['raw_source'] = int(gdb.parse_and_eval('(unsigned)source'))
    result['raw_target'] = int(gdb.parse_and_eval('(unsigned)target'))
    result["caller_locals"] = {}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'classify_member_pointer_conversion'
    owner_value = owner.read_var('source_parameters')
    result["caller_locals"]['caller_source'] = {"function": owner.name(), "local": 'source_parameters', "fields": [], "value": int(owner_value)}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'classify_member_pointer_conversion'
    owner_value = owner.read_var('target_shape')
    parameter_root=int(owner_value)
    assert 0 < parameter_root <= 0xffffffff-12 and owner_value.dereference().type.sizeof==12
    parameter_signature=owner_value["related"]
    assert int(parameter_signature.address)==parameter_root+4 and parameter_signature.type.sizeof==4
    assert 0 < int(parameter_signature) <= 0xffffffff-12 and parameter_signature.dereference().type.sizeof==12
    parameter_record=parameter_signature["data"]["parameters"]
    assert int(parameter_record.address)==int(parameter_signature)+8 and parameter_record.type.sizeof==4
    assert int(parameter_record)!=0
    parameter_path=[dict(address=int(parameter_signature.address),value=int(parameter_signature),bytes=4,read_bytes=4,read_succeeded=True),dict(address=int(parameter_record.address),value=int(parameter_record),bytes=4,read_bytes=4,read_succeeded=True)]
    owner_value=parameter_record
    result["caller_locals"]['caller_target'] = {"function": owner.name(), "local": 'target_shape', "fields": [], "value": int(owner_value)}
    result["caller_locals"]['caller_target'].update(member_parameter_path=True,root=parameter_root,pointer_path=parameter_path)
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
open('/root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1118-equal_member_function_parameters-8/native-raw/checkpoint.json',"w").write(json.dumps(result, indent=2)+"\n")
end
