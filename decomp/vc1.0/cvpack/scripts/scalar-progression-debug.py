"""98 progression stops at genuine strip_debug; no later symbol heap claim."""
def capture():
    import gdb
    f=gdb.newest_frame();assert f.name()=='strip_debug';caller=f.older();assert caller.name()=='main'
    handle=int(f.read_var('handle'));pc=int(caller.pc())
    return dict(signal=int(gdb.parse_and_eval('$_siginfo.si_signo')),frames=['strip_debug','main'],parameters=dict(handle=handle),input_file=int(gdb.parse_and_eval('input_file')),scope_depth=int(gdb.parse_and_eval('symbol_scope_depth')),suppressed_depth=int(gdb.parse_and_eval('suppressed_symbol_depth')),caller_pc=pc,caller_fp=int(caller.read_register('ebp')),call_bytes=bytes(gdb.selected_inferior().read_memory(pc-5,5)).hex(),function_address=int(gdb.parse_and_eval('&strip_debug')),thread=list(gdb.selected_thread().ptid),later_heap_equality=False)

def write(script,directory,policies):
    from state import BINARY
    marker='set argv[0] = recon_pgmptr\n';text=script.read_text();prefix=text[:text.index(marker)+len(marker)]
    prefix+='python\nimport runpy\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_sopen_observer"]('+repr(str(directory/'crt-returns.json'))+','+repr(str(directory/'debug-work'))+','+repr(policies)+')\nend\ncontinue\npython\nimport runpy,json\nstate=runpy.run_path('+repr(str(BINARY/'scripts/scalar-progression-debug.py'))+')["capture"]()\njson.dump(state,open('+repr(str(directory/'gdb.json'))+',"w"))\nend\nquit\n';script.write_text(prefix)
