set pagination off
set confirm off
start
set $buffer = (C2CoffRecord **)malloc(4096)
set $record = (C2CoffRecord *)malloc(sizeof(C2CoffRecord))
set var coff_symbol_buffer = $buffer
set var coff_symbol_capacity = 1023
set var coff_symbol_total = 7
set var coff_next_symbol = 2
set var coff_current_section = (C2OutputSection *)0x12345000
set var coff_reserved_symbols = 0x76543210
set $ecx = (unsigned)$record
set $result = ((unsigned (*)())add_coff_symbol)()
python
import gdb,json
print({name:str(gdb.parse_and_eval(name))for name in ['$result','$record','coff_symbol_total','coff_next_symbol','coff_symbol_buffer[2]','coff_current_section','coff_reserved_symbols']})
assert int(gdb.parse_and_eval('$result'))==2
assert int(gdb.parse_and_eval('coff_symbol_total'))==8
assert int(gdb.parse_and_eval('coff_next_symbol'))==3
assert int(gdb.parse_and_eval('coff_symbol_buffer[2]'))==int(gdb.parse_and_eval('$record'))
assert int(gdb.parse_and_eval('coff_current_section'))==0x12345000
assert int(gdb.parse_and_eval('coff_reserved_symbols'))==0x76543210
print('C2-SYMBOL-IDENTITIES '+json.dumps({'returned_index':2,'total':8,'next_index':3,'record_alias':True,'current_section_sentinel':0x12345000,'reserved_sentinel':0x76543210},sort_keys=True))
end
quit
