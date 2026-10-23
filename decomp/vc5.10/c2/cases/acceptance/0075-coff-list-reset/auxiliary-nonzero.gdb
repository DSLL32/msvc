set pagination off
set confirm off
start
set var coff_pending_499690 = 0x12345678
set var coff_output_list = (C2OutputList *)0x11111111
set var coff_current_list = (C2OutputList *)0x22222222
set var coff_free_chunks = (C2OutputChunk *)0x33333333
set var coff_free_blocks_508 = (C2OutputBlock508 *)0x44444444
set var coff_free_blocks_308 = (C2OutputBlock308 *)0x55555555
set var coff_pending_c90 = 0x76543210
call initialize_coff_lists()
python
import json
names=['coff_pending_499690','coff_output_list','coff_current_list','coff_free_chunks','coff_free_blocks_508','coff_free_blocks_308']
actual={name:int(gdb.parse_and_eval(name))for name in names}
assert all(value==0 for value in actual.values()),actual
assert int(gdb.parse_and_eval('coff_pending_c90'))==0x76543210
print('C2-NONZERO-RESET '+json.dumps(actual,sort_keys=True))
end
quit
