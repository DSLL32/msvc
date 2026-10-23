set pagination off
set confirm off
break *decode_op_4dcb0
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4dcb0-ycg9b18d/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4dcb0-ycg9b18d/native.stderr
printf "NATIVE_MATCH decode_op_4dcb0\n"
x/6wx $esp
bt 5
quit
