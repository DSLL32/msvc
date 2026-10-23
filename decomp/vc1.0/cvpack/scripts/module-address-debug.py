"""Capture actual entry arguments/input and the typed ushort return."""
def install():
    import gdb
    gdb.cvpack_module_addresses=[]
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,frame,row):super().__init__(frame,internal=True);self.row=row
        def stop(self):
            try:self.row['result']=int(self.return_value);return False
            except Exception as error:self.row['error']=str(error);return True
    class Entry(gdb.Breakpoint):
        def __init__(self):super().__init__('address_in_module',internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();assert f.older().name()=='convert_procedure32'and not gdb.cvpack_module_addresses
                input=int(gdb.parse_and_eval('symbol_input'));row=dict(module=int(f.read_var('module')),segment=int(f.read_var('segment')),offset=int(f.read_var('offset')),input=input,caller=f.older().name(),record=bytes(gdb.selected_inferior().read_memory(input,53)).hex())
                gdb.cvpack_module_addresses.append(row);Returned(f,row);return False
            except Exception as error:gdb.cvpack_module_addresses.append(dict(error=str(error)));return True
    Entry()

def capture():
    import gdb
    return getattr(gdb,'cvpack_module_addresses',[])
