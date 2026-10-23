"""Read actual resolver inputs and outputs without inferior helper calls."""
def install():
    import gdb
    gdb.cvpack_resolver=dict(entries=[],returns=[])
    def memory(address,size):return bytes(gdb.selected_inferior().read_memory(address,size)).hex()
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,frame,entry):
            super().__init__(frame,internal=True);self.entry=entry
        def stop(self):
            try:
                result=int(self.return_value);base=int(gdb.parse_and_eval('local_type_records'));relative=(result-base)//28
                assert result>=base and (result-base)%28==0 and relative<12000
                gdb.cvpack_resolver['returns'].append(dict(sequence=self.entry['sequence'],result=result,flag=self.entry['flag'],value=memory(self.entry['flag'],2),relative=relative,record=memory(result,28)))
                return False
            except Exception as error:
                gdb.cvpack_resolver['error']=str(error);return True
    class Entry(gdb.Breakpoint):
        def __init__(self):super().__init__('get_type_record',internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();caller=f.older();rows=gdb.cvpack_resolver
                assert caller.name()=='compact_new_type'and len(rows['entries'])<256
                flag=int(f.read_var('flag'));unused=caller.read_var('unused')
                assert flag==int(unused.address)and unused.type.sizeof==2
                parent=caller.read_var('entry');type=int(f.read_var('type'))
                assert type==((int(parent.dereference()['forward_index'])-0x1000)&0xffff)
                entry=dict(sequence=len(rows['entries'])+1,type=type,flag=flag,before=memory(flag,2),caller=caller.name(),caller_line=caller.find_sal().line,local='unused',size=2,local_address=int(unused.address),parent_type=int(caller.read_var('type')),parent_record=int(parent))
                rows['entries'].append(entry);Returned(f,entry);return False
            except Exception as error:
                gdb.cvpack_resolver['error']=str(error);return True
    Entry()

def capture():
    import gdb
    return getattr(gdb,'cvpack_resolver',None)
