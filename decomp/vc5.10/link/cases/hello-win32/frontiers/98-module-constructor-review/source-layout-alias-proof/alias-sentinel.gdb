set pagination off
set confirm off
starti
set variable mac_target=0x31
set variable native_ppc_symbol_maps=0x72
set $pc=134562633
stepi
python
import gdb,json
a=int(gdb.parse_and_eval("mac_target"));b=int(gdb.parse_and_eval("native_ppc_symbol_maps"))
assert (a,b)==(1,0x72),(a,b)
gdb.execute("set variable native_ppc_symbol_maps=0x47")
assert int(gdb.parse_and_eval("mac_target"))==1
print("ALIAS_SENTINEL_PASS",json.dumps(dict(mac_target=a,ppc_after_mac_store=b,ppc_after_separate_store=int(gdb.parse_and_eval("native_ppc_symbol_maps")))))
end
quit
