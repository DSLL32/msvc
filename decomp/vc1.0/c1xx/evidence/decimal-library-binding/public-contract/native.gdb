break convert_floating_literal
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/decimal-boundary-agbxbe98/native-stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/decimal-boundary-agbxbe98/native-stderr.bin
disable 1
break recon_atoldbl
continue
python
import gdb,json
frame=gdb.newest_frame(); caller=frame.older()
output=int(frame.read_var("output")); text=int(frame.read_var("text"))
parsed=int(caller.read_var("parsed").address)
assert caller.name()=="convert_floating_literal" and output==parsed
memory=gdb.selected_inferior()
def contents(address):
    value=bytearray()
    for i in range(129):
        b=bytes(memory.read_memory(address+i,1))[0]; value.append(b)
        if not b:return value.hex()
    raise AssertionError("unterminated numeric spelling")
before=contents(text)
entry_pc=int(gdb.parse_and_eval("$pc"))
end
finish
python
right=dict(text=before,after_text=contents(text),output=bytes(memory.read_memory(output,10)).hex(),result=int(gdb.parse_and_eval("(int)$eax")),entry="recon_atoldbl",entry_pc=entry_pc,caller=gdb.newest_frame().name(),output_matches_caller_parsed=True,output_prestate="unread/uncredited",defined_output_bytes=10)
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/decimal-boundary-agbxbe98/native-observation.json',"w").write(json.dumps(right,indent=2)+"\n")
end
