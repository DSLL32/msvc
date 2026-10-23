set pagination off
set confirm off
break *decode_op_4d9d0
run -dump /disasm standard486.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-standard486-decode_op_4d9d0-e4y5aeri/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-standard486-decode_op_4d9d0-e4y5aeri/native.stderr
printf "NATIVE_MATCH decode_op_4d9d0\n"
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
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/large-image-standard486-decode_op_4d9d0-e4y5aeri/native-decoder-state.json','w').write(json.dumps(state,indent=2)+'\n')
end
quit
