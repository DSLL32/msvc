break search_source_include
python
import gdb, json
exit_codes = []
gdb.events.exited.connect(lambda event: exit_codes.append(getattr(event, "exit_code", None)))
end
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/include-name-owner-before-q5hls9fq/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/include-name-owner-before-q5hls9fq/stderr.bin
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
    result['filename'] = int(gdb.parse_and_eval('(unsigned)filename'))
    result['token_begin'] = int(gdb.parse_and_eval('(unsigned)&token_buffer[0]'))
    result['token_end'] = int(gdb.parse_and_eval('(unsigned)(token_buffer+sizeof(token_buffer))'))
    result['source_cursor'] = int(gdb.parse_and_eval('(unsigned)source_text_cursor'))
    stream = gdb.parse_and_eval("expression_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(None if not int(stream) else (bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else ""))
    stream = gdb.parse_and_eval("symbol_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(None if not int(stream) else (bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else ""))
    stream = gdb.parse_and_eval("line_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(None if not int(stream) else (bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else ""))
    stream = gdb.parse_and_eval("global_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(None if not int(stream) else (bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else ""))
    stream = gdb.parse_and_eval("debug_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(None if not int(stream) else (bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else ""))
    stream = gdb.parse_and_eval("browser_file")
    length = int(stream["_ptr"]) - int(stream["_base"]) if int(stream) else 0
    result["buffers"].append(None if not int(stream) else (bytes(gdb.selected_inferior().read_memory(int(stream["_base"]), length)).hex() if length else ""))
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
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/include-name-owner-before-q5hls9fq/checkpoint.json',"w").write(json.dumps(result, indent=2)+"\n")
end
