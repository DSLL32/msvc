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

output_writes=[]
def save_output_writes():
    (path.parent/'output-writes.json').write_text(json.dumps(output_writes,indent=2)+'\n')
class OutputWriteReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry,file):
        self.entry=entry;self.file=file;super().__init__(frame,internal=True)
    def stop(self):
        file=gdb.parse_and_eval('(ShimFILE *)'+str(self.file)).dereference();base=int(file['_base']);size=int(file['_ptr'])-base
        assert base and 0<=size<=int(file['_bufsiz'])
        self.entry.update(result=int(gdb.parse_and_eval('$eax')),after_file=bytes(gdb.selected_inferior().read_memory(self.file,32)).hex(),written_buffer=bytes(gdb.selected_inferior().read_memory(base,size)).hex(),errno_after=int(gdb.parse_and_eval('*(int *)&errno')),last_error_after=int(gdb.parse_and_eval('last_error')))
        save_output_writes();return False
class OutputWriteEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller is None or caller.name()!='rc_write_preprocessed':return False
        self.enabled=False;data=int(frame.read_var('p'));size=int(frame.read_var('size'));count=int(frame.read_var('count'));file=int(frame.read_var('s'))
        entry=dict(args=dict(data=data,size=size,count=count,file=file,formal_bytes={field:caller.read_var(field).type.sizeof for field in ['data','size','count','file']},payload=bytes(gdb.selected_inferior().read_memory(data,size*count)).hex()),before_file=bytes(gdb.selected_inferior().read_memory(file,32)).hex(),errno_before=int(gdb.parse_and_eval('*(int *)&errno')),last_error_before=int(gdb.parse_and_eval('last_error')))
        output_writes.append(entry);save_output_writes();OutputWriteReturned(frame,entry,file);return False
OutputWriteEntered('recon_fwrite',internal=True);save_output_writes()


macro_listings=[]
def listing_memory(pointer,size):return bytes(gdb.selected_inferior().read_memory(pointer,size)).hex()
def listing_wide(pointer):
    raw=bytearray()
    for offset in range(257):
        value=bytes(gdb.selected_inferior().read_memory(pointer+offset*2,2));raw.extend(value)
        if value==b'\0\0':return raw.hex()
    raise AssertionError('unterminated listing field')
def save_macro_listings():(path.parent/'macro-listings.json').write_text(json.dumps(macro_listings,indent=2)+'\n')
class MacroListingReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):super().__init__(frame,internal=True);self.entry=entry
    def stop(self):
        self.entry.update(return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff,after=listing_memory(self.entry['pointer'],16),enabled_after=int(gdb.parse_and_eval('rc_macro_listing_enabled')))
        save_macro_listings();return False
class MacroListingEntered(gdb.Breakpoint):
    def stop(self):
        if macro_listings:self.enabled=False;return False
        frame=gdb.newest_frame();macro=frame.read_var('macro');value=macro.dereference();parent=frame.older();marker=parent.read_var('marker')
        entry=dict(pointer=int(macro),object=int(marker.address),object_bytes=marker.type.sizeof,formal_bytes=macro.type.sizeof,parent=parent.name(),record=listing_memory(int(macro),16),name=int(value['name']),name_bytes=listing_wide(int(value['name'])),text=int(value['text']),text_bytes=listing_wide(int(value['text'])),marker_global=int(gdb.parse_and_eval('&rc_source_marker')),marker_global_bytes=gdb.parse_and_eval('rc_source_marker').type.sizeof,enabled_before=int(gdb.parse_and_eval('rc_macro_listing_enabled')),entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff)
        macro_listings.append(entry);save_macro_listings();MacroListingReturned(frame,entry);return False
MacroListingEntered('rc_list_macro',internal=True);save_macro_listings()


class_reads=[]
def save_class_reads():(path.parent/'class-reads.json').write_text(json.dumps(class_reads,indent=2)+'\n')
class CharacterClassReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax'))&0xffff,raw_eax=int(gdb.parse_and_eval('$eax'))&0xffffffff,return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff)
        save_class_reads();return False
class CharacterClassScope(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();character=int(frame.read_var('character'))
        if character==256:return False # The native target's own INT3 enforces this hold.
        parent=frame.older()
        if parent is not None and parent.name()=='rc_read_identifier'and len(class_reads)<64:
            entry=dict(character=character,formal_bytes=frame.read_var('character').type.sizeof,stack_slot_bytes=4,table_entry=int(gdb.parse_and_eval('rc_character_classes[%d]'%character))if character<256 else 16,identifier_cursor=int(parent.read_var('cursor')),identifier_first=int(parent.read_var('character')),hash=int(parent.read_var('hash')),entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff)
            class_reads.append(entry);save_class_reads();CharacterClassReturned(frame,entry)
        return False
CharacterClassScope('rc_character_class',internal=True);save_class_reads()


directive_lookups=[]
def save_directive_lookups():(path.parent/'directive-lookups.json').write_text(json.dumps(directive_lookups,indent=2)+'\n')
class DirectiveLookupReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax'))&255,raw_eax=int(gdb.parse_and_eval('$eax'))&0xffffffff,return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff)
        save_directive_lookups();return False
class DirectiveLookupEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();word=frame.read_var('word')
        entry=dict(pointer=int(word),bytes=listing_wide(int(word)),formal_bytes=word.type.sizeof,entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff)
        directive_lookups.append(entry);save_directive_lookups();DirectiveLookupReturned(frame,entry);return False
DirectiveLookupEntered('rc_directive_code',internal=True);save_directive_lookups()


logical_reads=[]
def save_logical_reads():(path.parent/'logical-reads.json').write_text(json.dumps(logical_reads,indent=2)+'\n')
class LogicalCharacterReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax'))&65535,raw_eax=int(gdb.parse_and_eval('$eax'))&0xffffffff,return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff,cursor_after=int(gdb.parse_and_eval('rc_source_cursor')))
        save_logical_reads();return False
class LogicalCharacterEntered(gdb.Breakpoint):
    def stop(self):
        cursor=int(gdb.parse_and_eval('rc_source_cursor'));frame=gdb.newest_frame()
        entry=dict(formals=[],cursor_before=cursor,word=int.from_bytes(bytes(gdb.selected_inferior().read_memory(cursor,2)),'little'),depth=int(gdb.parse_and_eval('rc_source_depth')),alternate_mode=int(gdb.parse_and_eval('rc_number_buffer_active')),entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff)
        logical_reads.append(entry);save_logical_reads();LogicalCharacterReturned(frame,entry);return False
LogicalCharacterEntered('rc_get_logical_character',internal=True);save_logical_reads()


integer_conversions=[];digit_values=[]
def save_integer_conversions():(path.parent/'integer-conversions.json').write_text(json.dumps(integer_conversions,indent=2)+'\n')
def save_digit_values():(path.parent/'digit-values.json').write_text(json.dumps(digit_values,indent=2)+'\n')
class IntegerConversionReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax'))&0xffffffff,return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff)
        save_integer_conversions();return False
class IntegerConversionEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();text=frame.read_var('text');radix=frame.read_var('radix')
        entry=dict(text=int(text),text_bytes=listing_wide(int(text)),radix=int(radix),formal_bytes=dict(text=text.type.sizeof,radix=radix.type.sizeof),entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff)
        integer_conversions.append(entry);save_integer_conversions();IntegerConversionReturned(frame,entry);return False
class DigitValueReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax'))&0xffffffff,return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff)
        save_digit_values();return False
class DigitValueEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();parent=frame.older();character=frame.read_var('character')
        entry=dict(character=int(character),formal_bytes=character.type.sizeof,parent_cursor=int(parent.read_var('cursor')),accumulator=int(parent.read_var('value')),previous=int(parent.read_var('previous')),entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff)
        digit_values.append(entry);save_digit_values();DigitValueReturned(frame,entry);return False
IntegerConversionEntered('rc_parse_integer_value',internal=True);save_integer_conversions()
DigitValueEntered('rc_digit_value',internal=True);save_digit_values()


integer_kinds=[]
def save_integer_kinds():(path.parent/'integer-kinds.json').write_text(json.dumps(integer_kinds,indent=2)+'\n')
class DecimalIntegerKindReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(result=int(gdb.parse_and_eval('$eax'))&255,raw_eax=int(gdb.parse_and_eval('$eax'))&0xffffffff,return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff)
        save_integer_kinds();return False
class DecimalIntegerKindEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();value=frame.read_var('value')
        entry=dict(value=int(value),formal_bytes=value.type.sizeof,entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff)
        integer_kinds.append(entry);save_integer_kinds();DecimalIntegerKindReturned(frame,entry);return False
DecimalIntegerKindEntered('rc_decimal_integer_kind',internal=True);save_integer_kinds()

interned_types=[]
def save_interned_types():(path.parent/'interned-types.json').write_text(json.dumps(interned_types,indent=2)+'\n')
class InternedTypeReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        result=int(gdb.parse_and_eval('$eax'))&0xffffffff
        self.entry.update(result=result,return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff,result_block=bytes(gdb.selected_inferior().read_memory(result-12,32)).hex(),buckets_after=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_type_buckets')),1024)).hex())
        save_interned_types();return False
class InternedTypeEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();parent=frame.older();arg=frame.read_var('type');node=int(parent.read_var('node'));buckets=gdb.parse_and_eval('rc_type_buckets')
        entry=dict(pointer=int(arg),formal_bytes=arg.type.sizeof,object_bytes=arg.dereference().type.sizeof,entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff,payload=bytes(gdb.selected_inferior().read_memory(int(arg),20)).hex(),parent=parent.name(),node=node,node_bytes=bytes(gdb.selected_inferior().read_memory(node,20)).hex(),buckets_address=int(buckets.address),buckets_bytes=buckets.type.sizeof,buckets_before=bytes(gdb.selected_inferior().read_memory(int(buckets.address),1024)).hex())
        interned_types.append(entry);save_interned_types();InternedTypeReturned(frame,entry);return False
InternedTypeEntered('rc_intern_type',internal=True);save_interned_types()

string_reads=[]
def save_string_reads():(path.parent/'string-reads.json').write_text(json.dumps(string_reads,indent=2)+'\n')
class StringReaderReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff,cursor_after=int(gdb.parse_and_eval('rc_source_cursor')),token_value_after=int(gdb.parse_and_eval('rc_token_value')),extra_after=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_token_extra')),2)).hex(),buffer_after=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&rc_string_buffer')),4090)).hex())
        save_string_reads();return False
class StringReaderEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();cursor=int(gdb.parse_and_eval('rc_source_cursor'));buffer=gdb.parse_and_eval('rc_string_buffer');extra=gdb.parse_and_eval('rc_token_extra');extra_address=int(extra.address)
        entry=dict(formals=[],caller=frame.older().name(),cursor=cursor,current_word=int(gdb.parse_and_eval('*rc_source_cursor')),header_mode=int(gdb.parse_and_eval('rc_header_mode')),output_active=int(gdb.parse_and_eval('rc_cpp_active')),in_directive=int(gdb.parse_and_eval('rc_in_directive')),token_kind=int(gdb.parse_and_eval('rc_token_kind')),token_value=int(gdb.parse_and_eval('rc_token_value')),entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff,buffer=int(buffer.address),buffer_before=bytes(gdb.selected_inferior().read_memory(int(buffer.address),buffer.type.sizeof)).hex(),extra_before=bytes(gdb.selected_inferior().read_memory(extra_address,2)).hex(),layout=dict(buffer_bytes=buffer.type.sizeof,element_bytes=buffer[0].type.sizeof,buffer_alignment=int(buffer.address)%2,number_offset=int(gdb.parse_and_eval('rc_number_buffer'))-int(buffer.address),extra_bytes=extra.type.sizeof,extra_alignment=extra_address%2,hash_offset=int(extra['identifier']['hash'].address)-extra_address,length_offset=int(extra['identifier']['length'].address)-extra_address,string_length_offset=int(extra['string_length'].address)-extra_address))
        string_reads.append(entry);save_string_reads();StringReaderReturned(frame,entry);return False
StringReaderEntered('rc_read_string',internal=True);save_string_reads()

expected_ends=[]
def save_expected_ends():(path.parent/'expected-ends.json').write_text(json.dumps(expected_ends,indent=2)+'\n')
class ExpectedEndReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):self.entry=entry;super().__init__(frame,internal=True)
    def stop(self):
        self.entry.update(return_esp=int(gdb.parse_and_eval('$esp'))&0xffffffff,cursor_after=int(gdb.parse_and_eval('rc_source_cursor')))
        save_expected_ends();return False
class ExpectedEndEntered(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();word=frame.read_var('word');raw=bytearray()
        for offset in range(257):
            value=bytes(gdb.selected_inferior().read_memory(int(word)+2*offset,2));raw.extend(value)
            if value==b'\0\0':break
        else:raise AssertionError('unterminated directive-end context')
        entry=dict(word=int(word),word_bytes=raw.hex(),formal_bytes=word.type.sizeof,entry_esp=(int(gdb.parse_and_eval('$ebp'))+4)&0xffffffff,cursor=int(gdb.parse_and_eval('rc_source_cursor')),current_word=int(gdb.parse_and_eval('*rc_source_cursor')))
        expected_ends.append(entry);save_expected_ends();ExpectedEndReturned(frame,entry);return False
ExpectedEndEntered('rc_expect_end',internal=True);save_expected_ends()
