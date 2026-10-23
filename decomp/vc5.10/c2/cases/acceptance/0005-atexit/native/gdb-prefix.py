import gdb,json
trace=[]
class Entered(gdb.Breakpoint):
    def __init__(self,symbol,logical):
        self.logical=logical
        super().__init__(symbol,internal=True)
    def stop(self):
        if self.logical not in trace:
            trace.append(self.logical)
            gdb.write('C2-SOURCE-ENTRY %s 0x%x\n'%(self.logical,int(gdb.parse_and_eval('$pc'))))
        return False
for record in [{'name': '_c2_crt_startup', 'native': 'c2_crt_startup', 'start': 4532119, 'end': 4532369, 'code_chunks': [[4670578, 4670673]], 'seh_table': 4772816, 'kind': 'crt-source', 'abi': 'cdecl, no parameters', 'evidence': 'cases/analysis/code-chunks.json; source startup.c complete original-style CRT body, no acceptance yet'}, {'name': '_c2_set_argv', 'native': 'c2_set_argv', 'start': 4320137, 'end': 4320138, 'abi': 'cdecl, no parameters', 'kind': 'crt-source', 'evidence': 'original byte C3 at 0x41eb89; no parameters, state or callees'}, {'name': '_c2_set_default_precision', 'native': 'c2_set_default_precision', 'start': 4532369, 'end': 4532388, 'abi': 'cdecl, no parameters', 'kind': 'crt-source', 'evidence': 'cases/analysis/startup-disassembly.txt; full 19-byte precision helper; cdecl _controlfp(0x10000,0x30000), caller cleanup8'}, {'name': '_c2_main', 'native': 'c2_main', 'start': 4532400, 'end': 4532715, 'code_chunks': [[4670673, 4670813]], 'abi': 'cdecl, int argc and two pointer vectors', 'kind': 'application', 'seh_table': 4772800, 'evidence': 'cases/analysis/startup-disassembly.txt; complete 315-byte main and140-byte cold SEH chunks'}, {'name': '_c2_register_exit', 'native': 'c2_register_exit', 'start': 4532721, 'end': 4532742, 'pending_extent': True, 'kind': 'crt-source', 'abi': 'cdecl, one callback pointer', 'evidence': 'original4529f1..452a06: callback passed cdecl, return0 on nonNULL registration result else-1'}]:
    Entered(record.get('native',record['name'].lstrip('_')),record['name'])
