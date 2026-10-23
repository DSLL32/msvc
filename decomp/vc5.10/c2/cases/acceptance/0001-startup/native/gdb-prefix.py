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
for record in [{'name': '_c2_crt_startup', 'native': 'c2_crt_startup', 'start': 4532119, 'end': 4532369, 'code_chunks': [[4670578, 4670673]], 'seh_table': 4772816, 'kind': 'crt-source', 'abi': 'cdecl, no parameters', 'evidence': 'cases/analysis/code-chunks.json; source startup.c complete original-style CRT body, no acceptance yet'}]:
    Entered(record.get('native',record['name'].lstrip('_')),record['name'])
