set pagination off
set confirm off
break *strtoul
run -edit /rebase:base=0x500000 client.exe >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/strtoul-boundary-w_5ehrcn/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/strtoul-boundary-w_5ehrcn/stderr.bin
python
import gdb,json
inf=gdb.selected_inferior()
esp=int(gdb.parse_and_eval('$esp'))
a=[int.from_bytes(inf.read_memory(esp+i,4),'little') for i in (4,8,12)]
text=bytes(inf.read_memory(a[0],9))
assert text==b'0x500000\x00'
before=dict(input=text[:-1].decode(),base=a[2],end_pointer_present=a[1]!=0,errno=int(gdb.parse_and_eval('(int)errno')))
end
finish
python
result=int(gdb.parse_and_eval('$eax'))&0xffffffff
endptr=int.from_bytes(inf.read_memory(a[1],4),'little')
after=dict(result=result,end_offset=endptr-a[0],end_points_into_input=a[0]<=endptr<=a[0]+8,errno=int(gdb.parse_and_eval('(int)errno')))
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/strtoul-boundary-w_5ehrcn/native.json','w').write(json.dumps(dict(before=before,after=after),indent=2)+'\n')
end
quit
