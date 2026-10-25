set pagination off
set confirm off
handle SIGTRAP nostop noprint pass
break harness/startup.c:39
commands
silent
printf "WCC first App size=%u\n",amount
printf "startup_checked=%u allocation_calls=%u SS=%u DS=%u\n",wcc_startup.first_frontier_checked,wcc_startup.allocation_calls,wcc_startup.saved_ss,wcc_startup.saved_ds
printf "XI kinds=%u,%u,%u stderr=%x firstFILE=%u\n",wcc_startup.initializers[0].kind,wcc_startup.initializers[1].kind,wcc_startup.initializers[2].kind,wcc_startup.files[2].flag,wcc_startup.files[0].flag
if amount != 8 || wcc_startup.first_frontier_checked != 1 || wcc_startup.allocation_calls != 1 || wcc_startup.files[2].flag != 0x402 || wcc_startup.initializers[2].kind != 2 || wcc_startup.initializers[1].kind != 0
quit 2
end
disable 1
signal 0
end
continue
