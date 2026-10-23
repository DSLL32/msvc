set pagination off
set confirm off
break *decode_op_4bb50
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4bb50-z763ki5r/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4bb50-z763ki5r/native.stderr
printf "NATIVE_MATCH decode_op_4bb50\n"
x/6wx $esp
bt 5
quit
