"""Read-only native capture of the first recursive hash slab allocation."""
def install(source,line):
    import gdb
    gdb.cvpack_recursive_arena=dict(allocations=[])
    class Allocated(gdb.Breakpoint):
        def __init__(self):super().__init__(source+':'+str(line),internal=True)
        def stop(self):
            try:
                rows=gdb.cvpack_recursive_arena;frame=gdb.newest_frame()
                assert frame.name()=='publish_recursive_type'and not rows['allocations']
                assert int(gdb.parse_and_eval('recursive_type_slots'))==int(gdb.parse_and_eval('recursive_type_cursor'))==0
                base=int(frame.read_var('block'));assert base
                rows['allocations'].append(dict(base=base,size=0x3000,owner=int(frame.read_var('entry')),source_line=line,before=bytes(gdb.selected_inferior().read_memory(base,0x3000)).hex()))
                return False
            except Exception as error:
                gdb.cvpack_recursive_arena['error']=str(error);return True
    Allocated()

def capture():
    import gdb
    rows=getattr(gdb,'cvpack_recursive_arena',None)
    if rows is None or not rows['allocations']:return rows if rows and rows.get('error')else None
    if rows.get('error'):return rows
    assert len(rows['allocations'])==1
    base=rows['allocations'][0]['base']
    rows['frontier']=dict(name=gdb.newest_frame().name(),base=base,bytes=bytes(gdb.selected_inferior().read_memory(base,0x3000)).hex(),remaining=int(gdb.parse_and_eval('recursive_type_slots')),cursor=int(gdb.parse_and_eval('recursive_type_cursor')))
    return rows
