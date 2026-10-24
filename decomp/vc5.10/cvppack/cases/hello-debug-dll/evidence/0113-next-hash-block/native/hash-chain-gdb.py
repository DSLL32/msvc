"""Native-only candidate entry history and genuine typed block returns."""
import gdb
import struct


def install(ns,config,configuration):
    state=dict(calls=[],directory=str(__import__('pathlib').Path(configuration).parent))
    def memory(address,width):return bytes(gdb.selected_inferior().read_memory(address,width)).hex()
    def registers():return [int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')]
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,row):self.row=row;super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            row=self.row;row.update(result=int(self.return_value),exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=registers(),after_input=memory(row['argument'],row['width']));return False
    class Entered(gdb.Breakpoint):
        def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
        def stop(self):
            frame=gdb.newest_frame();caller=frame.older()
            if caller is None or caller.name()!='write_publics':return False
            if self.name=='first_hash_output_block' and not state['calls']:return False
            assert len(state['calls'])<3
            esp=int(gdb.parse_and_eval('$esp'));ret,arg=struct.unpack('<2I',bytes(gdb.selected_inferior().read_memory(esp,8)));scope=frame.block()
            while scope is not None and scope.function is None:scope=scope.superblock
            types={s.name:s.type.strip_typedefs()for s in scope if s.is_argument};name='stream'if self.name=='first_hash_output_block'else'block';width=24 if name=='stream'else 12
            assert set(types)=={name} and types[name].code==gdb.TYPE_CODE_PTR and types[name].sizeof==4 and types[name].target().sizeof==width
            row=dict(name=self.name,entry_pc=int(gdb.parse_and_eval('$pc')),entry_esp=esp,return_pc=ret,argument=arg,width=width,formal_type=str(types[name]),before_input=memory(arg,width),before_nonvolatile=registers(),owner=caller.name());state['calls'].append(row);Returned(row);return False
    Entered('first_hash_output_block');Entered('next_hash_output_block')
    class TypedPhase(gdb.Breakpoint):
        def __init__(self):super().__init__('next_hash_output_block',internal=True)
        def stop(self):
            if 'phase' in state:return False
            assert len(state['calls'])==1
            from pathlib import Path
            import json
            directory=Path(configuration).parent;phase=dict(config,observe_next_hash_output_block=False)
            path=directory/'first-next-block-config.json';path.write_text(json.dumps(phase,indent=2)+'\n')
            ns['capture'](str(path),str(directory/'data.json'),str(directory/'first-next-block-state.json'))
            state['phase']=dict(physical_pc=state['calls'][0]['entry_pc'],physical_esp=state['calls'][0]['entry_esp'],typed_pc=int(gdb.parse_and_eval('$pc')),typed_esp=int(gdb.parse_and_eval('$esp')),kind='actual candidate-entry ABI plus typed checkpoint; historical only')
            return False
    TypedPhase()
    return state


def finish(state):
    assert len(state['calls'])==3 and all('result'in row for row in state['calls'])
    # Only the live typed heap generations are read after write_publics returns;
    # its automatic arena descriptors remain historical entry observations.
    from pathlib import Path
    import json
    phase=json.loads((Path(state['directory'])/'first-next-block-state.json').read_text())
    current={}
    for name,row in [('names',phase['name_hash_complete']),('addresses',phase['address_hash_complete']['current'])]:
        current[name]=dict(block_address=row['block_address'],block=bytes(gdb.selected_inferior().read_memory(row['block_address'],12)).hex(),payload_address=row['payload_address'],payload=bytes(gdb.selected_inferior().read_memory(row['payload_address'],564 if name=='names'else 540)).hex())
    state['current']=current
    return state
