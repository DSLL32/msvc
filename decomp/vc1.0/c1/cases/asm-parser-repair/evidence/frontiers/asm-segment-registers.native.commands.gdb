set pagination off
set confirm off
set disable-randomization off
break make_asm_segment_value
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/asm-segment-registers/stdout 2>&1
python
import gdb,json
def state():
    o=gdb.parse_and_eval('asm_operand').dereference();a=o['address'].dereference();s=a['symbol'];v=s.dereference()
    return [int(gdb.parse_and_eval('asm_operand_count')),int(o['size']),int(o['flags']),int(a['displacement']),int(a['address_flags']),int(a['flags']),int(a['first_register']),int(a['second_register']),int(bool(int(s))),int(v['storage'][0]),int(v['index'])]
entry=dict(esp=int(gdb.parse_and_eval('$ebp'))+4,value=int(gdb.parse_and_eval('value')),state=state())
end
finish
python
row=dict(entry=entry,esp=int(gdb.parse_and_eval('$esp')),eax=int(gdb.parse_and_eval('$eax')) & 0xffffffff,state=state(),caller=gdb.newest_frame().name())
assert row['esp']-entry['esp']==8 and row['eax']==entry['value']
assert row['caller']=='parse_inline_assembler'
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/asm-segment-registers/registers.json','w')as out:json.dump(row,out,indent=2)
end
