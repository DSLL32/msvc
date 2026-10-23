set pagination off
break member_pointer_expression
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/member-pointer-model-policy-2plkr9i_/method-model1/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/member-pointer-model-policy-2plkr9i_/method-model1/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="member_pointer_expression"
qualifier=gdb.parse_and_eval("qualifier")
binding=qualifier["resolution"]["binding"]
member=binding["target"].cast(gdb.lookup_type("CppSymbol").pointer())
assert (int(member["kind"])&31)==7 and member["name"].string()=="add"
assert not (int(member["storage"][1])&2)
end
disable 1
break member_pointer_inheritance_model
continue
python
assert gdb.newest_frame().name()=="member_pointer_inheritance_model"
end
disable 2
finish
python
assert gdb.newest_frame().name()=="member_pointer_inheritance_model"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
pc=gdb.newest_frame().pc()
assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
end
finish
python
assert gdb.newest_frame().name()=="member_pointer_expression"
end
set $eax = 1
break make_compact_method_pointer
continue
python
assert gdb.newest_frame().name()=='make_compact_method_pointer'
result={"branch":'method',"injected_model":1,"helper_was_unaccepted_stub":True,"stopped_function":gdb.newest_frame().name(),"status":"pass"}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/member-pointer-model-policy-2plkr9i_/method-model1/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
