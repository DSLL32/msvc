"""Exact98 native first callback entry/Finish, never an invented app frontier."""
def install(destination):
    import gdb,json,runpy
    from pathlib import Path
    root=Path(__file__).resolve().parent
    class Finished(gdb.FinishBreakpoint):
        def __init__(self,frame,row):
            super().__init__(frame,internal=True);self.row=row
        def stop(self):
            row=self.row;f=gdb.newest_frame();thread=gdb.selected_thread().ptid
            row.update(return_pc=int(f.pc()),return_fp=int(f.read_register('ebp')),return_thread=list(thread),return_function=f.name(),after=[int(gdb.parse_and_eval(x))for x in('symbol_input','suppressed_symbol_depth','symbol_scope_depth','symbol_output')])
            assert f.name()=='pack_new_types'and row['return_pc']==row['caller_pc']and row['return_fp']==row['caller_fp']and row['return_thread']==row['thread']
            assert row['after']==[row['before'][0],0,1,row['before'][3]]
            gdb.cvpack_scalar_block=row;Path(destination).write_text(json.dumps(row,indent=2)+'\n');return True
        def out_of_scope(self):raise RuntimeError('98 callback owner frame exited without its actual return')
    class Entered(gdb.Breakpoint):
        def __init__(self):super().__init__('begin_symbol_block',internal=True);self.seen=False
        def stop(self):
            assert not self.seen,'only first98 invocation approved';self.seen=True;f=gdb.newest_frame();caller=f.older();assert caller.name()=='pack_new_types'
            row=runpy.run_path(str(root/'progression-debug.py'))['capture']();row['signal']=0
            row.update(before=[int(gdb.parse_and_eval(x))for x in('symbol_input','suppressed_symbol_depth','symbol_scope_depth','symbol_output')],thread=list(gdb.selected_thread().ptid),caller_pc=int(caller.pc()),caller_fp=int(caller.read_register('ebp')),entry_pc=int(f.pc()),function_address=int(gdb.parse_and_eval('&begin_symbol_block')),call_bytes=bytes(gdb.selected_inferior().read_memory(int(caller.pc())-8,8)).hex(),caller_line=caller.find_sal().line)
            conversion=caller.read_var('conversion');row.update(conversion=int(conversion),conversion_base=int(gdb.parse_and_eval('&symbol_conversions[0]')),conversion_kind=int(conversion.dereference()['kind']),conversion_target=int(conversion.dereference()['convert']),conversion_index=int(caller.read_var('index')))
            assert row['before'][1:3]==[0,0];Finished(f,row);return False
    Entered()

def write(script,directory,policies):
    from state import BINARY
    marker='set argv[0] = recon_pgmptr\n';text=script.read_text();prefix=text[:text.index(marker)+len(marker)]
    prefix+='python\nimport runpy\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_sopen_observer"]('+repr(str(directory/'crt-returns.json'))+','+repr(str(directory/'debug-work'))+','+repr(policies)+')\n'
    prefix+='runpy.run_path('+repr(str(BINARY/'scripts/scalar-block-debug.py'))+')["install"]('+repr(str(directory/'scalar-block.json'))+')\nend\ncontinue\npython\nimport json\njson.dump(dict(signal=0,frames=[gdb.newest_frame().name()],scalar_completion=True),open('+repr(str(directory/'gdb.json'))+',"w"))\nend\nquit\n';script.write_text(prefix)
