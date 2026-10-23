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
        if caller is None or caller.name()!='_CRT_INIT':return False
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
        if self.entry['operation']=='free':
            cursor=gdb.parse_and_eval('native_heaps')
            while int(cursor)and int(cursor)!=self.entry['heap']:cursor=cursor.dereference()['next']
            assert int(cursor),'freed allocation owner disappeared'
            block=cursor.dereference()['blocks'];live=[]
            while int(block):
                item=block.dereference();live.append(dict(payload=int(item['payload'])&0xffffffff,bytes=int(item['requested_size']),flags=int(item['flags'])))
                block=item['next']
            self.entry['live_after']=live
        save_heaps();return False
class HeapEntered(gdb.Breakpoint):
    def __init__(self,name,operation):self.operation=operation;super().__init__(name,internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        expected='rc_initialize' if self.operation=='create' else 'rc_allocate'
        if caller is None or caller.name()!=expected:return False
        limit=1 if self.operation=='create'else int(os.environ.get('RCDLL_NATIVE_HEAP_LIMIT','1'))
        seen=sum(record['operation']==self.operation for record in heaps)
        if seen>=limit:self.enabled=False;return False
        if seen+1>=limit:self.enabled=False
        entry=dict(operation=self.operation,caller=caller.name(),last_error_before=int(gdb.parse_and_eval('last_error')))
        fields=['flags','initial','maximum'] if self.operation=='create' else ['heap','flags','bytes']
        formals=['options','initial','maximum'] if self.operation=='create' else ['handle','flags','size']
        for name,formal in zip(fields,formals):entry[name]=int(frame.read_var(formal))&0xffffffff
        heaps.append(entry);save_heaps();HeapReturned(frame,entry);return False
HeapEntered('recon_HeapCreate','create');HeapEntered('recon_HeapAlloc','allocate');save_heaps()
class FreeEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('recon_HeapFree',internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_release':return False
        entry=dict(operation='free',caller=caller.name(),last_error_before=int(gdb.parse_and_eval('last_error')))
        for name,formal in [('heap','handle'),('flags','flags'),('payload','payload')]:entry[name]=int(frame.read_var(formal))&0xffffffff
        heaps.append(entry);save_heaps();HeapReturned(frame,entry);return False
FreeEntered()


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

write_path=path.parent/'writes.json'
writes=[]
class WriteReturned(gdb.FinishBreakpoint):
    def stop(self):
        writes.append(int(gdb.parse_and_eval('$eax'))&0xffffffff)
        write_path.write_text(json.dumps(writes)+'\n');return False
class WriteEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('rc_write_bytes',internal=True)
    def stop(self):
        if not writes:
            self.enabled=False;WriteReturned(gdb.newest_frame(),internal=True)
        return False
WriteEntered();write_path.write_text('[]\n')

cpinfo_path=path.parent/'cpinfo.json'
cpinfo=[]
class CPInfoReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry,pointer):self.entry=entry;self.pointer=pointer;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax')),after=bytes(gdb.selected_inferior().read_memory(self.pointer,20)).hex(),last_error_after=int(gdb.parse_and_eval('last_error')))
        cpinfo_path.write_text(json.dumps(cpinfo)+'\n');return False
class CPInfoEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('recon_GetCPInfo',internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_is_text_unicode':return False
        pointer=int(frame.read_var('info'));entry=dict(page=int(frame.read_var('page')),before=bytes(gdb.selected_inferior().read_memory(pointer,20)).hex(),last_error_before=int(gdb.parse_and_eval('last_error')))
        cpinfo.append(entry);CPInfoReturned(frame,entry,pointer);return False
CPInfoEntered();cpinfo_path.write_text('[]\n')
unicode_path=path.parent/'unicode-results.json'
unicode_results=[]
class UnicodeReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,pointer):self.pointer=pointer;super().__init__(frame,internal=True)
    def stop(self):
        unicode_results.append(dict(result=int(gdb.parse_and_eval('$eax')),flags=int.from_bytes(bytes(gdb.selected_inferior().read_memory(self.pointer,4)),'little')))
        unicode_path.write_text(json.dumps(unicode_results)+'\n');return False
class UnicodeEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('rc_is_text_unicode',internal=True)
    def stop(self):
        frame=gdb.newest_frame();UnicodeReturned(frame,int(frame.read_var('flags')));return False
UnicodeEntered();unicode_path.write_text('[]\n')

locals_path=path.parent/'locals.json'
local_records=[]
class LocalReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):self.record=record;super().__init__(frame,internal=True)
    def stop(self):
        self.record['result']=int(gdb.parse_and_eval('$eax'))&0xffffffff
        if self.record['operation']=='allocate':self.record['contents']=bytes(gdb.selected_inferior().read_memory(self.record['result'],self.record['bytes'])).hex()
        cursor=gdb.parse_and_eval('native_local_memory');live=[]
        while int(cursor):
            entry=cursor.dereference();live.append(dict(pointer=int(entry['memory']),bytes=int(entry['requested_size']),flags=int(entry['flags'])));cursor=entry['next']
        self.record['live']=live
        local_records.append(self.record);locals_path.write_text(json.dumps(local_records)+'\n');return False
class LocalEntered(gdb.Breakpoint):
    def __init__(self,name,operation):self.operation=operation;super().__init__(name,internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_read_wide_line':return False
        if len(local_records)>=2:return False
        self.enabled=False
        record=dict(operation=self.operation)
        if self.operation=='allocate':record.update(flags=int(frame.read_var('flags')),bytes=int(frame.read_var('bytes')),input_file=int(caller.read_var('file')))
        else:
            pointer=int(frame.read_var('memory'));record.update(pointer=pointer,contents=bytes(gdb.selected_inferior().read_memory(pointer,4177)).hex())
        LocalReturned(frame,record);return False
LocalEntered('recon_LocalAlloc','allocate');LocalEntered('recon_LocalFree','free');locals_path.write_text('[]\n')
ctype_path=path.parent/'ctype.json'
ctype_records=[]
class CTypeReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):self.record=record;super().__init__(frame,internal=True)
    def stop(self):
        self.record['result']=int(gdb.parse_and_eval('$eax'));ctype_records.append(self.record);ctype_path.write_text(json.dumps(ctype_records)+'\n');return False
class CTypeEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('recon_iswctype',internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_prepare_input':return False
        if ctype_records:return False
        self.enabled=False
        CTypeReturned(frame,dict(character=int(frame.read_var('character')),mask=int(frame.read_var('mask'))));return False
CTypeEntered();ctype_path.write_text('[]\n')

towupper_path=path.parent/'towupper.json'
towupper_records=[]
class UpperReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,value):self.value=value;super().__init__(frame,internal=True)
    def stop(self):
        towupper_records.append(dict(character=self.value,result=int(gdb.parse_and_eval('$eax'))&65535))
        towupper_path.write_text(json.dumps(towupper_records)+'\n');return False
class UpperEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('recon_towupper',internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_prefix_equal':return False
        if len(towupper_records)>=1:self.enabled=False
        UpperReturned(frame,int(frame.read_var('character')));return False
UpperEntered();towupper_path.write_text('[]\n')
prefix_path=path.parent/'prefix-results.json'
prefix_records=[]
class PrefixReturned(gdb.FinishBreakpoint):
    def stop(self):
        prefix_records.append(int(gdb.parse_and_eval('$eax')));prefix_path.write_text(json.dumps(prefix_records)+'\n');return False
class PrefixEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('rc_prefix_equal',internal=True)
    def stop(self):
        self.enabled=False;PrefixReturned(gdb.newest_frame(),internal=True);return False
PrefixEntered();prefix_path.write_text('[]\n')
format_path=path.parent/'line-format.json'
format_records=[]
class LineFormatReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry,pointer):self.entry=entry;self.pointer=pointer;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax')),after=bytes(gdb.selected_inferior().read_memory(self.pointer,8196)).hex())
        format_records.append(self.entry);format_path.write_text(json.dumps(format_records)+'\n');return False
class LineFormatEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('recon_swprintf',internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_prepare_input':return False
        pointer=int(frame.read_var('out'));format=int(frame.read_var('format'));base=int(frame.read_register('ebp'))
        self.enabled=False
        entry=dict(format=bytes(gdb.selected_inferior().read_memory(format,22)).hex(),number=int.from_bytes(bytes(gdb.selected_inferior().read_memory(base+16,4)),'little',signed=True),before=bytes(gdb.selected_inferior().read_memory(pointer,8196)).hex())
        LineFormatReturned(frame,entry,pointer);return False
LineFormatEntered();format_path.write_text('[]\n')

numeric_path=path.parent/'numeric-conversion.json'
numeric=[]
class NumericReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax'))&0xffffffff,last_error_after=int(gdb.parse_and_eval('last_error')),errno_after=int(gdb.parse_and_eval('*(int *)&errno')),
            after=bytes(gdb.selected_inferior().read_memory(self.entry['text'],len(bytes.fromhex(self.entry['before'])))).hex())
        numeric_path.write_text(json.dumps(numeric,indent=2)+'\n');return False
class NumericEntered(gdb.Breakpoint):
    def __init__(self):super().__init__('recon_wcstol',internal=True)
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_option_number':return False
        self.enabled=False
        from importlib.machinery import SourceFileLoader
        observer=SourceFileLoader('native_preprocess',str(Path(os.environ['RCDLL_NATIVE_SCRIPT']).parent/'native-preprocess.py')).load_module()
        pointer=int(frame.read_var('text'))
        entry=dict(caller=caller.name(),text=pointer,end=int(frame.read_var('end')),base=int(frame.read_var('base')),before=observer.wide_bytes(pointer),last_error_before=int(gdb.parse_and_eval('last_error')),errno_before=int(gdb.parse_and_eval('*(int *)&errno')))
        numeric.append(entry);NumericReturned(frame,entry);return False
NumericEntered();numeric_path.write_text('[]\n')

if os.environ.get('RCDLL_OBSERVE_ERRNO')=='1':
    errno_path=path.parent/'errno-history.json'
    errno_history=[]
    def record_errno(phase):
        frame=gdb.newest_frame();frames=[]
        while frame and len(frames)<10:
            frames.append(dict(function=frame.name(),pc=int(frame.pc())))
            frame=frame.older()
        capacity=int(gdb.parse_and_eval('recon_nhandle'))
        lowio=[dict(slot=index,flags=int(gdb.parse_and_eval('recon_osfile[%d]'%index)))for index in range(capacity)if int(gdb.parse_and_eval('recon_osfile[%d]'%index))&1]
        iob=[]
        for index in range(20):
            entry=gdb.parse_and_eval('recon_iob[%d]'%index)
            if int(entry['_flag']):iob.append(dict(slot=index,flags=int(entry['_flag']),descriptor=int(entry['_file'])))
        errno_history.append(dict(phase=phase,value=int(gdb.parse_and_eval('*(int *)&errno')),frames=frames,lowio_capacity=capacity,lowio=lowio,iob=iob))
        errno_path.write_text(json.dumps(errno_history,indent=2)+'\n')
    class ErrnoWrite(gdb.Breakpoint):
        def stop(self):record_errno('write');return False
    class ErrnoStart(gdb.Breakpoint):
        def stop(self):
            self.enabled=False;record_errno('host-main')
            address=int(gdb.parse_and_eval('&errno'))
            ErrnoWrite('*(int *)'+str(address),type=gdb.BP_WATCHPOINT,wp_class=gdb.WP_WRITE,internal=True)
            return False
    class ErrnoBoundary(gdb.Breakpoint):
        def __init__(self,name):self.label=name;super().__init__(name,internal=True)
        def stop(self):record_errno(self.label);return False
    ErrnoStart('main',internal=True)
    ErrnoBoundary('_DllMainCRTStartup');ErrnoBoundary('RC')
