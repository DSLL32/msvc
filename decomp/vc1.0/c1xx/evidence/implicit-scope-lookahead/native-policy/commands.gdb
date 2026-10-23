set pagination off
break begin_implicit_statement_scope
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/scope-native-policy-bzqfz0xz/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/scope-native-policy-bzqfz0xz/stderr.bin
python
import gdb, json
assert gdb.newest_frame().name() == "begin_implicit_statement_scope", "unexpected earlier stop"
assert int(gdb.parse_and_eval("parser_lookahead")) == -1, "valid input did not establish the normal lookahead state"
end
set var parser_lookahead = 2
disable 1
continue
python
frame = gdb.newest_frame()
assert frame.name() == "begin_implicit_statement_scope", "unsupported branch did not trap in scope entry"
assert int(gdb.parse_and_eval("$_siginfo.si_signo")) == 5, "unsupported branch did not raise SIGTRAP"
assert int(gdb.parse_and_eval("parser_lookahead")) == 2, "injected lookahead changed"
pc = frame.pc()
entry = int(gdb.parse_and_eval("(unsigned int)begin_implicit_statement_scope"))
assert pc > entry and bytes(gdb.selected_inferior().read_memory(pc-1,1)) == b"\xcc", "expected an interior inline INT3"
result = {"status": "unsupported native path rejected", "function": frame.name(), "injected_lookahead": 2, "signal": 5, "pc_offset": pc-entry, "preceding_instruction": "cc"}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/scope-native-policy-bzqfz0xz/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
