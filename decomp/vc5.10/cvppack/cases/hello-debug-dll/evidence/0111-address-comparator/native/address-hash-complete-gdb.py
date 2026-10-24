"""Native debugger-only observations of complete address hash lifetimes."""
import struct
import gdb


def install(ns):
    state=dict(sorts=[],callbacks=[],stream_calls=[],frees=[],allocations=[],initialization={})
    def registers():return [int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')]
    def memory(address,size):return bytes(gdb.selected_inferior().read_memory(address,size)).hex()
    def owned():return {str(row['address']):memory(row['address'],row['size']) for row in ns['address_hash_allocations']}
    def environment():return ns['sort_environment']()
    def formals(frame):
        scope=frame.block()
        while scope is not None and scope.function is None:scope=scope.superblock
        return {s.name:s.type.strip_typedefs() for s in scope if s.is_argument}
    class SortReturned(gdb.FinishBreakpoint):
        def __init__(self,row):self.row=row;super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            self.row.update(exit_esp=int(gdb.parse_and_eval('$esp')),callback_end=len(state['callbacks']),after_owned=owned(),after=environment());return False
    class SortEntered(gdb.Breakpoint):
        def __init__(self):super().__init__('*recon_qsort',internal=True)
        def stop(self):
            frame=gdb.newest_frame();caller=frame.older()
            if caller is None or caller.name()!='build_symbol_address_hash':return False
            assert len(state['sorts'])<3 and (not state['sorts'] or 'after' in state['sorts'][-1]),'address sort sequence unmodeled'
            esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(esp+4,16)))
            row=dict(index=len(state['sorts']),entry_esp=esp,args=list(args),owner=caller.name(),before_owned=owned(),callback_begin=len(state['callbacks']),before=environment());state['sorts'].append(row);SortReturned(row);return False
    SortEntered()
    class CallbackReturned(gdb.FinishBreakpoint):
        def __init__(self,row):self.row=row;super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):self.row.update(result=int(self.return_value),exit_esp=int(gdb.parse_and_eval('$esp')),after=environment());return False
    class CallbackEntered(gdb.Breakpoint):
        def __init__(self):super().__init__('*compare_symbol_address',internal=True)
        def stop(self):
            frame=gdb.newest_frame();assert frame.older().name()=='recon_qsort' and state['sorts'] and len(state['callbacks'])<4096
            actual=formals(frame);assert set(actual)=={'first','second'} and all(t.code==gdb.TYPE_CODE_PTR and t.sizeof==4 and t.target().code==gdb.TYPE_CODE_VOID for t in actual.values())
            esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)));root,count,width,callback=state['sorts'][-1]['args']
            assert width==8 and all(root<=p<root+count*8 and (p-root)%8==0 for p in args[1:]),'address callback outside own active array'
            row=dict(sort=len(state['sorts'])-1,sequence=len(state['callbacks']),entry_esp=esp,return_pc=args[0],first=args[1],second=args[2],records=memory(args[1],8)+memory(args[2],8),formal_inventory={k:str(t) for k,t in actual.items()},before=environment());state['callbacks'].append(row);CallbackReturned(row);return False
    CallbackEntered()
    class StreamReturned(gdb.FinishBreakpoint):
        def __init__(self,row):self.row=row;super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            r=self.row;raw=bytes.fromhex(memory(r['arena'],24));words=struct.unpack('<6I',raw)
            r.update(exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=registers(),after_arena=raw.hex())
            if r['name']=='initialize_arena':return False
            used,base,next_block=struct.unpack('<3I',bytes.fromhex(memory(words[2],12)));assert words[1]==1 and used<=4096
            r.update(result=int(self.return_value)&0xffffffff,output_address=base,output=memory(base,used));return False
    class StreamEntered(gdb.Breakpoint):
        def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
        def stop(self):
            frame=gdb.newest_frame();caller=frame.older()
            if caller is None or caller.name()!='build_symbol_address_hash':return False
            esp=int(gdb.parse_and_eval('$esp'));stack=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(esp,16)));types=formals(frame)
            if self.name=='initialize_arena':
                assert set(types)=={'arena','capacity'};arena=stack[1];row=state['initialization'];assert not row
                row.update(name=self.name,arena=arena,capacity=stack[2])
            else:
                copy=self.name=='copy_type_arena';assert set(types)==({'arena','data','size'} if copy else {'arena','value','size'})
                arena=(int(gdb.parse_and_eval('$ecx'))&0xffffffff) if copy else stack[1]
                value=(int(gdb.parse_and_eval('$edx'))&0xffffffff) if copy else stack[2];size=stack[1] if copy else stack[3]
                assert 0<size<=4 and len(state['stream_calls'])<256
                row=dict(name=self.name,kind=0 if copy else 1,arena=arena,value=value,size=size,before_arena=memory(arena,24))
                if copy:
                    row['input']=memory(value,size)
                    # Bind actual scalar source locals to their named owner.
                    row['scalar_sources']={name:dict(address=int(caller.read_var(name).address),width=int(caller.read_var(name).type.sizeof),type=str(caller.read_var(name).type)) for name in ('maximum_segment','offset')}
                state['stream_calls'].append(row)
            row.update(owner=caller.name(),entry_esp=esp,before_nonvolatile=registers(),formal_types={k:str(t) for k,t in types.items()});StreamReturned(row);return False
    for name in ('initialize_arena','copy_type_arena','fill_type_arena'):StreamEntered(name)
    class AllocationReturned(gdb.FinishBreakpoint):
        def __init__(self,row):self.row=row;super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):self.row.update(address=int(self.return_value)&0xffffffff,exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=registers());return False
    class AllocationEntered(gdb.Breakpoint):
        def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
        def stop(self):
            frame=gdb.newest_frame();caller=frame.older()
            if caller is None or caller.name()!='copy_type_arena' or caller.older() is None or caller.older().name()!='build_symbol_address_hash':return False
            types=formals(frame);assert set(types)=={'size'} and types['size'].code==gdb.TYPE_CODE_INT and types['size'].sizeof==4
            esp=int(gdb.parse_and_eval('$esp'));size=int.from_bytes(bytes(gdb.selected_inferior().read_memory(esp+4,4)),'little')
            row=dict(sequence=len(state['allocations']),kind='checked' if self.name=='allocate_bytes' else 'cleared',owner=caller.name(),context=caller.older().name(),size=size,entry_esp=esp,before_nonvolatile=registers(),formal_types={'size':str(types['size'])});state['allocations'].append(row);AllocationReturned(row);return False
    AllocationEntered('allocate_cleared');AllocationEntered('allocate_bytes')
    class FreeReturned(gdb.FinishBreakpoint):
        def __init__(self,row):self.row=row;super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):self.row.update(exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=registers(),returned=True);return False
    class FreeEntered(gdb.Breakpoint):
        def __init__(self):super().__init__('*recon_free',internal=True)
        def stop(self):
            frame=gdb.newest_frame();caller=frame.older()
            if caller is None or caller.name()!='build_symbol_address_hash':return False
            index=len(state['frees']);assert index<4;owner_index=(1,2,3,0)[index];allocation=ns['address_hash_allocations'][owner_index];esp=int(gdb.parse_and_eval('$esp'));address=int.from_bytes(bytes(gdb.selected_inferior().read_memory(esp+4,4)),'little');assert address==allocation['address']
            row=dict(sequence=index,index=owner_index,owner=caller.name(),entry_esp=esp,address=address,size=allocation['size'],before=memory(address,allocation['size']),before_nonvolatile=registers());state['frees'].append(row);FreeReturned(row);return False
    FreeEntered()
    return state


def finish(state,frame):
    owner=frame.older()
    while owner is not None and owner.name()!='write_publics':owner=owner.older()
    assert owner is not None and len(state['frees'])==4 and all(row.get('returned') for row in state['frees'])
    inferior=gdb.selected_inferior();arena=owner.read_var('addresses');raw=bytes(inferior.read_memory(arena.address,24));words=struct.unpack('<6I',raw);block=bytes(inferior.read_memory(words[2],12));used,base,nextblock=struct.unpack('<3I',block);assert used==540
    state['current']=dict(arena_address=int(arena.address),arena=raw.hex(),block_address=words[2],block=block.hex(),payload_address=base,payload=bytes(inferior.read_memory(base,used)).hex())
    return state
