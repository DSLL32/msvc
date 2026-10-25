set pagination off
set confirm off
handle SIGTRAP nostop noprint pass
break harness/startup.c:38
commands
silent
printf "WPP first App size=%u\n",amount
printf "startup_checked=%u allocation_calls=%u SS=%u DS=%u\n",wpp_startup.first_frontier_checked,wpp_startup.allocation_calls,wpp_startup.saved_ss,wpp_startup.saved_ds
printf "XI kinds=%u,%u,%u stderr=%x firstFILE=%u\n",wpp_startup.initializers[0].kind,wpp_startup.initializers[1].kind,wpp_startup.initializers[2].kind,wpp_startup.files[2].flag,wpp_startup.files[0].flag
if amount != 8 || wpp_startup.first_frontier_checked != 1 || wpp_startup.allocation_calls != 1 || wpp_startup.files[2].flag != 0x402 || wpp_startup.initializers[2].kind != 2 || wpp_startup.initializers[1].kind != 0 || wpp_startup.initializers[0].kind != 0 || wpp_startup.open_streams != 0 || wpp_startup.closed_streams != 0
quit 2
end
disable 1
signal 0
end
continue
