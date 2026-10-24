"""Fresh typed scanner-table reads; explicit known-field restore inputs."""
import gdb
from multibyte_fields import TABLES,seed

def capture():
 base=int(gdb.parse_and_eval('(unsigned int)&c1_data'))
 assert int(gdb.parse_and_eval('sizeof(c1_data)'))==2348
 result={}
 for name,rva,offset in TABLES:
  address=int(gdb.parse_and_eval('(unsigned int)&c1_data.scanner.'+name))
  assert address-base==offset and int(gdb.parse_and_eval('sizeof(c1_data.scanner.'+name+')'))==256
  result[name]=list(gdb.selected_inferior().read_memory(address,256).tobytes())
 saved=int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data'))
 snapshot={name:list(gdb.selected_inferior().read_memory(saved+offset,256).tobytes())for name,rva,offset in TABLES}if saved else None
 return dict(initialized=result,snapshot=snapshot,offsets={name:offset for name,rva,offset in TABLES},initialized_size=2348)

def seed_restore():
 saved=int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data'));assert saved
 for phase in ('snapshot','live'):
  values=seed(phase)
  for name,rva,offset in TABLES:
   address=saved+offset if phase=='snapshot'else int(gdb.parse_and_eval('(unsigned int)&c1_data.scanner.'+name))
   gdb.selected_inferior().write_memory(address,bytes(values[name]))
