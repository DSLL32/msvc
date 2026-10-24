"""Fresh bounded path-fold storage reads and labeled nonzero reset input."""
import struct,gdb
from path_fold_fields import OFFSET,SIZE,seed

def capture():
 address=int(gdb.parse_and_eval('(unsigned int)&c1_bss.path_fold'))
 assert address-int(gdb.parse_and_eval('(unsigned int)&c1_bss'))==OFFSET
 assert int(gdb.parse_and_eval('sizeof(c1_bss.path_fold)'))==SIZE
 return list(struct.unpack('<256H',gdb.selected_inferior().read_memory(address,SIZE).tobytes()))

def seed_restore():
 address=int(gdb.parse_and_eval('(unsigned int)&c1_bss.path_fold'))
 gdb.selected_inferior().write_memory(address,struct.pack('<256H',*seed('reset')))
