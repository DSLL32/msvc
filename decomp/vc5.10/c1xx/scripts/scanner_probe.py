"""Actual scanner/API caller, formal, state and return observations."""
from pathlib import Path
from scaler_probe import symbols


def setup(output,binary):
    (output/'scanner-probe-source.py').write_bytes(Path(__file__).read_bytes())
    names=symbols(binary)
    code='''python
scanner_entries=[]
scanner_returns=[]
scanner_api_entries=[]
scanner_api_returns=[]
scanner_next_entries=[]
def scanner_row():
    row=checksum_row();address=int(gdb.parse_and_eval('&frontend_data.scanner'))
    row['scanner']=gdb.selected_inferior().read_memory(address,512).tobytes().hex();row['scanner_address']=address
    return row
class ScannerReturn(gdb.Breakpoint):
    def __init__(self,address,entry_esp,rows,phase):
        super(ScannerReturn,self).__init__('*0x%x'%address,internal=True);self.entry_esp=entry_esp;self.rows=rows;self.phase=phase
    def stop(self):
        row=scanner_row();row.update(entry_esp=self.entry_esp,locale=checksum_locale_state(self.phase))
        self.rows.append(row);self.enabled=False;return False
class ScannerEntry(gdb.Breakpoint):
    def stop(self):
        row=scanner_row();row.update(return_pc=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4))[0],formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state('mb-entry'))
        scanner_entries.append(row);ScannerReturn(row['return_pc'],row['esp'],scanner_returns,'mb-helper-return');return False
class ScannerApiEntry(gdb.Breakpoint):
    def stop(self):
        row=scanner_row();words=struct.unpack('<Ii',gdb.selected_inferior().read_memory(row['esp'],8))
        row.update(return_pc=words[0],args=[words[1]],formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state('mb-before'))
        scanner_api_entries.append(row);ScannerReturn(words[0],row['esp'],scanner_api_returns,'mb-after');return False
class ScannerNextEntry(gdb.Breakpoint):
    def stop(self):
        row=scanner_row();row.update(return_pc=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4))[0],formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state('mb-next'))
        scanner_next_entries.append(row);return False
ScannerEntry('*0x%x'%@@ENTRY@@,internal=True)
ScannerApiEntry('*recon_setmbcp',internal=True)
ScannerNextEntry('*0x%x'%@@NEXT@@,internal=True)
end
'''.replace('@@ENTRY@@',str(names['initialize_multibyte_categories'][0])).replace('@@NEXT@@',str(names['initialize_lowercase_table'][0]))
    finish='python\n'
    for suffix in ('entries','returns','api_entries','api_returns','next_entries'):
        finish+='open(%r,"w").write(json.dumps(scanner_%s,indent=2)+"\\n")\n'%(str(output/('scanner-'+suffix.replace('_','-')+'.json')),suffix)
    return code,finish+'end\n'
