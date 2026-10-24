"""Actual filename-fold entries, complete API vector and return observations."""
from pathlib import Path
from scaler_probe import symbols


def setup(output,binary,seed):
    (output/'path-fold-probe-source.py').write_bytes(Path(__file__).read_bytes())
    names=symbols(binary)
    code='''python
fold_entries=[]
fold_returns=[]
fold_api_entries=[]
fold_api_returns=[]
fold_next_entries=[]
def fold_row():
    row=checksum_row();address=int(gdb.parse_and_eval('&frontend_bss.filename_case_map'))
    row['map']=gdb.selected_inferior().read_memory(address,512).tobytes().hex();row['map_address']=address
    address=int(gdb.parse_and_eval('&frontend_data.scanner.classes'));row['classes']=gdb.selected_inferior().read_memory(address,256).tobytes().hex()
    return row
class FoldReturn(gdb.Breakpoint):
    def __init__(self,entry,rows,phase):
        super(FoldReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry;self.rows=rows;self.phase=phase
    def stop(self):
        row=fold_row();row.update(entry_esp=self.entry['esp'],locale=checksum_locale_state(self.phase))
        self.rows.append(row);self.enabled=False;return False
class FoldEntry(gdb.Breakpoint):
    def stop(self):
        before=fold_row()
        if @@SEED@@:gdb.selected_inferior().write_memory(before['map_address'],struct.pack('<256H',*[(0x4303+37*i)&65535 for i in range(256)]))
        row=fold_row();row.update(before_seed=before,return_pc=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4))[0],formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state('fold-entry'))
        fold_entries.append(row);FoldReturn(row,fold_returns,'fold-complete');return False
class FoldApiEntry(gdb.Breakpoint):
    def stop(self):
        row=fold_row();words=struct.unpack('<II',gdb.selected_inferior().read_memory(row['esp'],8))
        row.update(return_pc=words[0],args=[words[1]],formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state('fold-api-%03d-before'%len(fold_api_entries)))
        fold_api_entries.append(row);FoldReturn(row,fold_api_returns,'fold-api-%03d-after'%(len(fold_api_entries)-1));return False
class FoldNext(gdb.Breakpoint):
    def stop(self):
        row=fold_row();words=struct.unpack('<II',gdb.selected_inferior().read_memory(row['esp'],8))
        row.update(return_pc=words[0],args=[words[1]],formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state('fold-next'))
        fold_next_entries.append(row);return False
FoldEntry('*0x%x'%@@ENTRY@@,internal=True)
FoldApiEntry('*recon_mbctolower',internal=True)
FoldNext('*0x%x'%@@NEXT@@,internal=True)
end
'''.replace('@@ENTRY@@',str(names['initialize_lowercase_table'][0])).replace('@@NEXT@@',str(names['read_env_arguments'][0])).replace('@@SEED@@',repr(bool(seed)))
    finish='python\n'
    for suffix in ('entries','returns','api_entries','api_returns','next_entries'):
        finish+='open(%r,"w").write(json.dumps(fold_%s,indent=2)+"\\n")\n'%(str(output/('path-fold-'+suffix.replace('_','-')+'.json')),suffix)
    return code,finish+'end\n'
