set pagination off
set confirm off
set disable-randomization off
break *copy_expression_tree
run 3 /root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-copier-native-ABI/report >/root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-copier-native-ABI/stdout 2>&1
python
import gdb,struct,json
def u(x):return int(gdb.parse_and_eval(x))&0xffffffff
sp=u('$esp');ret,source,arena=struct.unpack('<III',bytes(gdb.selected_inferior().read_memory(sp,12)));assert arena==6
before={r:u('$'+r)for r in ['ebx','esi','edi','ebp','fctrl']}
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
after={r:u('$'+r)for r in ['ebx','esi','edi','ebp','fctrl']}
assert after==before and u('$esp')==sp+12
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-copier-native-ABI/registers.json','w')as out:json.dump(dict(status='pass',before=before,after=after,esp_before=sp,esp_after=u('$esp'),result=u('$eax'),source=source,arena=arena,raw_slots=[source,arena],caller=gdb.newest_frame().name()),out,indent=2)
end
continue
