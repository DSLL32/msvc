set pagination off
set confirm off
break *decode_op_4d4e0
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/disassembler-x86-monitor-build/decode_op_4d4e0-after/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/disassembler-x86-monitor-build/decode_op_4d4e0-after/stderr.bin
python
import gdb,struct,json
esp=int(gdb.parse_and_eval('$esp'))
stack=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(esp,16)))
end
finish
python
words=list(struct.unpack('<56I',bytes(gdb.selected_inferior().read_memory(stack[3],224))))
base=words[19]
words[18]=(words[18]-base)&0xffffffff if words[18] else 0xffffffff
words[19]=int(bool(base))
state=dict(event='decoder_state',words=words,operand=stack[2],cursor=(stack[1]-base)&0xffffffff)
open('/root/.ai-tmp/msvc2/vc1.0/link/disassembler-x86-monitor-build/decode_op_4d4e0-after/state.json','w').write(json.dumps(state,indent=2)+'\n')
end
quit
