run > /root/.ai-tmp/vc1.0-c1xx/native-frontier-vzyrfiue/stdout.bin 2> /root/.ai-tmp/vc1.0-c1xx/native-frontier-vzyrfiue/stderr.bin
bt
python
import gdb, json
result = {"buffers": [], "states": [], "function": gdb.newest_frame().name(), "signal": int(gdb.parse_and_eval("$_siginfo.si_signo"))}
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
open('/root/.ai-tmp/vc1.0-c1xx/native-frontier-vzyrfiue/checkpoint.json',"w").write(json.dumps(result, indent=2)+"\n")
end
