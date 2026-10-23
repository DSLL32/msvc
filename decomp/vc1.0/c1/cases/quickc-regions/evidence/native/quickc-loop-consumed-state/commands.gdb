set pagination off
set confirm off
break finish_qc_loop
run
p *qc_loop_work->value.qc_loop
x/9wx qc_loop_work->value.qc_loop
bt
quit
