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


macro_copies=[]
macro_copy_path=path.parent/'macro-copies.json'
def save_macro_copies():macro_copy_path.write_text(json.dumps(macro_copies,indent=2)+'\n')
class MacroCopyReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        result=int(gdb.parse_and_eval('$eax'))&0xffffffff
        self.entry.update(result=result,result_block=bytes(gdb.selected_inferior().read_memory(result-8,2*self.entry['length']+10)).hex(),return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff,last_error_after=int(gdb.parse_and_eval('last_error')),errno_after=int(gdb.parse_and_eval('*(int *)&errno')))
        save_macro_copies();return False
class MacroCopyEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_define_macro':return False
        assert len(macro_copies)<6
        pointer=int(frame.read_var('value'));length=int(frame.read_var('length'));assert 0<length<=512
        maps=Path('/proc/%d/maps'%gdb.selected_inferior().pid).read_text().splitlines()
        mapping=[line for line in maps if int(line.split()[0].split('-')[0],16)<=pointer and pointer+2*length<=int(line.split()[0].split('-')[1],16)]
        assert len(mapping)==1 and mapping[0].split()[1].startswith('r')
        contract=None
        global_name=int(gdb.parse_and_eval('&rc_macro_name'))
        if global_name<=pointer and pointer+2*length<=global_name+1024:contract='initialized-global-name'
        heap=gdb.parse_and_eval('native_heaps')
        while int(heap):
            block=heap.dereference()['blocks']
            while int(block):
                item=block.dereference();start=int(item['payload']);end=start+int(item['requested_size'])
                if start<=pointer and pointer+2*length<=end:contract='requested-private-heap'
                block=item['next']
            heap=heap.dereference()['next']
        if contract is None and length==3 and mapping[0].split()[1].startswith('r-') and bytes(gdb.selected_inferior().read_memory(pointer,6))==b'1\0\0\0\0\0':contract='six-byte-constant-literal'
        if contract is None:raise gdb.GdbError('unsupported macro-copy source outside initialized owned extent')
        entry=dict(source_contract=contract,source=pointer,length=length,source_bytes=bytes(gdb.selected_inferior().read_memory(pointer,2*length)).hex(),entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff,source_mapping=mapping[0],last_error_before=int(gdb.parse_and_eval('last_error')),errno_before=int(gdb.parse_and_eval('*(int *)&errno')))
        macro_copies.append(entry);save_macro_copies();MacroCopyReturned(frame,entry);return False
MacroCopyEntered('rc_copy_macro_text',internal=True);save_macro_copies()


wide_initial_path=path.parent/'wide-initial.json'
wide_initial={}
def save_wide_initial():wide_initial_path.write_text(json.dumps(wide_initial,indent=2)+'\n')
class MacroTrimWrite(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();assert frame.name()=='rc_open_preprocessor'
        value=bytes(gdb.selected_inferior().read_memory(wide_initial['trim_pointer'],2)).hex()
        eax=int(gdb.parse_and_eval('$eax'))&0xffffffff
        wide_initial['writes'].append(dict(function=frame.name(),pc=int(frame.pc()),after=value,eax=eax))
        gdb.write('RCDLL_TRIM_WRITE=%08x %s %08x %s %08x\n'%(wide_initial['trim_pointer'],frame.name(),int(frame.pc()),value,eax))
        self.enabled=False;save_wide_initial();return False
class WideInitial(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();count=int(frame.read_var('count'));vector=frame.read_var('arguments')
        assert count==20
        allocations=[record for record in heaps if record['operation']=='allocate'];assert len(allocations)==5
        size=allocations[4]['bytes'];pointer=int(vector)
        raw=bytes(gdb.selected_inferior().read_memory(pointer-8,size))
        final=int(vector[19]);value=bytearray()
        for offset in range(512):
            word=bytes(gdb.selected_inferior().read_memory(final+2*offset,2));value.extend(word)
            if word==b'\0\0':break
        else:raise AssertionError('unbounded final definition')
        wide_initial.update(pointer=pointer,requested_size=size,bytes=raw.hex(),last_argument=bytes(value).hex(),writes=[])
        if bytes(value).endswith(b' \0\0\0'):
            wide_initial['trim_pointer']=final+len(value)-4
            MacroTrimWrite('*(unsigned short *)'+str(wide_initial['trim_pointer']),type=gdb.BP_WATCHPOINT,wp_class=gdb.WP_WRITE,internal=True)
        save_wide_initial();return False
WideInitial('rc_preprocessor_main',internal=True);save_wide_initial()


class SourceOpened(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();depth=int(gdb.parse_and_eval('rc_source_depth'))
        if depth!=-1:return False
        pointer=int(gdb.parse_and_eval('&rc_source_frames'));data=bytes(gdb.selected_inferior().read_memory(pointer,36*1024))
        entry={field:int(frame.read_var(field))for field in ['path','first']};entry.update(depth=depth,frames=data.hex())
        (path.parent/'source-entry.json').write_text(json.dumps(entry,indent=2)+'\n');self.enabled=False;return False
SourceOpened('rc_open_source',internal=True)

refill_records=[]
def save_refills():
    (path.parent/'refills.json').write_text(json.dumps(refill_records,indent=2)+'\n')
class RefillReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):
        self.entry=entry
        super().__init__(frame,internal=True)
    def stop(self):
        self.entry['result']=int(gdb.parse_and_eval('$eax'))&0xffffffff
        save_refills();return False
class RefillEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();depth=int(gdb.parse_and_eval('rc_source_depth'))
        assert depth==0 and not refill_records
        self.enabled=False
        source=gdb.parse_and_eval('rc_source_frames[0]');buffer=int(source['buffer']);file=int(source['file'])
        file_value=source['file'].dereference();base=int(file_value['_base']);size=int(file_value['_bufsiz'])
        entry=dict(depth=depth,cursor=int(gdb.parse_and_eval('rc_source_cursor')),encoding=int(gdb.parse_and_eval('rc_source_encoding')),frame=bytes(gdb.selected_inferior().read_memory(int(source.address),36)).hex(),buffer=bytes(gdb.selected_inferior().read_memory(buffer-8,8206)).hex(),file=bytes(gdb.selected_inferior().read_memory(file,32)).hex(),file_buffer=bytes(gdb.selected_inferior().read_memory(base,size)).hex(),formals=[])
        refill_records.append(entry);save_refills();RefillReturned(frame,entry);return False
RefillEntered('rc_refill_source',internal=True);save_refills()

name_copies=[]
def save_name_copies():
    (path.parent/'source-name-copies.json').write_text(json.dumps(name_copies,indent=2)+'\n')
class SourceNameReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):
        self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        result=int(gdb.parse_and_eval('$eax'))&0xffffffff;size=len(bytes.fromhex(self.entry['bytes']))+8
        self.entry.update(result=result,result_block=bytes(gdb.selected_inferior().read_memory(result-8,size)).hex())
        save_name_copies();return False
class SourceNameEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();pointer=int(frame.read_var('path'));value=bytearray()
        for offset in range(257):
            word=bytes(gdb.selected_inferior().read_memory(pointer+2*offset,2));value.extend(word)
            if word==b'\0\0':break
        else:raise AssertionError('source-name input has no bounded terminator')
        assert not name_copies;self.enabled=False
        entry=dict(path=pointer,bytes=bytes(value).hex(),formal_bytes=4);name_copies.append(entry);save_name_copies();SourceNameReturned(frame,entry);return False
SourceNameEntered('rc_duplicate_source_name',internal=True);save_name_copies()

source_formats=[]
def save_source_formats():
    (path.parent/'source-line-format.json').write_text(json.dumps(source_formats,indent=2)+'\n')
class SourceFormatReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry,pointer):
        self.entry=entry;self.pointer=pointer;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax')),after=bytes(gdb.selected_inferior().read_memory(self.pointer,1024)).hex(),errno_after=int(gdb.parse_and_eval('*(int *)&errno')),last_error_after=int(gdb.parse_and_eval('last_error')))
        save_source_formats();return False
class SourceFormatEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_emit_source_line':return False
        self.enabled=False;pointer=int(frame.read_var('out'));format=int(frame.read_var('format'));base=int(frame.read_register('ebp'))
        entry=dict(destination=pointer,format=bytes(gdb.selected_inferior().read_memory(format,20)).hex(),number=int.from_bytes(bytes(gdb.selected_inferior().read_memory(base+16,4)),'little',signed=True),before=bytes(gdb.selected_inferior().read_memory(pointer,1024)).hex(),errno_before=int(gdb.parse_and_eval('*(int *)&errno')),last_error_before=int(gdb.parse_and_eval('last_error')))
        source_formats.append(entry);save_source_formats();SourceFormatReturned(frame,entry,pointer);return False
SourceFormatEntered('recon_swprintf',internal=True);save_source_formats()
