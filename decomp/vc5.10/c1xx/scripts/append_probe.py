"""Actual append entry/return storage and next resolver ABI observations."""
from pathlib import Path


def setup(output, binary, collector):
    (output / 'append-probe-source.py').write_bytes(Path(__file__).read_bytes())
    checkpoint=output/'collect-first-append.gdb'
    checkpoint.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'first-append-state.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'first-append-argv.bin'))))
    code = r'''python
append_entries=[]
append_returns=[]
append_resolvers=[]
def append_storage(row,pointer):
    memory=gdb.selected_inferior()
    header=memory.read_memory(pointer,12).tobytes()
    count,capacity,arena,items=struct.unpack('<hhiI',header)
    if capacity!=40 or arena!=1 or not 0<=count<=40:raise RuntimeError('unknown actual append vector')
    row.update(vector_header=header.hex(),items=memory.read_memory(items,160).tobytes().hex(),committed=memory.read_memory(row['arena'][2],32768).tobytes().hex())
    return row
class AppendReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(AppendReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('append-%d-return'%self.entry['index'])
        row.update(index=self.entry['index'],entry_esp=self.entry['esp'],argument_slots_after=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(row['esp'],8))),value=action_string(self.entry['args'][1]))
        append_storage(row,self.entry['args'][0]);append_returns.append(row);self.enabled=False;return False
class AppendEntry(gdb.Breakpoint):
    def stop(self):
        index=len(append_entries);row=vector_abi(vector_row('append-%d-entry'%index),2)
        row.update(index=index,value=action_string(row['args'][1]))
        append_storage(row,row['args'][0]);append_entries.append(row)
        if not index:gdb.execute('source '+@@CHECKPOINT@@)
        AppendReturn(row);return False
class AppendResolver(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_row('actions-resolver-entry-0'),3)
        caller=gdb.newest_frame().older();resolved=caller.read_var('resolved')
        row.update(input=action_string(row['args'][1]),output_owner={'caller':caller.name(),'address':int(resolved.address),'bytes':int(resolved.type.sizeof)},committed=gdb.selected_inferior().read_memory(row['arena'][2],32768).tobytes().hex())
        append_resolvers.append(row);return False
AppendEntry('*append_option_vector',internal=True)
AppendResolver('*resolve_option_path',internal=True)
end
'''.replace('@@CHECKPOINT@@',repr(str(checkpoint)))
    finish='python\n'
    for name in ('entries','returns','resolvers'):
        finish+='open(%r,"w").write(json.dumps(append_%s,indent=2)+"\\n")\n'%(str(output/('append-'+name+'.json')),name)
    return code,finish+'end\n'
