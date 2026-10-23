set pagination off
set confirm off
break *decode_op_4bc10
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4bc10-b1l3anl_/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4bc10-b1l3anl_/native.stderr
printf "NATIVE_MATCH decode_op_4bc10\n"
x/6wx $esp
bt 5
quit
