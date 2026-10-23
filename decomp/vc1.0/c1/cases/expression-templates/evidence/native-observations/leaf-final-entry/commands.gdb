set pagination off
set confirm off
set disable-randomization off
run 1 /root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-leaf-final-entry/report >/root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-leaf-final-entry/stdout 2>&1
bt
python
import gdb,json,struct
frame=gdb.newest_frame();assert frame.name()=='copy_expression_tree'
source=int(gdb.parse_and_eval('source'));arena=int(gdb.parse_and_eval('arena'));bp=int(gdb.parse_and_eval('$ebp'))
slots=struct.unpack('<II',bytes(gdb.selected_inferior().read_memory(bp+8,8)))
assert slots==(source,arena)and arena==6
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-leaf-final-entry/registers.json','w')as out:json.dump(dict(logical=frame.name(),source=source,arena=arena,raw_slots=slots),out,indent=2)
end
