"""Physical generated-table reads and labeled nonzero pre-reset input."""
import gdb
from checksum_fields import TABLES,seed

def capture():
 base=int(gdb.parse_and_eval('(unsigned int)&c1_bss'))
 address=int(gdb.parse_and_eval('(unsigned int)&c1_bss.checksums'))
 assert address-base==1092 and int(gdb.parse_and_eval('sizeof(c1_bss.checksums)'))==768
 raw=gdb.selected_inferior().read_memory(address,768).tobytes()
 values={}
 for name,rva,offset in TABLES:
  assert int(gdb.parse_and_eval('(unsigned int)&c1_bss.checksums.'+name))-base==offset
  assert int(gdb.parse_and_eval('sizeof(c1_bss.checksums.'+name+')'))==256
  values[name]=list(raw[offset-1092:offset-1092+256])
 return values

def seed_restore():
 for name,values in seed().items():
  address=int(gdb.parse_and_eval('(unsigned int)&c1_bss.checksums.'+name))
  gdb.selected_inferior().write_memory(address,bytes(values))
