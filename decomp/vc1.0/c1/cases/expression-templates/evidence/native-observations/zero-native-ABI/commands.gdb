set pagination off
set confirm off
set disable-randomization off
break *make_reusable_zero
run 0 /root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-zero-native-ABI/report >/root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-zero-native-ABI/stdout 2>&1
python
import gdb,struct,json
def u(x):return int(gdb.parse_and_eval(x))&0xffffffff
sp=u('$esp');ret=struct.unpack('<I',bytes(gdb.selected_inferior().read_memory(sp,4)))[0]
before={r:u('$'+r)for r in ['ebx','esi','edi','ebp','fctrl']}
gdb.Breakpoint('*'+str(ret),temporary=True)
end
delete 1
continue
python
after={r:u('$'+r)for r in ['ebx','esi','edi','ebp','fctrl']}
assert after==before and u('$esp')==sp+4
with open('/root/.ai-tmp/msvc2/vc1.0/c1/validation/expression-zero-native-ABI/registers.json','w')as out:json.dump(dict(status='pass',before=before,after=after,esp_before=sp,esp_after=u('$esp'),result=u('$eax'),formals=[],caller=gdb.newest_frame().name()),out,indent=2)
end
continue
