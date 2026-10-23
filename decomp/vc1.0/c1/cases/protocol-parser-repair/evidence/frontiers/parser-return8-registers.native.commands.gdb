set pagination off
set confirm off
set disable-randomization off
break recovery_token_value
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/parser-return8-registers/stdout 2>&1
python
import gdb,json
entry=dict(esp=int(gdb.parse_and_eval('$ebp'))+4,token=int(gdb.parse_and_eval('token')))
end
finish
python
row=dict(entry=entry,esp=int(gdb.parse_and_eval('$esp')),eax=int(gdb.parse_and_eval('$eax')) & 0xffffffff,edx=int(gdb.parse_and_eval('$edx')) & 0xffffffff,caller=gdb.newest_frame().name())
assert row['esp']-entry['esp']==8
assert entry['token']==130 and row['eax'] & 255==130
assert row['caller']=='parse_translation_unit'
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/parser-return8-registers/registers.json','w')as out:json.dump(row,out,indent=2)
end
