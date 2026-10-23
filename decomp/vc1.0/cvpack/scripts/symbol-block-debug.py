"""Read-only capture of two reached allocate_symbol malloc8100 lifetimes."""
def install(source,line):
    import gdb
    gdb.cvpack_symbol_blocks=dict(allocations=[])
    class Allocated(gdb.Breakpoint):
        def __init__(self):super().__init__(source+':'+str(line),internal=True)
        def stop(self):
            try:
                rows=gdb.cvpack_symbol_blocks;frame=gdb.newest_frame();assert frame.name()=='allocate_symbol'and len(rows['allocations'])<2
                family=frame.older().name();assert family in('intern_public','intern_global')
                base=int(frame.read_var('result'));request=int(frame.read_var('size'));available=int(gdb.parse_and_eval('public_symbol_available'));assert base and available<request and request<=8100
                rows['allocations'].append(dict(base=base,size=8100,family=family,request=request,previous_cursor=int(gdb.parse_and_eval('public_symbol_cursor')),previous_available=available,before=bytes(gdb.selected_inferior().read_memory(base,8100)).hex()))
                return False
            except Exception as error:gdb.cvpack_symbol_blocks['error']=str(error);return True
    Allocated()

def capture():
    import gdb
    rows=getattr(gdb,'cvpack_symbol_blocks',None)
    if rows is None or not rows['allocations']:return rows if rows and rows.get('error')else None
    if rows.get('error'):return rows
    assert len(rows['allocations'])==2
    for a in rows['allocations']:a['after']=bytes(gdb.selected_inferior().read_memory(a['base'],8100)).hex()
    rows['frontier']=gdb.newest_frame().name();return rows
