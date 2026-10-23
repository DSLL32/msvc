set pagination off
break member_pointer_expression
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/member-pointer-model-policy-2plkr9i_/data-model0/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/member-pointer-model-policy-2plkr9i_/data-model0/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="member_pointer_expression"
qualifier=gdb.parse_and_eval("qualifier")
binding=qualifier["resolution"]["binding"]
member=binding["target"].cast(gdb.lookup_type("CppSymbol").pointer())
assert (int(member["kind"])&31)==7 and member["name"].string()=="add"
assert not (int(member["storage"][1])&2)
owner=binding["next"]["target"].cast(gdb.lookup_type("CppSymbol").pointer())
assert (int(owner["kind"])&31)==4 and owner["name"].string()=="Counter"
data=owner["auxiliary"]["scope_members"]
for index in range(512):
    assert int(data)!=0, "actual value member absent"
    if (int(data["kind"])&31)==3 and data["name"].string()=="value": break
    data=data["next"]
else: raise AssertionError("bounded member owner search exhausted")
assert not (int(data["storage"][1])&2)
binding["target"].assign(data.cast(gdb.lookup_type("ClassReference").pointer()))
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
set $eax = 0
continue
python
frame=gdb.newest_frame()
assert frame.name()=="member_pointer_expression", "invalid model escaped exact guard"
assert int(gdb.parse_and_eval("model"))==0
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert bytes(gdb.selected_inferior().read_memory(frame.pc()-1,1))==b"\xcc"
result={"branch":'data',"injected_model":0,"helper_was_unaccepted_stub":True,"stopped_function":gdb.newest_frame().name(),"status":"pass"}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/member-pointer-model-policy-2plkr9i_/data-model0/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
