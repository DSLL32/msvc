"""Native GDB observation of the encountered CRT initialization allocation."""
import gdb
import json
import os
from pathlib import Path
path=Path(os.environ['RCDLL_NATIVE_SNAPSHOT']).parent/'allocations.json'
records=[]
def save():path.write_text(json.dumps(records,indent=2)+'\n')
class Returned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry['result']=int(gdb.parse_and_eval('$eax'))&0xffffffff
        save();return False
class Entered(gdb.Breakpoint):
    def __init__(self):super().__init__('malloc',internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rcdll_crt_init':return False
        size=int(gdb.parse_and_eval('*(unsigned int *)($esp+4)'))
        entry=dict(caller=caller.name(),bytes=size)
        records.append(entry);save()
        if size!=128:return True
        Returned(frame,entry)
        return False
Entered();save()

heap_path=path.parent/'heap.json'
heaps=[]
def save_heaps():heap_path.write_text(json.dumps(heaps,indent=2)+'\n')
class HeapReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry['result']=int(gdb.parse_and_eval('$eax'))&0xffffffff
        self.entry['last_error_after']=int(gdb.parse_and_eval('last_error'))
        if self.entry['operation']=='allocate':
            self.entry['payload']=bytes(gdb.selected_inferior().read_memory(self.entry['result'],self.entry['bytes'])).hex()
        save_heaps();return False
class HeapEntered(gdb.Breakpoint):
    def __init__(self,name,operation):self.operation=operation;super().__init__(name,internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        expected='rc_initialize' if self.operation=='create' else 'rc_allocate'
        if caller is None or caller.name()!=expected:return False
        entry=dict(operation=self.operation,caller=caller.name(),last_error_before=int(gdb.parse_and_eval('last_error')))
        fields=['flags','initial','maximum'] if self.operation=='create' else ['heap','flags','bytes']
        formals=['options','initial','maximum'] if self.operation=='create' else ['handle','flags','size']
        for name,formal in zip(fields,formals):entry[name]=int(frame.read_var(formal))&0xffffffff
        heaps.append(entry);save_heaps();HeapReturned(frame,entry);return False
HeapEntered('recon_HeapCreate','create');HeapEntered('recon_HeapAlloc','allocate');save_heaps()

class Complete(gdb.Breakpoint):
    def __init__(self):super().__init__('fixture_complete',internal=True)
    def stop(self):
        script=Path(os.environ['RCDLL_NATIVE_SCRIPT'])
        gdb.execute('source '+str(script))
        return False
Complete()
def exited(event):
    (path.parent/'exit.json').write_text(json.dumps(dict(exit_code=getattr(event,'exit_code',None)))+'\n')
gdb.events.exited.connect(exited)
