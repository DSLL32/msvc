set pagination off
set confirm off
set disable-randomization off
break *report_asm_operand_error
run >/root/.ai-tmp/msvc2/vc1.0/c1/validation/direct-malloc-probe/stdout 2>&1
set $direct_stack = ((void *(*)(unsigned))malloc)(65536)
p/x $direct_stack
quit
