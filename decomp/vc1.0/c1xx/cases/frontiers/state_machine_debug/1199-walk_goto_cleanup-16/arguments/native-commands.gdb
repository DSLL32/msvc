break walk_goto_cleanup
python
import gdb, json
exit_codes = []
gdb.events.exited.connect(lambda event: exit_codes.append(getattr(event, "exit_code", None)))
end
run > /root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1199-walk_goto_cleanup-16/native-raw/stdout.bin 2> /root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1199-walk_goto_cleanup-16/native-raw/stderr.bin
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
    result['raw_record'] = int(gdb.parse_and_eval('(unsigned)record'))
    result['raw_mode'] = int(gdb.parse_and_eval('(unsigned)mode'))
    result['raw_label'] = int(gdb.parse_and_eval('(unsigned)label'))
    result['label_tail'] = int(gdb.parse_and_eval('(unsigned)ir_record_tail'))
    result['tail_kind'] = int(gdb.parse_and_eval('(unsigned char)ir_record_tail->kind'))
    result['label_record_bytes'] = int(gdb.parse_and_eval('sizeof(LabelFrontendRecord)'))
    result['tail_previous_address'] = int(gdb.parse_and_eval('(unsigned)&ir_record_tail->previous'))
    result['tail_label_address'] = int(gdb.parse_and_eval('(unsigned)&((LabelFrontendRecord *)ir_record_tail)->label'))
    result['tail_previous'] = int(gdb.parse_and_eval('(unsigned)ir_record_tail->previous'))
    result['tail_label'] = int(gdb.parse_and_eval('(unsigned)((LabelFrontendRecord *)ir_record_tail)->label'))
    result['pointee_expression'] = int(gdb.parse_and_eval('(unsigned)*expression'))
    result["caller_locals"] = {}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'resolve_goto_cleanup'
    owner_value = owner.read_var('expression')
    result["caller_locals"]['caller_expression'] = {"function": owner.name(), "local": 'expression', "fields": [], "value": int(owner_value)}
    result["caller_locals"]['caller_expression']["address"] = int(owner_value.address)
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'resolve_goto_cleanup'
    owner_value = owner.read_var('to')
    result["caller_locals"]['caller_to'] = {"function": owner.name(), "local": 'to', "fields": [], "value": int(owner_value)}
    owner = gdb.newest_frame().older()
    assert owner is not None and owner.name() == 'resolve_goto_cleanup'
    owner_value = owner.read_var('label')
    result["caller_locals"]['caller_label'] = {"function": owner.name(), "local": 'label', "fields": [], "value": int(owner_value)}
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
open('/root/msvc2/decomp/vc1.0/c1xx/cases/argument-observations/state_machine_debug/1199-walk_goto_cleanup-16/native-raw/checkpoint.json',"w").write(json.dumps(result, indent=2)+"\n")
end
