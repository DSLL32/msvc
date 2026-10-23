set pagination off
set confirm off
break *decode_op_4b790
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4b790-a4hejl3u/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4b790-a4hejl3u/native.stderr
printf "NATIVE_MATCH decode_op_4b790\n"
x/6wx $esp
bt 5
quit
