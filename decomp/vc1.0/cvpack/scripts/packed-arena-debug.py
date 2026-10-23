"""Read-only requested-size capture of the distinct packed-node pool."""
def install(source,line):
    import gdb
    gdb.cvpack_packed_arena=dict(allocations=[])
    class Allocated(gdb.Breakpoint):
        def __init__(self):super().__init__(source+':'+str(line),internal=True)
        def stop(self):
            try:
                rows=gdb.cvpack_packed_arena;frame=gdb.newest_frame();assert frame.name()=='publish_packed_type'and not rows['allocations']
                assert int(gdb.parse_and_eval('packed_type_slots'))==int(gdb.parse_and_eval('packed_type_cursor'))==0
                base=int(frame.read_var('block'));assert base
                rows['allocations'].append(dict(base=base,size=12288,owner=int(frame.read_var('record')),type=int(frame.read_var('type')),source_line=line,before=bytes(gdb.selected_inferior().read_memory(base,12288)).hex()))
                return False
            except Exception as error:gdb.cvpack_packed_arena['error']=str(error);return True
    Allocated()

def capture():
    import gdb
    rows=getattr(gdb,'cvpack_packed_arena',None)
    if rows is None or not rows['allocations']:return rows if rows and rows.get('error')else None
    if rows.get('error'):return rows
    assert len(rows['allocations'])==1;base=rows['allocations'][0]['base']
    rows['frontier']=dict(name=gdb.newest_frame().name(),base=base,bytes=bytes(gdb.selected_inferior().read_memory(base,12288)).hex(),remaining=int(gdb.parse_and_eval('packed_type_slots')),cursor=int(gdb.parse_and_eval('packed_type_cursor')))
    return rows
