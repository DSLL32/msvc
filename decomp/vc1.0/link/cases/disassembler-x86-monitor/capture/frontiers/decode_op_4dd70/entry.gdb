set pagination off
set confirm off
break *decode_op_4dd70
run -dump /disasm monitor.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4dd70-05o3p21t/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/dump-disasm-x86-monitor-decode_op_4dd70-05o3p21t/native.stderr
printf "NATIVE_MATCH decode_op_4dd70\n"
x/6wx $esp
bt 5
quit
