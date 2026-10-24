"""GDB-side typed state capture, copied into each raw native evidence directory."""
import json
import os
import stat
import hashlib
from pathlib import Path
import gdb


file_calls=[]
mapping_calls=[]
file_reads=[]
directory_allocation={}
subsection_allocations=[]
image_allocations=[]
checked_allocations=[]
cleared_allocations=[]
module_allocations=[]
widen_factory_calls=[]
widen_query_calls=[]
widen_emit_calls=[]
widen_release_calls=[]
widen_sequence=[]
main_free_calls=[]
lookup_calls=[]
lookup_stack=[]
type_compare_calls=[]
live_mappings={}
def install(configuration):
    config=json.loads(Path(configuration).read_text())
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,call):
            self.call=call
            super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            self.call['handle']=int(self.return_value)&0xffffffff
            self.call['last_error']=int(gdb.parse_and_eval("'win32.c'::last_error"))
            return False
    class Opened(gdb.Breakpoint):
        def __init__(self):super().__init__('recon_CreateFileA',internal=True)
        def stop(self):
            frame=gdb.newest_frame()
            call={name:int(frame.read_var(name))&0xffffffff for name in
                  ('access','share','security','disposition','attributes','template')}
            call['pathname']=(frame.read_var('name').string(encoding='latin1').encode('latin1')+b'\0').hex()
            file_calls.append(call);Returned(call)
            return False
    Opened()
    class MappingReturned(gdb.FinishBreakpoint):
        def __init__(self,call):
            self.call=call
            super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            call=self.call;call['result']=int(self.return_value)&0xffffffff
            call['last_error']=int(gdb.parse_and_eval("'win32.c'::last_error"))
            if call['kind']=='create' and call['result']:
                node=gdb.parse_and_eval("'win32.c'::mappings")
                found=[]
                while int(node):
                    value=node.dereference()
                    if int(node)==call['result']:found.append(value)
                    node=value['next']
                assert len(found)==1,'actual mapping HANDLE absent from typed registry'
                value=found[0];source=call['args'][0]-1;duplicate=int(value['fd'])
                proc=Path('/proc')/str(gdb.selected_inferior().pid)/'fd'
                left=os.stat(proc/str(source));right=os.stat(proc/str(duplicate))
                assert (left.st_dev,left.st_ino)==(right.st_dev,right.st_ino),'mapping source identity differs'
                call['registry']=dict(length=int(value['length']),writable=int(value['writable']),source_owned=True)
                live_mappings[call['result']]=call
            elif call['kind']=='close':
                assert call['args'][0] in live_mappings
                live_mappings.pop(call['args'][0])
                node=gdb.parse_and_eval("'win32.c'::mappings")
                while int(node):
                    assert int(node)!=call['args'][0],'closed mapping HANDLE remains registered'
                    node=node.dereference()['next']
            return False
    class MappingEntered(gdb.Breakpoint):
        def __init__(self,symbol,kind,names):
            self.kind,self.names=kind,names
            super().__init__(symbol,internal=True)
        def stop(self):
            frame=gdb.newest_frame()
            args=[int(frame.read_var(name))&0xffffffff for name in self.names]
            if self.kind=='close' and args[0] not in live_mappings:return False
            call=dict(kind=self.kind,args=args)
            mapping_calls.append(call);MappingReturned(call)
            return False
    MappingEntered('recon_CreateFileMappingA','create',('file','security','protection','high','low','name'))
    MappingEntered('recon_MapViewOfFileEx','map',('handle','access','high','low','bytes','base'))
    MappingEntered('recon_CloseHandle','close',('handle',))
    class ReadReturned(gdb.FinishBreakpoint):
        def __init__(self,call,address):
            self.call,self.address=call,address
            super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            row=self.call;returned=int(self.return_value)&0xffffffff
            assert returned==row['size'],'partial read needs a defined-tail contract'
            row.update(returned=returned,exit_position=int(gdb.parse_and_eval('cached_files[%d]->position'%row['file'])),
                exit_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")),
                bytes=bytes(gdb.selected_inferior().read_memory(self.address,returned)).hex())
            return False
    class ReadEntered(gdb.Breakpoint):
        def __init__(self):super().__init__('read_cached_file',internal=True)
        def stop(self):
            frame=gdb.newest_frame();owner=frame.older()
            assert owner is not None
            buffer=int(frame.read_var('buffer'));size=int(frame.read_var('size'));file=int(frame.read_var('file'))
            roles=[]
            for site in config.get('read_sites',[]):
                if site['owner']!=owner.name():continue
                local=gdb.parse_and_eval(site['local']) if site.get('global_buffer') or (site.get('heap_buffer') and not site.get('count_parameter')) else owner.read_var(site['local'])
                if site.get('segment_input'):
                    segment=int(gdb.parse_and_eval('type_segments[0]'))
                    capacity=min(int(gdb.parse_and_eval('type_segment_bytes')),0xffe8)
                    assert int(gdb.parse_and_eval('type_segment_index'))==0,'later segment allocation requires its own observation'
                    matches=int(local)+int(owner.read_var('carried'))==buffer and size==int(owner.read_var('amount')) and segment<=buffer and buffer+size<=segment+capacity
                elif site.get('allocated_global'):
                    capacity=int(gdb.parse_and_eval(site['capacity_global']))
                    if 'capacity_max' in site:capacity=min(capacity,site['capacity_max'])
                    count=site['count_literal'] if 'count_literal' in site else int(owner.read_var(site['count_parameter']).dereference()[site['count_member']])
                    matches=int(local)==buffer and size==count and size<=capacity
                elif site.get('count_parameter'):
                    local=gdb.parse_and_eval(site['local']) if site.get('global_buffer') else owner.read_var(site['local']);entry=owner.read_var(site['count_parameter'])
                    capacity=int(entry.dereference()[site['count_member']])
                    entry_index=(int(entry)-int(gdb.parse_and_eval('directory_entries')))//12
                    matches=int(local)==buffer and size==capacity and dict(address=buffer,size=capacity,entry_index=entry_index) in (module_allocations if site.get('allocation_kind')=='module' else subsection_allocations)
                elif site.get('heap_buffer'):
                    local=gdb.parse_and_eval(site['local']);count=int(gdb.parse_and_eval(site['count']))
                    capacity=(count+site['spare_elements'])*site['element_size']
                    matches=int(local)==buffer and size==count*site['element_size'] and directory_allocation==dict(address=buffer,size=capacity)
                else:
                    capacity=site['capacity'];matches=int(local.address)==buffer and local.type.sizeof==capacity and size<=capacity
                if matches:
                    role=dict(owner=site['owner'],local=site['local'],capacity=capacity)
                    if site.get('segment_input'):role.update(segment_index=0,segment_offset=buffer-int(gdb.parse_and_eval('type_segments[0]')))
                    if role not in roles:roles.append(role)
            assert len(roles)==1,'read buffer actual caller/local/extent is unobserved'
            call=dict(**roles[0],file=file,size=size,entry_position=int(gdb.parse_and_eval('cached_files[%d]->position'%file)),
                entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
            file_reads.append(call);ReadReturned(call,buffer)
            return False
    ReadEntered()
    class AllocationReturned(gdb.FinishBreakpoint):
        def __init__(self,record):
            self.record=record
            super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            self.record['address']=int(gdb.parse_and_eval('$eax'))&0xffffffff
            return False
    class AllocationEntered(gdb.Breakpoint):
        def __init__(self):super().__init__('*malloc',internal=True)
        def stop(self):
            frame=gdb.newest_frame();owner=frame.older()
            if owner is None or owner.name() not in ('read_codeview_directory','load_debug_subsection','read_image_debug','allocate_bytes','initialize_single_module','allocate_cleared','rewrite_module_symbols','rewrite_module_global_types'):return False
            stack=int(gdb.parse_and_eval('$esp'))
            size=int.from_bytes(bytes(gdb.selected_inferior().read_memory(stack+4,4)),'little')
            if owner.name()=='read_codeview_directory':
                assert not directory_allocation,'directory allocation generation needs an explicit model'
                directory_allocation['size']=size;record=directory_allocation
            elif owner.name()=='allocate_cleared':
                record=dict(size=size);cleared_allocations.append(record)
            elif owner.name()=='allocate_bytes':
                record=dict(size=size);checked_allocations.append(record)
            elif owner.name()=='rewrite_module_symbols':
                entry=owner.read_var('entry');info=owner.read_var('info')
                record=dict(size=size,owner=owner.name(),source=dict(
                    entry_index=(int(entry)-int(gdb.parse_and_eval('directory_entries')))//12,
                    module=int(info.dereference()['module']),input_size=int(entry.dereference()['size']),
                    symbols=int(owner.read_var('symbols').dereference()),references=int(owner.read_var('references').dereference())))
                image_allocations.append(record)
            elif owner.name()=='read_image_debug':
                record=dict(size=size);image_allocations.append(record)
            else:
                entry=int(owner.read_var('entry'));root=int(gdb.parse_and_eval('directory_entries'))
                assert entry>=root and (entry-root)%12==0
                record=dict(size=size,entry_index=(entry-root)//12);(module_allocations if owner.name()=='initialize_single_module' else subsection_allocations).append(record)
            AllocationReturned(record);return False
    AllocationEntered()

    if config.get('observe_main_frees'):
        class MainFreeReturned(gdb.FinishBreakpoint):
            def __init__(self,call):
                self.call=call
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                self.call.update(returned=True,exit_esp=int(gdb.parse_and_eval('$esp')),
                    exit_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                return False
        class MainFreeEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('recon_free',internal=True)
            def stop(self):
                frame=gdb.newest_frame();owner=frame.older()
                if owner is None or owner.name() not in ('cvpack_main','free_widen_buffers'):return False
                names=['debug_directory','line_data','symbol_data','type_data','module_data']
                capacities=['module_capacity','type_segment_bytes','symbol_capacity','line_capacity','type_capacity']
                if config.get('observe_widen_frees'):
                    names+=['pack_buffer','pack_fixups'];capacities+=['pack_buffer_size','pack_fixup_count']
                index=len(main_free_calls);assert index<len(names),'unexpected additional main free'
                name=names[index];address=int(frame.read_var('pointer'))
                assert owner.name()==('cvpack_main' if index<5 else 'free_widen_buffers'),'cleanup free owner/order differs'
                assert address==int(gdb.parse_and_eval(name)),'main free argument is not its actual source global'
                size=int(gdb.parse_and_eval(capacities[index])) if address else 0
                if index==1:size=min(size,0xffe8)
                if index==6:size*=8
                assert 0<=size<=65536,'main free exceeds observed allocation bound'
                inferior=gdb.selected_inferior();call=dict(name=name,index=index,address=address,size=size,
                    before=bytes(inferior.read_memory(address,size)).hex() if address else '',
                    entry_esp=int(gdb.parse_and_eval('$ebp'))+4,
                    global_roots={key:int(gdb.parse_and_eval(key)) for key in names},
                    segment0=int(gdb.parse_and_eval('type_segments[0]')),
                    symbol_cursor=int(gdb.parse_and_eval('symbol_input')),
                    module_symbol_records=int(gdb.parse_and_eval('module_symbol_records')),
                    owner=owner.name(),entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                main_free_calls.append(call);MainFreeReturned(call);return False
        MainFreeEntered()

    if config.get('observe_widen_factory'):
        class WidenFactoryReturned(gdb.FinishBreakpoint):
            def __init__(self,call):
                self.call=call
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                call=self.call;inferior=gdb.selected_inferior()
                call.update(result=int(self.return_value),object=int.from_bytes(bytes(inferior.read_memory(call['output_argument'],4)),'little'),
                    exit_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                assert call['object']==int(gdb.parse_and_eval("&'native-widen.c'::opaque")),'factory did not return the actual provider object'
                table=int.from_bytes(bytes(inferior.read_memory(call['object'],4)),'little')
                slots=bytes(inferior.read_memory(table,40));names=['destroy','unknown','unknown','record','unknown','unknown','unknown','unknown','query','emit']
                expected=[int(gdb.parse_and_eval("&'native-widen.c'::"+name)) for name in names]
                assert [int.from_bytes(slots[i:i+4],'little') for i in range(0,40,4)]==expected,'provider vtable typed function identities differ'
                widen_sequence.append('factory-return')
                call['vtable_roles']=names
                call['vtable_slots']=[int.from_bytes(slots[i:i+4],'little') for i in range(0,40,4)]
                call['provider_functions']={name:int(gdb.parse_and_eval("&'native-widen.c'::"+name)) for name in set(names)}
                return False
        class WidenFactoryEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('cvpack_widen_factory',internal=True)
            def stop(self):
                frame=gdb.newest_frame();out=int(frame.read_var('out'));owner=frame.older()
                while owner is not None and owner.name()!='widen_subsection':owner=owner.older()
                assert owner is not None and int(owner.read_var('object').address)==out,'factory output is not actual caller local object'
                call=dict(output_argument=out,capacity=int(frame.read_var('capacity')),flags=int(frame.read_var('flags')),
                    entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")),owner='widen_subsection',local='object',instance=len(widen_factory_calls))
                widen_sequence.append('factory-call');widen_factory_calls.append(call);WidenFactoryReturned(call);return False
        WidenFactoryEntered()

    if config.get('observe_widen_query'):
        class WidenQueryReturned(gdb.FinishBreakpoint):
            def __init__(self,call):
                self.call=call
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                call=self.call;inferior=gdb.selected_inferior()
                call.update(result=int(self.return_value),description_after=bytes(inferior.read_memory(call['description'],16)).hex(),
                    input_after=bytes(inferior.read_memory(call['input_address'],call['size'])).hex(),
                    exit_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                widen_sequence.append('query-return')
                assert int(gdb.parse_and_eval("'native-widen.c'::stage"))==2 and int(gdb.parse_and_eval("'native-widen.c'::cursor"))==call['instance'],'query provider lifetime/state differs'
                return False
        class WidenQueryEntered(gdb.Breakpoint):
            def __init__(self):super().__init__("native-widen.c:query",internal=True)
            def stop(self):
                frame=gdb.newest_frame();owner=frame.older()
                while owner is not None and owner.name()!='widen_subsection':owner=owner.older()
                assert owner is not None
                obj=int(frame.read_var('object'));out=int(frame.read_var('out'));source=int(frame.read_var('input'));size=int(frame.read_var('size'))
                assert obj==int(owner.read_var('object'))==widen_factory_calls[-1]['object'],'query opaque object caller/factory alias differs'
                assert out==int(owner.read_var('output').address),'query output descriptor actual caller-local alias differs'
                assert source==int(owner.read_var('buffer').dereference())+4 and size==int(owner.read_var('input_size')),'query input actual caller alias/extent differs'
                inferior=gdb.selected_inferior()
                call=dict(instance=len(widen_factory_calls)-1,object=obj,description=out,input_address=source,size=size,flags=int(frame.read_var('flags')),
                    description_before=bytes(inferior.read_memory(out,16)).hex(),input=bytes(inferior.read_memory(source,size)).hex(),
                    entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                roots=[name for name in ('symbol_data','type_data','module_data') if source==int(gdb.parse_and_eval(name))+4]
                assert len(roots)==1,'query input has no unique actual source-buffer root'
                call.update(input_root=roots[0],read_count=len(file_reads))
                widen_sequence.append('query-call');widen_query_calls.append(call);WidenQueryReturned(call);return False
        WidenQueryEntered()

    if config.get('observe_widen_emit'):
        class WidenEmitReturned(gdb.FinishBreakpoint):
            def __init__(self,call):
                self.call=call
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                call=self.call;inferior=gdb.selected_inferior()
                call.update(result=int(self.return_value),description_after=bytes(inferior.read_memory(call['description'],16)).hex(),
                    input_after=bytes(inferior.read_memory(call['input_address'],call['size'])).hex(),
                    output_after=bytes(inferior.read_memory(call['output_address'],call['output_capacity'])).hex(),
                    fixups_after=bytes(inferior.read_memory(call['fixup_address'],call['fixup_capacity']*8)).hex(),
                    exit_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                widen_sequence.append('emit-return')
                assert int(gdb.parse_and_eval("'native-widen.c'::stage"))==3 and int(gdb.parse_and_eval("'native-widen.c'::cursor"))==call['instance'],'emit provider lifetime/state differs'
                return False
        class WidenEmitEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('native-widen.c:emit',internal=True)
            def stop(self):
                frame=gdb.newest_frame();owner=frame.older()
                while owner is not None and owner.name()!='widen_subsection':owner=owner.older()
                assert owner is not None
                obj=int(frame.read_var('object'));out=int(frame.read_var('out'));source=int(frame.read_var('input'));size=int(frame.read_var('size'))
                assert obj==int(owner.read_var('object'))==widen_factory_calls[-1]['object'],'emit opaque object caller/factory alias differs'
                assert out==int(owner.read_var('output').address),'emit output descriptor actual caller-local alias differs'
                assert source==int(owner.read_var('buffer').dereference())+4 and size==int(owner.read_var('input_size')),'emit input actual caller alias/extent differs'
                inferior=gdb.selected_inferior();description=bytes(inferior.read_memory(out,16));words=[int.from_bytes(description[i:i+4],'little') for i in range(0,16,4)]
                assert words[2]==int(gdb.parse_and_eval('pack_buffer')) and words[3]==int(gdb.parse_and_eval('pack_fixups')),'emit output actual allocation aliases differ'
                assert words[0]<=int(gdb.parse_and_eval('pack_buffer_size')) and words[1]<=int(gdb.parse_and_eval('pack_fixup_count')),'emit capacities exceed owned allocations'
                call=dict(instance=len(widen_factory_calls)-1,object=obj,description=out,input_address=source,size=size,flags=int(frame.read_var('flags')),
                    description_before=description.hex(),input=bytes(inferior.read_memory(source,size)).hex(),
                    output_address=words[2],fixup_address=words[3],output_capacity=words[0],fixup_capacity=words[1],
                    output_before=bytes(inferior.read_memory(words[2],words[0])).hex(),
                    fixups_before=bytes(inferior.read_memory(words[3],words[1]*8)).hex(),
                    entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                widen_sequence.append('emit-call');widen_emit_calls.append(call);WidenEmitReturned(call);return False
        WidenEmitEntered()

    if config.get('observe_widen_release'):
        class WidenReleaseReturned(gdb.FinishBreakpoint):
            def __init__(self,call):
                self.call=call
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                call=self.call
                call.update(exit_stage=int(gdb.parse_and_eval("'native-widen.c'::stage")),
                    exit_cursor=int(gdb.parse_and_eval("'native-widen.c'::cursor")),
                    exit_esp=int(gdb.parse_and_eval('$esp')),
                    exit_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                widen_sequence.append('release-return')
                assert call['exit_stage']==0 and call['exit_cursor']==call['instance']+1,'Release lifetime did not end exactly once'
                assert call['exit_esp']==call['entry_esp']+4,'native Release callback did not use RET0'
                return False
        class WidenReleaseEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('native-widen.c:destroy',internal=True)
            def stop(self):
                frame=gdb.newest_frame();owner=frame.older()
                while owner is not None and owner.name()!='widen_subsection':owner=owner.older()
                assert owner is not None
                obj=int(frame.read_var('object'));instance=len(widen_release_calls)
                assert instance==len(widen_factory_calls)-1 and obj==int(owner.read_var('object'))==widen_factory_calls[instance]['object'],'Release object/instance identity differs'
                # Break at the first source line after the compiler prologue.
                # The actual DWARF frame exposes the caller CFA; saved return
                # address at EBP+4 and cdecl-frame ESP at EBP+4 are independent
                # of compiler local stack allocation.
                entry_esp=int(gdb.parse_and_eval('$ebp'))+4
                inferior=gdb.selected_inferior();copied=int(owner.read_var('buffer').dereference())+4
                size=int(owner.read_var('output')['bytes'])
                call=dict(instance=instance,object=obj,entry_esp=entry_esp,entry_ecx=int(gdb.parse_and_eval('$ecx')),
                    caller='widen_subsection',copied_address=copied,copied=bytes(inferior.read_memory(copied,size)).hex(),
                    copied_size=size,entry_stage=int(gdb.parse_and_eval("'native-widen.c'::stage")),
                    entry_cursor=int(gdb.parse_and_eval("'native-widen.c'::cursor")),
                    entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                assert call['entry_stage']==3 and call['entry_cursor']==instance,'Release entered without completed emit'
                widen_sequence.append('release-call');widen_release_calls.append(call);WidenReleaseReturned(call);return False
        WidenReleaseEntered()

    if config.get('observe_global_type_lookup'):
        class LookupReturned(gdb.FinishBreakpoint):
            def __init__(self,record,output):
                self.record,self.output=record,output
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                assert lookup_stack and lookup_stack[-1] is self.record,'lookup return nesting differs'
                lookup_stack.pop()
                self.record.update(result=int(self.return_value)&0xffffffff,
                    output=int.from_bytes(bytes(gdb.selected_inferior().read_memory(self.output,4)),'little'))
                return False
        class LookupEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('lookup_global_type',internal=True)
            def stop(self):
                frame=gdb.newest_frame();output=frame.read_var('from_global')
                record=dict(index=int(frame.read_var('index')),depth=len(lookup_stack),
                    parent_argument_alias=bool(lookup_stack and int(output)==lookup_stack[-1]['argument_address']),
                    argument_address=int(output.address))
                lookup_calls.append(record);lookup_stack.append(record)
                LookupReturned(record,int(output));return False
        LookupEntered()

    if config.get('observe_recursive_type_compare'):
        class TypeCompareReturned(gdb.FinishBreakpoint):
            def __init__(self,record):
                self.record=record
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                self.record['different']=bool(int(gdb.newest_frame().read_register('eax')))
                return False
        class TypeCompareEntered(gdb.Breakpoint):
            def __init__(self):super().__init__("*'memcmp@plt'",internal=True)
            def stop(self):
                frame=gdb.newest_frame();owner=frame.older()
                if owner is None or owner.name()!='compare_recursive_type':return False
                inferior=gdb.selected_inferior();stack=int(gdb.parse_and_eval('$esp'))
                args=[int.from_bytes(bytes(inferior.read_memory(stack+4+i*4,4)),'little') for i in range(3)]
                cursor=int(owner.read_var('cursor'));offset=int(owner.read_var('offset'))
                assert args[0]==int(owner.read_var('data'))+cursor and args[1]==int(owner.read_var('other_data'))+cursor,'comparison actual operand aliases differ'
                assert args[2]==((offset-cursor)&0xffff) and 0<args[2]<=65536,'comparison actual length differs'
                root=int(gdb.parse_and_eval('type_records'));record=int(owner.read_var('record'));count=int(gdb.parse_and_eval('type_record_count'))
                assert root<=record<root+count*32 and (record-root)%32==0,'comparison record ownership differs'
                observed=dict(record_index=(record-root)//32,offset=cursor,size=args[2],
                    left=bytes(inferior.read_memory(args[0],args[2])).hex(),right=bytes(inferior.read_memory(args[1],args[2])).hex())
                type_compare_calls.append(observed);TypeCompareReturned(observed);return False
        TypeCompareEntered()


def capture(configuration,data_file,output):
    config=json.loads(open(configuration).read());data=json.loads(open(data_file).read())
    frame=gdb.newest_frame();actual_inferior=gdb.selected_inferior()
    def released(address):
        return any(row.get('returned') and row['address'] and row['address']<=address<row['address']+row['size'] for row in main_free_calls)
    class CurrentMemory:
        pid=actual_inferior.pid
        def read_memory(self,address,size):
            assert not any(row.get('returned') and row['address'] and max(int(address),row['address'])<min(int(address)+int(size),row['address']+row['size']) for row in main_free_calls), 'current-state observer tried to dereference released payload'
            return actual_inferior.read_memory(address,size)
    inferior=CurrentMemory()
    state=dict(function=frame.name(),pc=int(gdb.parse_and_eval('$pc')),signal=gdb.execute('info program',to_string=True))
    state['frontier_caller']=frame.older().name() if frame.older() is not None else None
    state['global_addresses']={item['name']:int(gdb.parse_and_eval('&('+item.get('native_expression',item['name'])+')')) for item in data}
    record=next((row for row in config['functions'] if row['native_symbol']==frame.name()),None)
    parameters={}
    if record:
        block=frame.block()
        while block is not None and block.function is None:block=block.superblock
        assert block is not None, 'frontier function has no actual debug block'
        formals={symbol.name:symbol.type.strip_typedefs() for symbol in block if symbol.is_argument}
        arguments=list(formals)
        declared=[item['name'] for item in record.get('parameters',[])]
        assert len(arguments)==len(declared) and set(arguments)==set(declared), ('incomplete frontier ABI contract',arguments,declared)
        inventory=[]
        for item in record.get('parameters',[]):
            actual=formals[item['name']];integer=item['kind']=='integer'
            if integer:
                assert actual.code in (gdb.TYPE_CODE_INT,gdb.TYPE_CODE_BOOL,gdb.TYPE_CODE_ENUM,gdb.TYPE_CODE_CHAR), 'integer ABI formal is not an actual integer'
            else:
                assert actual.code==gdb.TYPE_CODE_PTR, 'pointer ABI formal is not an actual pointer'
            width=item.get('native_width',4) if integer else 4
            assert actual.sizeof==width, ('actual frontier formal width differs',item['name'],str(actual),actual.sizeof,width)
            observation=dict(name=item['name'],kind='integer' if integer else 'pointer',width=int(actual.sizeof),native_type=str(actual))
            if not integer:
                pointee=actual.target().strip_typedefs()
                if pointee.code not in (gdb.TYPE_CODE_VOID,gdb.TYPE_CODE_FUNC):observation['pointee_width']=int(pointee.sizeof)
                required=item.get('width') if item['kind']=='caller-local-pointer' else item.get('element_size') if item['kind']=='blob-element-pointer' else 4 if item['kind']=='integer-pointer' else item.get('native_pointee_width')
                if required is not None:assert observation.get('pointee_width')==required, 'actual pointer formal pointee extent differs'
                if item['kind']=='function-pointer':assert pointee.code==gdb.TYPE_CODE_FUNC, 'callback formal is not an actual function pointer'
            inventory.append(observation)
        state['observed_argument_names']=arguments
        state['observed_argument_types']=inventory
    for item in (record or {}).get('parameters',[]):
        value=frame.read_var(item['name'])
        if item['kind']=='string':parameters[item['name']]=dict(null=int(value)==0,bytes=(value.string(encoding='latin1').encode('latin1')+b'\0').hex() if int(value) else None)
        elif item['kind']=='argv':parameters[item['name']]=[(value[i].string(encoding='latin1').encode('latin1')+b'\0').hex() for i in range(int(frame.read_var('argc')))]
        elif item['kind']=='integer-pointer':parameters[item['name']]=dict(null=int(value)==0,bytes=bytes(inferior.read_memory(int(value),4)).hex() if int(value) else None)
        elif item['kind']=='global-buffer':
            names=[name for name in item['allowed_globals'] if int(gdb.parse_and_eval(name))+item.get('offset',0)==int(value)]
            assert len(names)==1,'buffer argument does not alias specified global'
            if 'count_name' in item:size=int(frame.read_var(item['count_name']))
            else:
                entry=frame.read_var(item['entry_name']);size=int(entry.dereference()['size'])
            assert 0<=size<=65536
            parameters[item['name']]=dict(global_name=names[0],size=size,bytes=bytes(inferior.read_memory(int(value),size)).hex())
            if item.get('offset'):parameters[item['name']]['offset']=item['offset']
        elif item['kind']=='blob-element-pointer':
            root=int(gdb.parse_and_eval(item['root']));count=int(gdb.parse_and_eval(item['count']));address=int(value)
            assert root<=address<root+count*item['element_size'] and (address-root)%item['element_size']==0,'argument is not a directory element'
            parameters[item['name']]=dict(root=item['root'],index=(address-root)//item['element_size'],
                bytes=bytes(inferior.read_memory(address,item['element_size'])).hex())
        elif item['kind']=='type-record-pointer':
            root=int(gdb.parse_and_eval(item['root']));count=int(gdb.parse_and_eval(item['count']));address=int(value)
            assert root<=address<root+count*32 and (address-root)%32==0,'argument is not an owned local type record'
            parameters[item['name']]=dict(root=item['root'],index=(address-root)//32)
        elif item['kind']=='current-symbol-record':
            address=int(value);base=int(gdb.parse_and_eval(item['root']));capacity=int(gdb.parse_and_eval(item['capacity']));offset=address-base
            assert address==int(gdb.parse_and_eval(item['cursor'])) and 0<=offset<=capacity-4,'symbol record does not alias active owned input cursor'
            length=int.from_bytes(bytes(inferior.read_memory(address,2)),'little')+2
            assert 4<=length<=capacity-offset,'symbol record exceeds observed input capacity'
            parameters[item['name']]=dict(root=item['root'],offset=offset,bytes=bytes(inferior.read_memory(address,length)).hex())
        elif item['kind']=='type-record-name':
            address=int(value);base=int(gdb.parse_and_eval('type_segments[0]'))
            capacity=min(int(gdb.parse_and_eval('type_segment_bytes')),0xffe8)
            segment=bytes(inferior.read_memory(base,capacity));count=int(gdb.parse_and_eval('type_record_count'))
            assert 0<=count<=32768,'invalid type-name owner inventory'
            root=int(gdb.parse_and_eval('type_records'));records=bytes(inferior.read_memory(root,count*32));indices=[]
            for i in range(count):
                pointer=int.from_bytes(records[i*32:i*32+4],'little');offset=pointer-base
                if not 0<=offset<=capacity-4:continue
                kind=int.from_bytes(segment[offset+2:offset+4],'little')
                if kind not in (0x1004,0x1005,0x1006,0x1007):continue
                name_offset=16 if kind==0x1007 else 12 if kind==0x1006 else 20
                if kind!=0x1007:
                    if offset+name_offset+2>capacity:continue
                    if int.from_bytes(segment[offset+name_offset:offset+name_offset+2],'little')>=0x8000:continue
                    name_offset+=2
                if pointer+name_offset!=address:continue
                length=segment[address-base]+1;record_size=int.from_bytes(segment[offset:offset+2],'little')+2
                assert name_offset+length<=record_size and offset+record_size<=capacity,'type name exceeds its initialized owner record'
                indices.append(i)
            assert indices,'type name has no observed immediate-size local-record owner'
            length=segment[address-base]+1
            parameters[item['name']]=dict(segment=0,offset=address-base,record_indices=indices,bytes=segment[address-base:address-base+length].hex())
        elif item['kind']=='arena-type-record':
            address=int(value);arena=gdb.parse_and_eval(item['arena']);node=int(arena['first']);seen=set();owners=[]
            block_index=0;capacity=int(arena['capacity'])
            while node:
                assert node not in seen and len(seen)<1024,'arena argument owner chain is cyclic/unbounded'
                seen.add(node);header=bytes(inferior.read_memory(node,12))
                used=int.from_bytes(header[:4],'little');base=int.from_bytes(header[4:8],'little')
                assert 0<=used<=capacity<=65536,'arena argument block extent invalid'
                if base<=address<base+used:owners.append((block_index,base,used))
                node=int.from_bytes(header[8:12],'little');block_index+=1
            assert len(owners)==1,'arena argument has no initialized owned block'
            block_index,base,used=owners[0];offset=address-base
            assert offset+2<=used,'arena argument lacks initialized length'
            length=int.from_bytes(bytes(inferior.read_memory(address,2)),'little')+2
            assert 4<=length<=used-offset,'arena argument exceeds initialized record extent'
            index=int(frame.read_var(item['index_parameter']))
            counter=next(row for row in data if row['name']=='next_global_type')
            limit=int(gdb.parse_and_eval(counter.get('native_expression',counter['name'])))
            assert 0x1000<=index<limit,'arena argument index out of range'
            count=int(gdb.parse_and_eval('type_record_count'));root=int(gdb.parse_and_eval('type_records'))
            records=bytes(inferior.read_memory(root,count*32))
            indices=[i for i in range(count) if int.from_bytes(records[i*32:i*32+4],'little')==address and int.from_bytes(records[i*32+4:i*32+8],'little')==index]
            assert indices,'arena argument has no mapped local-record owner'
            assert int(gdb.parse_and_eval('type_table_used'))==512 and index-0x1000<512,'arena argument block inventory needs explicit contract'
            block=int(gdb.parse_and_eval('global_type_blocks[0]'));slot=bytes(inferior.read_memory(block+(index&511)*4,4))
            assert int.from_bytes(slot,'little')==address,'arena argument/global index slot alias differs'
            parameters[item['name']]=dict(arena=item['arena'],block_index=block_index,offset=offset,
                record_indices=indices,index=index,bytes=bytes(inferior.read_memory(address,length)).hex())
        elif item['kind']=='type-record-data':
            address=int(value);base=int(gdb.parse_and_eval('type_segments[0]'))
            capacity=min(int(gdb.parse_and_eval('type_segment_bytes')),0xffe8)
            count=int(gdb.parse_and_eval('type_record_count'));root=int(gdb.parse_and_eval('type_records'))
            records=bytes(inferior.read_memory(root,count*32))
            indices=[i for i in range(count) if int.from_bytes(records[i*32:i*32+4],'little')==address]
            assert indices and base<=address<=base+capacity-4,'type payload has no observed local-record/segment owner'
            length=int.from_bytes(bytes(inferior.read_memory(address,2)),'little')+2
            assert 4<=length<=base+capacity-address,'type payload exceeds segment capacity'
            parameters[item['name']]=dict(segment=0,offset=address-base,record_indices=indices,bytes=bytes(inferior.read_memory(address,length)).hex())
        elif item['kind']=='output-buffer':
            owner=frame.older()
            while owner is not None and owner.name()!=item['owner']:owner=owner.older()
            assert owner is not None,'output buffer owner frame missing'
            local=owner.read_var(item['local']);size=int(frame.read_var(item['count_name']))
            assert int(value)==int(local.address) and local.type.sizeof==item['capacity'] and 0<=size<=item['capacity'],'output buffer caller-local identity/extent differs'
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],capacity=item['capacity'],size=size)
        elif item['kind']=='local-record':
            owner=frame.older()
            while owner is not None and owner.name()!=item['owner']:owner=owner.older()
            assert owner is not None
            local=owner.read_var(item['local']);address=int(value)
            assert int(local.address)==address and local.type.sizeof==item['capacity'],'record buffer owner/identity differs'
            size=int.from_bytes(bytes(inferior.read_memory(address,2)),'little')+2
            assert 2<=size<=item['capacity']
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],capacity=item['capacity'],bytes=bytes(inferior.read_memory(address,size)).hex())
        elif item['kind']=='widen-object':
            owner=frame.older()
            while owner is not None and owner.name()!=item['owner']:owner=owner.older()
            assert owner is not None and int(owner.read_var(item['local']))==int(value),'opaque object caller-local alias differs'
            assert len(widen_factory_calls)==1 and widen_factory_calls[0]['object']==int(value),'opaque object was not returned by observed factory'
            assert int(gdb.parse_and_eval("'native-widen.c'::stage"))==item.get('native_stage',1) and int(gdb.parse_and_eval("'native-widen.c'::cursor"))==0,'opaque object provider lifetime/state differs'
            state.setdefault('widen_object_arguments',{})[item['name']]=dict(address=int(value),output_slot=int(owner.read_var(item['local']).address))
            parameters[item['name']]=dict(provider='MSPDB50.DLL',instance=1,state=item.get('state','created'),owner=item['owner'],local=item['local'])
        elif item['kind']=='caller-local-pointer':
            owner=frame.older()
            while owner is not None and owner.name()!=item['owner']:owner=owner.older()
            assert owner is not None
            local=owner.read_var(item['local']);address=int(value)
            assert int(local.address)==address and local.type.sizeof==item['width'],'output argument caller-local identity differs'
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],width=item['width'])
            if item.get('initialized'):
                raw=bytes(inferior.read_memory(address,item['width']))
                if item.get('widen_output'):
                    words=[int.from_bytes(raw[i:i+4],'little') for i in range(0,16,4)]
                    assert item['width']==16 and words[2]==int(gdb.parse_and_eval('pack_buffer')) and words[3]==int(gdb.parse_and_eval('pack_fixups')),'widen output actual allocation aliases differ'
                    assert 0<=words[0]<=int(gdb.parse_and_eval('pack_buffer_size')) and 0<=words[1]<=int(gdb.parse_and_eval('pack_fixup_count')),'widen output actual capacities exceed owned allocations'
                    parameters[item['name']]['description']=dict(bytes=words[0],fixups=words[1],output='pack_buffer',records='pack_fixups')
                else:parameters[item['name']]['bytes']=raw.hex()
        elif item['kind']=='function-pointer':
            assert int(value)==int(gdb.parse_and_eval('&'+item['function'])),'function-pointer argument differs'
            parameters[item['name']]=dict(function=item['function'])
        elif item['kind']=='object-pointer':parameters[item['name']]=dict(null=int(value)==0,address=int(value))
        elif item['kind']=='allocation-output':
            owned=[row for row in image_allocations if row.get('owner')==item['owner']]
            assert len(owned)==1,'symbol output allocation lifetime differs'
            allocation=owned[0];offset=int(value)-allocation['address']
            cursor=int(gdb.parse_and_eval(item['cursor']))
            if item.get('record_argument'):
                expected_offset=int(frame.read_var(item['offset_name']))
                assert offset==expected_offset and 0<=offset<=cursor-allocation['address']-4,'reference record offset/owner differs'
                length=int.from_bytes(bytes(inferior.read_memory(int(value),2)),'little')+2
                assert 4<=length<=cursor-int(value),'reference record exceeds initialized output'
                parameters[item['name']]=dict(owner=item['owner'],offset=offset,capacity=allocation['size'],bytes=bytes(inferior.read_memory(int(value),length)).hex())
            elif item.get('base_argument'):
                size=int(frame.read_var(item['size_name']))
                assert offset==0 and 0<=size<=allocation['size'] and int(value)+size==cursor,'initialized symbol buffer ownership/extent differs'
                parameters[item['name']]=dict(owner=item['owner'],offset=0,capacity=allocation['size'],size=size,bytes=bytes(inferior.read_memory(int(value),size)).hex())
            else:
                assert int(value)==cursor and 0<=offset<=allocation['size']-item['write_size'],'symbol output argument ownership/extent differs'
                parameters[item['name']]=dict(owner=item['owner'],offset=offset,capacity=allocation['size'],write_size=item['write_size'])
        elif item['kind']=='global-pointer':
            address=int(value)
            if not address:
                assert item.get('nullable');parameters[item['name']]=dict(null=True)
            else:
                names=[name for name in item['allowed_globals'] if (int(gdb.parse_and_eval(name)) if item.get('global_value') else state['global_addresses'][name])==address]
                assert len(names)==1,'argument is not the specified global address'
                parameters[item['name']]=dict(null=False,global_name=names[0])
                if 'buffer_count_slot' in item:
                    size=int(frame.read_var(item['buffer_count_name']));capacity=int(gdb.parse_and_eval(item['capacity_global']))
                    assert 0<=size<=capacity and size<=65536
                    pointer=int(value.dereference())
                    parameters[item['name']].update(size=size,capacity=capacity,bytes=bytes(inferior.read_memory(pointer,size)).hex())
        else:parameters[item['name']]=int(value)&0xffffffff
    state['parameters']=parameters
    caller_state={}
    for item in (record or {}).get('caller_state',[]):
        owner=frame.older()
        while owner is not None and owner.name()!=item['owner']:owner=owner.older()
        assert owner is not None,'required caller frame is missing'
        value=owner.read_var(item['local']);actual=value.type.strip_typedefs()
        assert actual.code==gdb.TYPE_CODE_INT and actual.sizeof==item['width'] and str(actual)==item['native_type'],'actual caller-local type/extent differs'
        caller_state[item['name']]=dict(owner=owner.name(),local=item['local'],width=int(actual.sizeof),native_type=str(actual),value=int(value))
    state['caller_state']=caller_state
    state['file_calls']=file_calls
    state['mapping_calls']=mapping_calls
    state['file_reads']=file_reads
    state['directory_allocation']=directory_allocation
    state['main_free_calls']=main_free_calls
    state['widen_sequence']=widen_sequence
    state['widen_release_calls']=widen_release_calls
    state['widen_emit_calls']=widen_emit_calls
    state['widen_query_calls']=widen_query_calls
    state['widen_factory_calls']=widen_factory_calls
    state['subsection_allocations']=subsection_allocations
    state['image_allocations']=image_allocations
    state['checked_allocations']=checked_allocations
    state['cleared_allocations']=cleared_allocations
    state['lookup_type_calls']=lookup_calls
    state['lookup_type_active_depth']=len(lookup_stack)
    state['type_compare_calls']=type_compare_calls
    state['module_allocations']=module_allocations
    state['globals']={item['name']:bytes(inferior.read_memory(gdb.parse_and_eval('&('+item.get('native_expression',item['name'])+')'),item['size'])).hex() for item in data}
    if any(item.get('function_table') for item in data):
        state['function_addresses']={row['native_symbol']:int(gdb.parse_and_eval('&'+row['native_symbol'])) for row in config['functions']}
    memory={};strings={};handles={};mapped_views={};captured_shapes={}
    if widen_emit_calls:
        call=widen_emit_calls[-1]
        for name,address,count in [('pack_buffer',call['output_address'],call['output_capacity']),('pack_fixups',call['fixup_address'],call['fixup_capacity']*8)]:
            assert address==int(gdb.parse_and_eval(name)),'live emitted scratch allocation identity differs'
            if released(address):continue
            memory[str(address)]=bytes(inferior.read_memory(address,count)).hex()
    state["last_error"]=int(gdb.parse_and_eval("'win32.c'::last_error"))
    def linked(head,shape):
        visited=set()
        while head:
            signature={key:shape[key] for key in ('size','link','string','handle','view','payload') if key in shape}
            if head in captured_shapes:
                assert captured_shapes[head]==signature,'same live object observed through incompatible layouts'
                return
            captured_shapes[head]=signature
            if head in visited:return
            assert len(visited)<128,'unbounded target object graph'
            visited.add(head)
            raw=bytes(inferior.read_memory(head,shape['size']));memory[str(head)]=raw.hex()
            if 'string' in shape:
                offset=shape['string'];pointer=int.from_bytes(raw[offset:offset+4],'little')
                if pointer:
                    text=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1').encode('latin1')
                    assert len(text)<1024
                    strings[str(pointer)]=(text+b'\0').hex()
            if shape['size']==48 and shape['link']==44 and shape.get('payload')==dict(offset=12,length_offset=16):
                address=int.from_bytes(raw[28:32],'little');length=int.from_bytes(raw[32:36],'little')
                if address:
                    assert 0<=length<=65536,'source-line extent exceeds bounded observation'
                    memory[str(address)]=bytes(inferior.read_memory(address,length)).hex()
            if 'handle' in shape:
                offset=shape['handle'];handle=int.from_bytes(raw[offset:offset+4],'little')
                if handle:
                    assert 0<handle<65536,'not the shared fd+1 file HANDLE representation'
                    proc=Path('/proc')/str(inferior.pid)
                    descriptor=proc/'fd'/str(handle-1)
                    info=(proc/'fdinfo'/str(handle-1)).read_text()
                    position=int(next(line.split()[1] for line in info.splitlines() if line.startswith('pos:')))
                    flags=int(next(line.split()[1] for line in info.splitlines() if line.startswith('flags:')),8)
                    actual=os.stat(descriptor)
                    text=int.from_bytes(raw[shape['string']:shape['string']+4],'little')
                    pathname=bytes.fromhex(strings[str(text)]).rstrip(b'\0').decode('latin1')
                    expected=Path(os.readlink(proc/'cwd'))/pathname
                    identified=os.stat(expected)
                    assert (actual.st_dev,actual.st_ino)==(identified.st_dev,identified.st_ino),'file HANDLE owns another pathname'
                    content=descriptor.read_bytes()
                    assert len(content)==actual.st_size
                    handles[str(handle)]=dict(type=1 if stat.S_ISREG(actual.st_mode) else -1,size=actual.st_size,
                        position=position,access_mode=flags&3,pathname=pathname,sha256=hashlib.sha256(content).hexdigest())
            if 'view' in shape:
                spec=shape['view'];address=int.from_bytes(raw[spec['offset']:spec['offset']+4],'little')
                length=int.from_bytes(raw[spec['length_offset']:spec['length_offset']+4],'little')
                if address and str(address) not in mapped_views:
                    assert 0<length<=16*1024*1024
                    node=gdb.parse_and_eval("'win32.c'::views");found=[]
                    while int(node):
                        value=node.dereference()
                        if int(value['address'])==address:found.append(int(value['length']))
                        node=value['next']
                    assert found==[length],'mapped view absent from actual typed view registry'
                    raw_view=bytes(inferior.read_memory(address,length))
                    view_name='native-view-%d.bin'%(len(mapped_views)+1)
                    (Path(output).parent/view_name).write_bytes(raw_view)
                    proc=Path('/proc')/str(inferior.pid)
                    regions=[]
                    for line in (proc/'maps').read_text().splitlines():
                        fields=line.split();first,last=[int(value,16) for value in fields[0].split('-')]
                        if first<=address and address+length<=last:
                            regions.append((first,fields))
                    assert len(regions)==1,'mapped view does not have one complete host region'
                    first,fields=regions[0]
                    pathname=bytes.fromhex(strings[str(int.from_bytes(raw[0:4],'little'))]).rstrip(b'\0').decode('latin1')
                    file=Path(os.readlink(proc/'cwd'))/pathname;actual=os.stat(file)
                    major,minor=[int(value,16) for value in fields[3].split(':')]
                    assert actual.st_ino==int(fields[4]) and os.major(actual.st_dev)==major and os.minor(actual.st_dev)==minor,'view backing file identity differs'
                    matches=[(i,row) for i,row in enumerate(mapping_calls) if row['kind']=='map' and row['result']==address]
                    assert len(matches)==1,'view generation needs an explicit unmap/reuse model'
                    ordinal,call=matches[0]
                    mapped_views[str(address)]=dict(length=length,permissions=fields[1],
                        offset=int(fields[2],16)+address-first,pathname=pathname,call_index=ordinal,
                        file=view_name,sha256=hashlib.sha256(raw_view).hexdigest())
            if 'payload' in shape:
                spec=shape['payload'];address=int.from_bytes(raw[spec['offset']:spec['offset']+4],'little');size=int.from_bytes(raw[spec['length_offset']:spec['length_offset']+4],'little')
                if address:
                    assert 0<size<=65536
                    memory[str(address)]=bytes(inferior.read_memory(address,size)).hex()
            head=int.from_bytes(raw[shape['link']:shape['link']+4],'little')
    for item in data:
        if 'shape' not in item:continue
        shape=item['shape'];head=state['global_addresses'][item['name']] if shape.get('inline') else int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
        if not head:continue
        if 'count' not in shape and 'count_literal' not in shape:linked(head,shape)
        else:
            count=shape['count_literal'] if 'count_literal' in shape else int(gdb.parse_and_eval(shape['count']));assert 0<=count<=128
            raw=bytes(inferior.read_memory(head,count*4));memory[str(head)]=raw.hex()
            for i in range(count):linked(int.from_bytes(raw[i*4:i*4+4],'little'),shape)
    for item in data:
        if 'blob' not in item:continue
        head=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
        if not head or released(head):continue
        spec=item['blob'];size=spec.get('size')
        if size is None:size=int(gdb.parse_and_eval(spec['count']))*spec['element_size']
        assert 0<=size<=65536
        raw=bytes(inferior.read_memory(head,size));memory[str(head)]=raw.hex()
        for pointer in spec.get('pointers',[]):
            for offset in range(pointer['offset'],size,pointer.get('stride',size+1)):
                child=int.from_bytes(raw[offset:offset+4],'little')
                if not child or 'allocation' in pointer:continue
                if 'shape' in pointer:linked(child,pointer['shape'])
                else:memory[str(child)]=bytes(inferior.read_memory(child,pointer['size'])).hex()
    for item in data:
        if not item.get('public_buckets'):continue
        address=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
        if not address:continue
        raw=bytes(inferior.read_memory(address,4096*4));memory[str(address)]=raw.hex();visited=set()
        for offset in range(0,len(raw),4):
            node=int.from_bytes(raw[offset:offset+4],'little')
            while node:
                assert node not in visited and len(visited)<512,'aliased/circular public-symbol nodes'
                visited.add(node);header=bytes(inferior.read_memory(node,18));length=int.from_bytes(header[16:18],'little')
                assert length<=1024
                memory[str(node)]=bytes(inferior.read_memory(node,16+length+2)).hex()
                node=int.from_bytes(header[:4],'little')
    for item in data:
        if item.get('type_arena'):
            words=bytes.fromhex(state['globals'][item['name']])
            capacity=int.from_bytes(words[:4],'little');head=int.from_bytes(words[20:24],'little');seen=set()
            while head:
                assert head not in seen and len(seen)<1024,'arena chain is cyclic/unbounded'
                seen.add(head);block=bytes(inferior.read_memory(head,12));memory[str(head)]=block.hex()
                payload=int.from_bytes(block[4:8],'little')
                assert 0<capacity<=65536 and payload,'invalid arena payload capacity'
                memory[str(payload)]=bytes(inferior.read_memory(payload,capacity)).hex()
                head=int.from_bytes(block[8:12],'little')
        if item.get('global_type_blocks'):
            slots=bytes.fromhex(state['globals'][item['name']]);capacity=int(gdb.parse_and_eval('type_table_used'))
            assert 0<=capacity<=512 and capacity%512==0,'global block inventory needs an explicit contract'
            if capacity:
                block=int.from_bytes(slots[:4],'little');memory[str(block)]=bytes(inferior.read_memory(block,2048)).hex()
        if item.get('recursive_types'):
            root=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
            if root:
                heads=bytes(inferior.read_memory(root,16384));memory[str(root)]=heads.hex();seen=set()
                for offset in range(0,16384,4):
                    head=int.from_bytes(heads[offset:offset+4],'little')
                    while head:
                        assert head not in seen and len(seen)<4096,'recursive-type chain is cyclic/unbounded'
                        seen.add(head);node=bytes(inferior.read_memory(head,12));memory[str(head)]=node.hex()
                        head=int.from_bytes(node[8:12],'little')
        if item.get('interned_types'):
            root=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
            if root:
                heads=bytes(inferior.read_memory(root,16384));memory[str(root)]=heads.hex();seen=set()
                for offset in range(0,16384,4):
                    head=int.from_bytes(heads[offset:offset+4],'little')
                    while head:
                        assert head not in seen and len(seen)<4096,'interned-type chain is cyclic/unbounded'
                        seen.add(head);node=bytes(inferior.read_memory(head,12));memory[str(head)]=node.hex()
                        head=int.from_bytes(node[8:12],'little')
    for item in data:
        if not item.get('type_name_cache'):continue
        root=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
        if not root:continue
        heads=bytes(inferior.read_memory(root,2048));memory[str(root)]=heads.hex();visited=set()
        for offset in range(0,2048,4):
            node=int.from_bytes(heads[offset:offset+4],'little')
            while node:
                assert node not in visited and len(visited)<512,'aliased/circular type-name cache nodes'
                visited.add(node);raw=bytes(inferior.read_memory(node,20));memory[str(node)]=raw.hex()
                name=int.from_bytes(raw[:4],'little')
                if name:
                    length=int.from_bytes(bytes(inferior.read_memory(name,1)),'little')+1
                    memory[str(name)]=bytes(inferior.read_memory(name,length)).hex()
                node=int.from_bytes(raw[16:20],'little')
    for item in data:
        if item.get('written_allocation'):
            owned=[row for row in image_allocations if row.get('owner')==item['written_allocation']['owner']]
            assert owned,'symbol output allocation lifetime differs'
            for allocation in owned:
                memory[str(allocation['address'])]=bytes(inferior.read_memory(allocation['address'],allocation['size'])).hex()
    for item in data:
        if item.get('type_record_table'):
            address=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
            if address:
                size=int(gdb.parse_and_eval('type_record_capacity'))*32
                assert 0<size<=1048576
                records=bytes(inferior.read_memory(address,size));memory[str(address)]=records.hex()
                count=int(gdb.parse_and_eval('type_record_count'))
                for index in range(count):
                    record=records[index*32:index*32+32]
                    if not record[29]&1:continue
                    for offset in (16,24):
                        child=int.from_bytes(record[offset:offset+4],'little')
                        owned=[row for row in cleared_allocations if row.get('address')==child]
                        assert owned and 0<owned[-1]['size']<=65536,'extended reference allocation not observed'
                        memory[str(child)]=bytes(inferior.read_memory(child,owned[-1]['size'])).hex()
        if item.get('segment_contents'):
            address=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
            if address and not released(address):
                size=min(int(gdb.parse_and_eval('type_segment_bytes')),0xffe8)
                memory[str(address)]=bytes(inferior.read_memory(address,size)).hex()
    state['memory']=memory
    state['strings']=strings
    state['file_handles']=handles
    state['mapped_views']=mapped_views
    open(output,'w').write(json.dumps(state,indent=2)+'\n')
