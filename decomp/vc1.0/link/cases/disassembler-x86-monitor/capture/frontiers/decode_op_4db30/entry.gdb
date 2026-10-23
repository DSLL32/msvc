set pagination off
set confirm off
break *decode_op_4db30
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4db30-cz4vtrqg/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4db30-cz4vtrqg/native.stderr
printf "NATIVE_MATCH decode_op_4db30\n"
x/6wx $esp
bt 5
python
import gdb,json,struct
esp=int(gdb.parse_and_eval('$esp'))
read=lambda address,size: bytes(gdb.selected_inferior().read_memory(address,size))
stack=struct.unpack('<4I',read(esp,16))
words=list(struct.unpack('<56I',read(stack[3],224)))
base=words[19]
words[18]=(words[18]-base)&0xffffffff if words[18] else 0xffffffff
words[19]=int(bool(base))
state=dict(event='decoder_state',words=words,operand=stack[2],cursor=(stack[1]-base)&0xffffffff)
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4db30-cz4vtrqg/native-decoder-state.json','w').write(json.dumps(state,indent=2)+'\n')
end
quit
