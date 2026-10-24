import gdb,json
class Returned(gdb.FinishBreakpoint):
    def __init__(self,frame):super().__init__(frame,internal=True)
    def stop(self):
        pointer=int(self.return_value);inf=gdb.selected_inferior();raw=bytes(inf.read_memory(pointer,208));word=int.from_bytes(bytes(inf.read_memory(pointer-4,4)),'little')
        mapping=next(line.strip()for line in open('/proc/%s/maps'%inf.pid)if int(line.split('-')[0],16)<=pointer<int(line.split('-')[1].split()[0],16))
        assert pointer and len(raw)==208
        gdb.cvpack_allocator_observation=dict(requested=208,address=pointer,raw_before=raw.hex(),tail_before=raw[205:].hex(),chunk_size_word=word,is_mmapped=bool(word&2),mapping=mapping,caller='build_source_file_index',callee='checked_alloc',return_pc=int(gdb.newest_frame().pc()))
        return False
class Allocated(gdb.Breakpoint):
    def stop(self):
        f=gdb.newest_frame();p=f.older()
        if p and p.name()=='build_source_file_index'and int(f.read_var('size'))==208:Returned(f)
        return False
Allocated('checked_alloc',internal=True)
