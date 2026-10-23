set pagination off
set confirm off
break *decode_op_4bbd0
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4bbd0-cp44keg6/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4bbd0-cp44keg6/native.stderr
printf "NATIVE_MATCH decode_op_4bbd0\n"
x/6wx $esp
bt 5
quit
