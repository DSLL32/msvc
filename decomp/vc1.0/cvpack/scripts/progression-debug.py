"""Exact97 next-stub inputs; no later-module heap equivalence claim."""
def capture():
    import gdb
    f=gdb.newest_frame();assert f.name()=='begin_symbol_block';caller=f.older();assert caller.name()=='pack_new_types'
    scope=int(gdb.parse_and_eval('symbol_scope_depth'));suppressed=int(gdb.parse_and_eval('suppressed_symbol_depth'));assert scope==suppressed==0
    base=int(gdb.parse_and_eval('type_data'));cursor=int(gdb.parse_and_eval('symbol_input'));capacity=int(gdb.parse_and_eval('type_capacity'))
    module=caller.read_var('module');entry=caller.read_var('entry');directory=entry.dereference();out=int(caller.read_var('start'))
    assert capacity==4746 and cursor==base+4 and int(caller.read_var('types'))==base and int(module.dereference()['module'])==10
    assert int(directory['module'])==10 and int(directory['subsection'])==0x124 and int(directory['location']['offset'])==4754 and int(directory['size'])==43
    assert(int(caller.read_var('kind')),int(caller.read_var('length')),int(caller.read_var('index')),int(caller.read_var('output_size')))==(0x206,35,20,44)
    assert int(gdb.parse_and_eval('symbol_output'))==out+4
    raw=bytes(gdb.selected_inferior().read_memory(base,43));header=bytes(gdb.selected_inferior().read_memory(out,4));assert header==raw[:4]==b'\1\0\0\0'
    frames=[];current=f
    while current:frames.append(current.name());current=current.older()
    return dict(signal=int(gdb.parse_and_eval('$_siginfo.si_signo')),frames=frames,parameters={},scope_depth=scope,suppressed_depth=suppressed,module=10,module_address=int(module),input=base,cursor=cursor,capacity=capacity,directory=int(entry),offset=4754,size=43,cvbase=int(gdb.parse_and_eval('cv_file_offset')),read_offset=int(gdb.parse_and_eval('cv_read_offset')),output=out,output_cursor=out+4,bytes=raw.hex(),callback=dict(kind=0x206,length=35,index=20),later_heap_equality=False)

def write(script,directory,policies):
    import json
    from state import BINARY
    marker='set argv[0] = recon_pgmptr\n';text=script.read_text();prefix=text[:text.index(marker)+len(marker)]
    prefix+='python\nimport runpy\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_sopen_observer"]('+repr(str(directory/'crt-returns.json'))+','+repr(str(directory/'debug-work'))+','+repr(policies)+')\nend\n'
    prefix+='continue\npython\nimport runpy,json\nstate=runpy.run_path('+repr(str(BINARY/'scripts/progression-debug.py'))+')["capture"]()\njson.dump(state,open('+repr(str(directory/'gdb.json'))+',"w"))\nend\nquit\n';script.write_text(prefix)
