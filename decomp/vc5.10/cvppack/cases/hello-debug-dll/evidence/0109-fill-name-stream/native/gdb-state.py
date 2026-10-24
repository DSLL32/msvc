"""GDB-side typed state capture, copied into each raw native evidence directory."""
import json
import struct
import re
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
source_index_allocations=[]
source_family_allocations=[]
source_family_frees=[]
output_buffer_allocations=[]
subsection_calls=[]
public_padding={}
name_hash_allocations=[]
name_hash_entry={}
name_hash_reused_calls={}
name_hash_first_frame={}
name_hash_stream_calls=[]
name_hash_temp_frees=[]
output_directory_before={}
output_directory_sort={}
output_directory_complete={}
sort_calls=[]
sort_environments={}
lookup_calls=[]
lookup_stack=[]
type_compare_calls=[]
live_mappings={}
def name_hash_frame_snapshot(owner,actual_inferior):
    scalars={};scalar_addresses={}
    for name,width in [('hash_count',2),('page_count',2),('position',4),('overflow_count',4)]:
        value=owner.read_var(name);kind=value.type.strip_typedefs();assert kind.code==gdb.TYPE_CODE_INT and kind.sizeof==width,'namehash actual scalar extent differs';scalars[name]=int(value);scalar_addresses[name]=int(value.address)
    block=owner.block()
    while block is not None and block.function is None:block=block.superblock
    formal_types={symbol.name:symbol.type.strip_typedefs() for symbol in block if symbol.is_argument}
    assert set(formal_types)=={'key','method','bytes','stream','count','buckets'},'namehash actual six formal inventory differs'
    for name in formal_types:
        kind=formal_types[name];assert kind.sizeof==4 and kind.code==(gdb.TYPE_CODE_INT if name=='count' else gdb.TYPE_CODE_PTR),'namehash actual formal kind/width differs'
    assert formal_types['key'].target().strip_typedefs().code==gdb.TYPE_CODE_FUNC and {name:int(formal_types[name].target().sizeof) for name in ('method','bytes','stream','buckets')}=={'method':2,'bytes':4,'stream':24,'buckets':4},'namehash actual formal pointed extent differs'
    arguments={name:int(owner.read_var(name))&0xffffffff for name in formal_types}
    pages=owner.read_var('pages');kind=pages.type.strip_typedefs();assert kind.code==gdb.TYPE_CODE_PTR and kind.sizeof==4 and kind.target().strip_typedefs().code==gdb.TYPE_CODE_PTR and kind.target().target().sizeof==104,'namehash actual bucket pointer type differs'
    overflow=owner.read_var('overflow_pages');kind=overflow.type.strip_typedefs();assert kind.code==gdb.TYPE_CODE_ARRAY and kind.sizeof==40 and kind.target().sizeof==4 and kind.target().target().sizeof==44,'namehash actual overflow pointer array differs'
    def layout(typename):
        kind=gdb.lookup_type(typename).strip_typedefs();return dict(width=int(kind.sizeof),fields={f.name:dict(offset=int(f.bitpos)//8,width=int(f.type.sizeof)) for f in kind.fields()})
    offset=owner.read_var('offset');assert offset.type.strip_typedefs().code==gdb.TYPE_CODE_INT and offset.type.sizeof==4,'namehash actual offset local width differs'
    return dict(offset_address=int(offset.address),offset_type=str(offset.type.strip_typedefs()),owner=owner.name(),caller=owner.older().name(),arguments=arguments,formal_types={name:str(kind) for name,kind in formal_types.items()},scalars=scalars,scalar_addresses=scalar_addresses,arena_bytes=bytes(actual_inferior.read_memory(owner.read_var('stream'),24)).hex(),pages=int(pages),overflow_pages=bytes(actual_inferior.read_memory(overflow.address,40)).hex(),layouts={name:layout(name) for name in ('NameHashBucket','NameHashOverflow','NameHashPair')},initialized_allocations={str(row['address']):bytes(actual_inferior.read_memory(row['address'],2 if index==4 else row['size'])).hex() for index,row in enumerate(name_hash_allocations)})

def install(configuration):
    global sort_environment
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
            if self.record.get('owner')=='begin_buffered_output':self.record.update(exit_esp=int(gdb.parse_and_eval('$esp')),after=sort_environment())
            return False
    class AllocationEntered(gdb.Breakpoint):
        def __init__(self):super().__init__('*malloc',internal=True)
        def stop(self):
            frame=gdb.newest_frame();owner=frame.older()
            if owner is None or owner.name() not in ('read_codeview_directory','load_debug_subsection','read_image_debug','allocate_bytes','initialize_single_module','allocate_cleared','rewrite_module_symbols','rewrite_module_global_types','begin_buffered_output'):return False
            stack=int(gdb.parse_and_eval('$esp'))
            size=int.from_bytes(bytes(gdb.selected_inferior().read_memory(stack+4,4)),'little')
            if owner.name()=='begin_buffered_output':
                assert config.get('observe_output_buffer_init') and len(main_free_calls)==12 and len(source_family_frees)==2 and all(row.get('returned') for row in main_free_calls+source_family_frees),'buffer allocation before prior lifetimes ended'
                block=owner.block()
                while block is not None and block.function is None:block=block.superblock
                formals={symbol.name:symbol.type.strip_typedefs() for symbol in block if symbol.is_argument}
                assert set(formals)=={'file'} and formals['file'].code==gdb.TYPE_CODE_INT and formals['file'].sizeof==4,'buffer initializer actual DWARF ABI differs'
                parent=owner.older();assert parent is not None and parent.name()=='flush_packed_file','buffer allocation parent owner differs'
                record=dict(sequence=len(output_buffer_allocations),owner=owner.name(),file=int(owner.read_var('file')),size=size,entry_esp=stack,before=sort_environment(),formal_inventory={'file':str(formals['file'])})
                output_buffer_allocations.append(record)
            elif owner.name()=='read_codeview_directory':
                assert not directory_allocation,'directory allocation generation needs an explicit model'
                directory_allocation['size']=size;record=directory_allocation
            elif owner.name()=='allocate_cleared':
                record=dict(size=size);cleared_allocations.append(record)
                if config.get('observe_name_hash') and owner.older() is not None and (owner.older().name()=='build_symbol_name_hash' or owner.older().name()=='copy_type_arena' and owner.older().older() is not None and owner.older().older().name()=='build_symbol_name_hash'):record.update(owner=owner.older().name(),context='build_symbol_name_hash')
            elif owner.name()=='allocate_bytes':
                record=dict(size=size);checked_allocations.append(record)
                caller=owner.older()
                if config.get('observe_name_hash') and caller is not None and (caller.name()=='build_symbol_name_hash' or caller.name()=='copy_type_arena' and caller.older() is not None and caller.older().name()=='build_symbol_name_hash'):record.update(owner=caller.name(),context='build_symbol_name_hash')
                if config.get('observe_source_index_complete') and caller is not None and caller.name() in ('build_source_file_index','append_source_name'):
                    assert len(main_free_calls)==12 and all(row.get('returned') for row in main_free_calls) and not source_family_frees,'source family allocation lifetime order differs'
                    role=['module','name','output'][len(source_family_allocations)]
                    assert caller.name()==('append_source_name' if role=='name' else 'build_source_file_index'),'source family allocation owner differs'
                    record.update(owner=caller.name(),role=role,after_frees=12)
                    if role=='name':
                        parent=caller.older();assert parent is not None and parent.name()=='build_source_file_index'
                        block=caller.block()
                        while block is not None and block.function is None:block=block.superblock
                        formals={symbol.name:symbol.type.strip_typedefs() for symbol in block if symbol.is_argument}
                        assert set(formals)=={'name','tail','offset'} and formals['name'].target().sizeof==1 and formals['tail'].target().sizeof==16 and formals['offset'].sizeof==4,'source append actual DWARF ABI differs'
                        tail=int(caller.read_var('tail'));assert tail==int(parent.read_var('names').address),'source append native actual sentinel owner differs'
                        record['arguments']=dict(name=int(caller.read_var('name')),tail=tail,offset=int(caller.read_var('offset')),tail_before=bytes(gdb.selected_inferior().read_memory(tail,16)).hex())
                    if role=='output':
                        record['after_append']=dict(name_tail=int(caller.read_var('name_tail')),sentinel_next=int(caller.read_var('names')['next']),module_head=int(caller.read_var('modules')['next']))
                    source_family_allocations.append(record)
                if config.get('observe_source_index_entry') and caller is not None and caller.name()=='build_source_file_index':
                    assert len(main_free_calls)==12 and all(row.get('returned') for row in main_free_calls),'source-index allocation before prior free returns'
                    record.update(owner='build_source_file_index',after_frees=12)
                    source_index_allocations.append(record)
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

    def sort_environment():
        selected={};names={'st'+str(i) for i in range(8)}|{'fctrl','fstat','ftag','fiseg','fioff','foseg','fooff','fop'}
        for line in gdb.execute('maintenance print raw-registers',to_string=True).splitlines():
            fields=line.split()
            if fields and fields[0] in names:
                assert len(fields)==7 and fields[-1].startswith('0x'),'unavailable native raw x87 register'
                selected[fields[0]]=fields[-1]
        assert set(selected)==names,'incomplete native raw x87 state'
        return dict(errno=int(gdb.parse_and_eval('(int)errno')),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")),
            errno_address=int(gdb.parse_and_eval('&errno')),doserrno_address=int(gdb.parse_and_eval('&recon_doserrno')),last_error_address=int(gdb.parse_and_eval("&'win32.c'::last_error")),fp=selected,nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ['ebx','esi','edi','ebp']])

    if config.get('observe_complete_directory_sort'):
        class CompareReturned(gdb.FinishBreakpoint):
            def __init__(self,record):
                self.record=record
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                self.record.update(result=int(self.return_value),exit_esp=int(gdb.parse_and_eval('$esp')),after=sort_environment())
                return False
        class CompareEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('*compare_packed_directory_entries',internal=True)
            def stop(self):
                frame=gdb.newest_frame();parent=frame.older()
                assert parent is not None and parent.name()=='recon_qsort' and len(sort_calls)<4096,'unmodeled comparator callback owner/count'
                block=frame.block()
                while block is not None and block.function is None:block=block.superblock
                formals={symbol.name:symbol.type.strip_typedefs() for symbol in block if symbol.is_argument}
                assert set(formals)=={'first','second'} and all(value.code==gdb.TYPE_CODE_PTR and value.sizeof==4 and value.target().strip_typedefs().code==gdb.TYPE_CODE_VOID for value in formals.values()),'sort actual DWARF comparator ABI differs'
                stack=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(stack,12)));root=int(gdb.parse_and_eval('directory_entries'));count=int(gdb.parse_and_eval('directory_count'))
                assert all(root<=value<root+count*12 and (value-root)%12==0 for value in args[1:]),'sort callback argument outside initialized active array'
                record=dict(sequence=len(sort_calls),entry_esp=stack,return_pc=args[0],first=args[1],second=args[2],records=(bytes(gdb.selected_inferior().read_memory(args[1],12))+bytes(gdb.selected_inferior().read_memory(args[2],12))).hex(),before=sort_environment(),formal_inventory={name:str(value) for name,value in formals.items()})
                sort_calls.append(record);CompareReturned(record);return False
        CompareEntered()
        class DirectorySortReturned(gdb.FinishBreakpoint):
            def stop(self):
                root=int(gdb.parse_and_eval('directory_entries'));output_directory_complete.update(address=root,count=int(gdb.parse_and_eval('directory_count')),capacity=directory_allocation['size'],bytes=bytes(gdb.selected_inferior().read_memory(root,output_directory_before['count']*12)).hex(),exit_esp=int(gdb.parse_and_eval('$esp')))
                sort_environments['after']=sort_environment()
                return False

    if config.get('observe_output_directory'):
        class DirectoryBefore(gdb.Breakpoint):
            def __init__(self):super().__init__('*flush_packed_file',internal=True)
            def stop(self):
                assert not output_directory_before,'directory writer input observed twice'
                address=int(gdb.parse_and_eval('directory_entries'));count=int(gdb.parse_and_eval('directory_count'));capacity=directory_allocation['size']
                assert directory_allocation['address']==address and capacity==(count+6)*12,'directory actual prewriter owner differs'
                output_directory_before.update(address=address,count=count,capacity=capacity,bytes=bytes(gdb.selected_inferior().read_memory(address,count*12)).hex())
                return False
        DirectoryBefore()
        class DirectorySort(gdb.Breakpoint):
            def __init__(self):super().__init__('*recon_qsort',internal=True)
            def stop(self):
                owner=gdb.newest_frame().older()
                if owner is None or owner.name()!='flush_packed_file':return False
                assert not output_directory_sort,'directory sort observed twice'
                stack=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(stack+4,16)))
                assert args[0]==int(gdb.parse_and_eval('directory_entries')) and args[1]==int(gdb.parse_and_eval('directory_count')) and args[2]==12 and args[3]==int(gdb.parse_and_eval('&compare_packed_directory_entries')),'directory native qsort complete arguments differ'
                output_directory_sort.update(address=args[0],count=args[1],capacity=directory_allocation['size'],width=args[2],callback=args[3],bytes=bytes(gdb.selected_inferior().read_memory(args[0],output_directory_before['count']*12)).hex())
                if config.get('observe_complete_directory_sort'):
                    output_directory_sort['entry_esp']=stack
                    sort_environments['before']=sort_environment();DirectorySortReturned(gdb.newest_frame(),internal=True)
                return False
        DirectorySort()

    if config.get('observe_name_hash'):
        class NameHashEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('*build_symbol_name_hash',internal=True)
            def stop(self):
                assert not name_hash_entry and gdb.newest_frame().older().name()=='write_publics','namehash actual entry owner/inventory differs'
                esp=int(gdb.parse_and_eval('$esp'));name_hash_entry.update(esp=esp,stack=list(struct.unpack('<7I',bytes(gdb.selected_inferior().read_memory(esp,28)))));return False
        NameHashEntered()
        class ReusedArenaReturned(gdb.FinishBreakpoint):
            def __init__(self,record):self.record=record;super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                r=self.record;r.update(exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')],after_arena=bytes(gdb.selected_inferior().read_memory(r['arena'],24)).hex())
                if r['name']=='copy_type_arena':r['result']=int(self.return_value)&0xffffffff
                return False
        class ReusedArenaEntered(gdb.Breakpoint):
            def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
            def stop(self):
                frame=gdb.newest_frame();caller=frame.older()
                if caller is None or caller.name()!='build_symbol_name_hash':return False
                if self.name in name_hash_reused_calls and config.get('observe_name_hash_complete'):return False
                assert self.name not in name_hash_reused_calls,'namehash contextual arena call repeated'
                block=frame.block()
                while block is not None and block.function is None:block=block.superblock
                formals={s.name:s.type.strip_typedefs() for s in block if s.is_argument};expected={'arena','capacity'} if self.name=='initialize_arena' else {'arena','data','size'}
                assert set(formals)==expected and formals['arena'].code==gdb.TYPE_CODE_PTR and formals['arena'].sizeof==4 and formals['arena'].target().sizeof==24,'namehash reused actual arena formal differs'
                esp=int(gdb.parse_and_eval('$esp'));stack=bytes(gdb.selected_inferior().read_memory(esp,12))
                arena=int.from_bytes(stack[4:8],'little') if self.name=='initialize_arena' else int(gdb.parse_and_eval('$ecx'))&0xffffffff
                record=dict(name=self.name,owner=caller.name(),entry_esp=esp,arena=arena,before_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')],formal_types={name:str(kind) for name,kind in formals.items()})
                if self.name=='initialize_arena':
                    assert formals['capacity'].code==gdb.TYPE_CODE_INT and formals['capacity'].sizeof==4;record['capacity']=int.from_bytes(stack[8:12],'little')
                else:
                    assert formals['data'].code==gdb.TYPE_CODE_PTR and formals['data'].target().sizeof==1 and formals['size'].code==gdb.TYPE_CODE_INT and formals['size'].sizeof==4
                    record.update(data=int(gdb.parse_and_eval('$edx'))&0xffffffff,size=int.from_bytes(stack[4:8],'little'),before_arena=bytes(gdb.selected_inferior().read_memory(record['arena'],24)).hex())
                    value=caller.read_var('hash_count');assert value.type.sizeof==2 and int(value.address)==record['data'] and record['size']==2,'namehash actual reused copy WORD input owner differs';record['input']=bytes(gdb.selected_inferior().read_memory(record['data'],2)).hex()
                name_hash_reused_calls[self.name]=record;ReusedArenaReturned(record);return False
        ReusedArenaEntered('initialize_arena');ReusedArenaEntered('copy_type_arena')
        if config.get('observe_name_hash_complete'):
            class StreamReturned(gdb.FinishBreakpoint):
                def __init__(self,record):self.record=record;super().__init__(gdb.newest_frame(),internal=True)
                def stop(self):
                    r=self.record;inferior=gdb.selected_inferior();raw=bytes(inferior.read_memory(r['arena'],24));words=struct.unpack('<6I',raw);used,address,nextblock=struct.unpack('<3I',bytes(inferior.read_memory(words[2],12)));assert words[1]==1 and used<=4096
                    r.update(exit_esp=int(gdb.parse_and_eval('$esp')),result=int(self.return_value)&0xffffffff,after_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')],after_arena=raw.hex(),output_address=address,output=bytes(inferior.read_memory(address,used)).hex());return False
            class StreamEntered(gdb.Breakpoint):
                def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
                def stop(self):
                    frame=gdb.newest_frame();caller=frame.older()
                    if caller is None or caller.name()!='build_symbol_name_hash':return False
                    inferior=gdb.selected_inferior();esp=int(gdb.parse_and_eval('$esp'));stack=bytes(inferior.read_memory(esp,16));copy=self.name=='copy_type_arena'
                    arena=(int(gdb.parse_and_eval('$ecx'))&0xffffffff) if copy else int.from_bytes(stack[4:8],'little');value=(int(gdb.parse_and_eval('$edx'))&0xffffffff) if copy else int.from_bytes(stack[8:12],'little');size=int.from_bytes(stack[4:8] if copy else stack[12:16],'little');assert 0<size<=96
                    scope=frame.block()
                    while scope is not None and scope.function is None:scope=scope.superblock
                    types={symbol.name:symbol.type.strip_typedefs() for symbol in scope if symbol.is_argument};expected={'arena','data','size'} if copy else {'arena','value','size'}
                    assert set(types)==expected and types['arena'].code==gdb.TYPE_CODE_PTR and types['arena'].target().sizeof==24 and types['size'].code==gdb.TYPE_CODE_INT and types['size'].sizeof==4
                    if copy:assert types['data'].code==gdb.TYPE_CODE_PTR and types['data'].target().sizeof==1
                    else:assert types['value'].code==gdb.TYPE_CODE_INT and types['value'].sizeof==4
                    if not copy and not name_hash_first_frame:
                        name_hash_first_frame.update(name_hash_frame_snapshot(caller,inferior))
                        parent=caller.older();header=parent.read_var('header');name_hash_first_frame['parent_header']=bytes(inferior.read_memory(header.address,16)).hex()
                    record=dict(kind=0 if copy else 1,owner=caller.name(),name=self.name,arena=arena,value=value,size=size,entry_esp=esp,before_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')],formal_types={name:str(kind) for name,kind in types.items()},before_arena=bytes(inferior.read_memory(arena,24)).hex())
                    if copy:record['input']=bytes(inferior.read_memory(value,size)).hex()
                    name_hash_stream_calls.append(record);StreamReturned(record);return False
            StreamEntered('copy_type_arena');StreamEntered('fill_type_arena')
            class HashFreeReturned(gdb.FinishBreakpoint):
                def __init__(self,record):self.record=record;super().__init__(gdb.newest_frame(),internal=True)
                def stop(self):
                    self.record.update(returned=True,exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')]);return False
            class HashFreeEntered(gdb.Breakpoint):
                def __init__(self):super().__init__('*recon_free',internal=True)
                def stop(self):
                    frame=gdb.newest_frame();caller=frame.older()
                    if caller is None or caller.name()!='build_symbol_name_hash':return False
                    esp=int(gdb.parse_and_eval('$esp'));address=int.from_bytes(bytes(gdb.selected_inferior().read_memory(esp+4,4)),'little');indices=[index for index,row in enumerate(name_hash_allocations[:3]) if row['address']==address];assert not address or len(indices)==1
                    index=indices[0] if address else 3;size=name_hash_allocations[index]['size'] if address else 0
                    record=dict(owner=caller.name(),entry_esp=esp,address=address,index=index,size=size,before=bytes(gdb.selected_inferior().read_memory(address,size)).hex() if size else '',before_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')]);name_hash_temp_frees.append(record);HashFreeReturned(record);return False
            HashFreeEntered()

        class HashAllocationReturned(gdb.FinishBreakpoint):
            def __init__(self,record):
                self.record=record;super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                self.record.update(address=int(self.return_value)&0xffffffff,exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')]);return False
        class HashAllocationEntered(gdb.Breakpoint):
            def __init__(self,name):self.name=name;super().__init__('*'+name,internal=True)
            def stop(self):
                frame=gdb.newest_frame();caller=frame.older()
                if caller is None or not(caller.name()=='build_symbol_name_hash' or caller.name()=='copy_type_arena' and caller.older() is not None and caller.older().name()=='build_symbol_name_hash'):return False
                block=frame.block()
                while block is not None and block.function is None:block=block.superblock
                formals={s.name:s.type.strip_typedefs() for s in block if s.is_argument}
                assert set(formals)=={'size'} and formals['size'].sizeof==4 and formals['size'].code==gdb.TYPE_CODE_INT,'namehash allocation actual size formal differs'
                esp=int(gdb.parse_and_eval('$esp'));size=int.from_bytes(bytes(gdb.selected_inferior().read_memory(esp+4,4)),'little')
                assert len(main_free_calls)==12 and len(source_family_frees)==2 and all(row.get('returned') for row in main_free_calls+source_family_frees),'namehash new allocation before cleanup completion'
                record=dict(sequence=len(name_hash_allocations),owner=caller.name(),context='build_symbol_name_hash',kind='checked' if self.name=='allocate_bytes' else 'cleared',size=size,after_frees=12,source_frees=2,entry_esp=esp,before_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')],formal_inventory={'size':str(formals['size'])})
                name_hash_allocations.append(record);HashAllocationReturned(record);return False
        HashAllocationEntered('allocate_bytes');HashAllocationEntered('allocate_cleared')
    if config.get('observe_public_padding'):
        class PaddingReturned(gdb.FinishBreakpoint):
            def __init__(self):super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                public_padding.update(result=int(self.return_value)&0xffffffff,exit_esp=int(gdb.parse_and_eval('$esp')),after_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')])
                return False
        class PaddingEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('*insert_symbol_page_padding',internal=True)
            def stop(self):
                assert not public_padding,'public padding repeated unexpectedly'
                inferior=gdb.selected_inferior();frame=gdb.newest_frame();esp=int(gdb.parse_and_eval('$esp'));args=bytes(inferior.read_memory(esp,12));ret,root,reserve=struct.unpack('<3I',args)
                assert root==int(gdb.parse_and_eval('type_table_global')) and reserve==4 and frame.older().name()=='write_publics','public padding actual entry arguments differ'
                table=bytes(inferior.read_memory(root,16384));nodes=[];seen=set()
                for bucket in range(4096):
                    node=int.from_bytes(table[bucket*4:bucket*4+4],'little')
                    while node:
                        assert node not in seen and len(seen)<512,'public padding input cycle';seen.add(node)
                        header=bytes(inferior.read_memory(node,18));length=int.from_bytes(header[16:18],'little');assert length<=1024
                        nodes.append(dict(bucket=bucket,address=node,bytes=bytes(inferior.read_memory(node,18+length)).hex()));node=int.from_bytes(header[:4],'little')
                public_padding.update(entry_esp=esp,return_pc=ret,buckets_address=root,reserve=reserve,remaining=int(gdb.parse_and_eval('public_arena_remaining')),cursor=int(gdb.parse_and_eval('public_arena_cursor')),before_nonvolatile=[int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')],buckets=table.hex(),nodes=nodes)
                PaddingReturned();return False
        PaddingEntered()
    if config.get('observe_subsection_output'):
        def preserved_registers():return [int(gdb.parse_and_eval('$'+name))&0xffffffff for name in ('ebx','esi','edi','ebp')]
        class SubsectionReturned(gdb.FinishBreakpoint):
            def __init__(self,record):
                self.record=record;super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                index=self.record['index'];root=int(gdb.parse_and_eval('directory_entries'))
                self.record.update(exit_esp=int(gdb.parse_and_eval('$esp')),after_used=int(gdb.parse_and_eval('output_used')),padding=int(gdb.parse_and_eval('output_padding_size')),after_row=bytes(gdb.selected_inferior().read_memory(root+12*index,12)).hex(),after_nonvolatile=preserved_registers())
                return False
        class SubsectionEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('*write_subsection',internal=True)
            def stop(self):
                frame=gdb.newest_frame();parent=frame.older();assert parent is not None and parent.name()=='flush_packed_file' and len(subsection_calls)<58,'subsection actual caller/inventory differs'
                block=frame.block()
                while block is not None and block.function is None:block=block.superblock
                formals={symbol.name:symbol.type.strip_typedefs() for symbol in block if symbol.is_argument}
                assert set(formals)=={'index'} and formals['index'].code==gdb.TYPE_CODE_INT and formals['index'].sizeof==4,'subsection actual DWARF ABI differs'
                stack=int(gdb.parse_and_eval('$esp'));words=struct.unpack('<2I',bytes(gdb.selected_inferior().read_memory(stack,8)));index=words[1];assert index==len(subsection_calls),'subsection actual index sequence differs'
                root=int(gdb.parse_and_eval('directory_entries'));row=bytes(gdb.selected_inferior().read_memory(root+12*index,12));kind,module,pointer,size=struct.unpack('<HHII',row)
                assert 0<=size<=65512 and not any(item['address'] and size and max(pointer,item['address'])<min(pointer+size,item['address']+item['size']) for item in main_free_calls+source_family_frees),'subsection source overlaps a retired generation'
                record=dict(sequence=len(subsection_calls),index=index,entry_esp=stack,return_pc=words[0],before_used=int(gdb.parse_and_eval('output_used')),before_row=row.hex(),input_address=pointer,input=bytes(gdb.selected_inferior().read_memory(pointer,size)).hex(),before_nonvolatile=preserved_registers(),formal_inventory={'index':str(formals['index'])})
                subsection_calls.append(record);SubsectionReturned(record);return False
        SubsectionEntered()

    if config.get('observe_source_index_complete'):
        class SourceFreeReturned(gdb.FinishBreakpoint):
            def __init__(self,record):
                self.record=record
                super().__init__(gdb.newest_frame(),internal=True)
            def stop(self):
                self.record.update(returned=True,exit_esp=int(gdb.parse_and_eval('$esp')),exit_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                return False
        class SourceFreeEntered(gdb.Breakpoint):
            def __init__(self):super().__init__('recon_free',internal=True)
            def stop(self):
                frame=gdb.newest_frame();owner=frame.older()
                if owner is None or owner.name()!='build_source_file_index':return False
                index=len(source_family_frees);assert len(source_family_allocations)==3 and index<2,'source family free inventory differs'
                allocation=source_family_allocations[index];pointer=int(frame.read_var('pointer'))
                assert pointer==allocation['address'] and int(owner.read_var('block' if index==0 else 'name'))==pointer,'source family free actual local/owner differs'
                record=dict(index=index,address=pointer,size=allocation['size'],owner=owner.name(),before=bytes(gdb.selected_inferior().read_memory(pointer,allocation['size'])).hex(),entry_esp=int(gdb.parse_and_eval('$ebp'))+4,entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
                source_family_frees.append(record);SourceFreeReturned(record);return False
        SourceFreeEntered()

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
                if owner is None or owner.name() not in ('cvpack_main','free_widen_buffers','free_type_tables'):return False
                names=['debug_directory','line_data','symbol_data','type_data','module_data']
                capacities=['module_capacity','type_segment_bytes','symbol_capacity','line_capacity','type_capacity']
                if config.get('observe_widen_frees'):
                    names+=['pack_buffer','pack_fixups'];capacities+=['pack_buffer_size','pack_fixup_count']
                if config.get('observe_type_table_frees'):
                    names+=['type_hash_buckets','type_table_primary','type_table_secondary','type_table_pending','type_records']
                    capacities+=['1536','16384','16384','16384','type_record_capacity*32']
                index=len(main_free_calls);assert index<len(names),'unexpected additional main free'
                name=names[index];address=int(frame.read_var('pointer'))
                assert owner.name()==('cvpack_main' if index<5 else 'free_widen_buffers' if index<7 else 'free_type_tables'),'cleanup free owner/order differs'
                assert address==int(gdb.parse_and_eval(name)),'main free argument is not its actual source global'
                size=int(gdb.parse_and_eval(capacities[index])) if address else 0
                if index==1:size=min(size,0xffe8)
                if index==6:size*=8
                assert 0<=size<=1048576,'main free exceeds observed allocation bound'
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
            assert not any(row.get('returned') and row['address'] and max(int(address),row['address'])<min(int(address)+int(size),row['address']+row['size']) for row in main_free_calls+source_family_frees), 'current-state observer tried to dereference released payload'
            return actual_inferior.read_memory(address,size)
    inferior=CurrentMemory()
    def historical_table(address,size,name):
        if not released(address):return bytes(inferior.read_memory(address,size))
        assert config.get('observe_type_table_frees') and name in ('type_table_primary','type_table_secondary','type_records'),'unmodeled historical table role'
        calls=[row for row in main_free_calls if row['address']==address and row['returned']]
        assert len(calls)==1 and calls[0]['name']==name and calls[0]['size']==size,'historical table free identity differs'
        state.setdefault('historical_type_tables',[]).append(dict(name=name,free_index=calls[0]['index'],size=size))
        return bytes.fromhex(calls[0]['before'])
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
        elif item['kind']=='directory-output-pointer':
            root=int(gdb.parse_and_eval(item['root']));count=int(gdb.parse_and_eval(item['count']));address=int(value);width=item['element_size']
            assert address==root+count*width and 0<=count and (count+1)*width<=directory_allocation['size'],'output directory argument lacks actual spare owner/extent'
            parameters[item['name']]=dict(root=item['root'],index=count,extent=width)
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
            if item.get('field'):local=local[item['field']]
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
        elif item['kind']=='packed-directory-entry':
            root=int(gdb.parse_and_eval('directory_entries'));count=int(gdb.parse_and_eval('directory_count'));address=int(value)
            assert gdb.lookup_type('CodeViewDirectoryEntry').sizeof==12 and root<=address<root+count*12 and (address-root)%12==0,'packed directory argument exceeds initialized active rows'
            assert frame.older() is not None and frame.older().name()=='recon_qsort','packed directory callback lacks actual qsort caller'
            parameters[item['name']]=dict(index=(address-root)//12,width=12)
        elif item['kind']=='source-file-name':
            owner=frame.older();assert owner is not None and owner.name()=='build_source_file_index'
            info=owner.read_var('info');assert info.type.target().sizeof==48
            module=info.dereference();root=int(module['source_lines']);size=int(module['source_lines_size'])
            payload=bytes(inferior.read_memory(root,size));count=int.from_bytes(payload[:2],'little');matches=[]
            assert 4+count*4<=size,'source-file offset array exceeds source module'
            for index in range(count):
                offset=int.from_bytes(payload[4+index*4:8+index*4],'little');assert offset+4<=size
                segments=int.from_bytes(payload[offset:offset+2],'little');offset+=4+segments*12
                assert offset<size and offset+1+payload[offset]<=size,'counted source name exceeds live module'
                if root+offset==int(value):matches.append((index,offset,payload[offset:offset+1+payload[offset]]))
            assert len(matches)==1,'source-name argument lacks exact live module owner'
            index,offset,text=matches[0];parameters[item['name']]=dict(module=int(module['module']),file_index=index,source_offset=offset,bytes=text.hex())
            block=int(owner.read_var('module_tail'));alloc=[row for row in source_index_allocations if row.get('address')==block]
            assert len(alloc)==1 and alloc[0]['owner']=='build_source_file_index' and alloc[0]['after_frees']==12,'new source-index block lacks actual allocation generation'
            assert alloc[0]['size']==8+count*4 and owner.read_var('modules').type.sizeof==8 and owner.read_var('names').type.sizeof==16
            state['source_index_context']=dict(module_address=int(info),source_address=root,source_size=size,block_address=block,block_size=alloc[0]['size'],
                block_bytes=bytes(actual_inferior.read_memory(block,alloc[0]['size'])).hex(),generation=dict(owner=alloc[0]['owner'],after_frees=alloc[0]['after_frees']),
                module_sentinel_address=int(owner.read_var('modules').address),module_sentinel_bytes=bytes(inferior.read_memory(owner.read_var('modules').address,8)).hex(),
                names_sentinel_address=int(owner.read_var('names').address),offset_cursor=int(owner.read_var('offsets')),file_index=int(owner.read_var('file_index')),
                total_files=int(owner.read_var('total_files')),name_bytes=int(owner.read_var('name_bytes')))
        elif item['kind']=='caller-local-pointer':
            owner=frame.older()
            while owner is not None and owner.name()!=item['owner']:owner=owner.older()
            assert owner is not None
            local=owner.read_var(item['local']);address=int(value)
            if item.get('field'):local=local[item['field']]
            assert int(local.address)==address and local.type.sizeof==item['width'],'output argument caller-local identity differs'
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],width=item['width'])
            if item.get('field'):parameters[item['name']]['field']=item['field']
            if item.get('initialized'):
                raw=bytes(inferior.read_memory(address,item['width']))
                if item.get('arena_output'):
                    words=list(struct.unpack('<6I',raw));assert len(name_hash_allocations)==5 and words[2]==words[5]==name_hash_allocations[3]['address'] and words[3]==name_hash_allocations[4]['address']+2,'arena output argument actual new-owner aliases differ'
                    parameters[item['name']]['description']=dict(capacity=words[0],blocks=words[1],remaining=words[4],last='namehash-output-block',first='namehash-output-block',cursor_owner='namehash-output-payload',cursor_offset=2)
                elif item.get('widen_output'):
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
    if config.get('observe_public_writer_prefix'):
        owner=frame.older()
        while owner is not None and owner.name()!='write_publics':owner=owner.older()
        assert owner is not None and owner.older() is not None and owner.older().name()=='flush_packed_file','public writer actual parent scope differs'
        header=owner.read_var('header');actual=header.type.strip_typedefs();assert actual.code==gdb.TYPE_CODE_STRUCT and actual.sizeof==16,'public writer header actual type/extent differs'
        layout={field.name:dict(offset=int(field.bitpos)//8,width=int(field.type.strip_typedefs().sizeof)) for field in actual.fields()}
        assert layout=={'name_method':dict(offset=0,width=2),'address_method':dict(offset=2,width=2),'symbol_bytes':dict(offset=4,width=4),'name_bytes':dict(offset=8,width=4),'address_bytes':dict(offset=12,width=4)},'public writer actual header field layout differs'
        padding=owner.read_var('padding');actual=padding.type.strip_typedefs();assert actual.code==gdb.TYPE_CODE_INT and actual.sizeof==4 and str(actual)=='unsigned int','public writer padding actual type differs'
        block=owner.block()
        while block is not None and block.function is None:block=block.superblock
        formals={symbol.name:symbol.type.strip_typedefs() for symbol in block if symbol.is_argument};assert set(formals)=={'entry','offset'} and formals['entry'].code==gdb.TYPE_CODE_PTR and formals['entry'].sizeof==4 and formals['entry'].target().strip_typedefs().sizeof==12 and formals['offset'].code==gdb.TYPE_CODE_INT and formals['offset'].sizeof==4,'public writer actual parent formal ABI differs'
        streams=[]
        for local in ('names','addresses'):
            value=owner.read_var(local);kind=value.type.strip_typedefs();assert kind.code==gdb.TYPE_CODE_STRUCT and kind.sizeof==24,'public writer actual stream extent differs'
            base=int(value.address);arena_layout={name:dict(offset=int(value[name].address)-base,width=int(value[name].type.sizeof)) for name in ('capacity','block_count','last','cursor','remaining','first')}
            assert arena_layout=={name:dict(offset=index*4,width=4) for index,name in enumerate(('capacity','block_count','last','cursor','remaining','first'))},'public writer actual arena member layout differs'
            alias=value['fields'];assert int(alias.address)-base==4 and alias.type.strip_typedefs().code==gdb.TYPE_CODE_ARRAY and alias.type.sizeof==20 and alias.type.target().sizeof==4,'public writer actual anonymous arena alias differs'
            streams.append(dict(local=local,address=int(value.address),width=int(kind.sizeof),first_offset=20))
        state['public_writer_frame']=dict(owner=owner.name(),caller=owner.older().name(),header_address=int(header.address),header=bytes(inferior.read_memory(header.address,16)).hex(),header_layout=layout,padding_address=int(padding.address),padding=int(padding),entry=int(owner.read_var('entry')),offset=int(owner.read_var('offset')),stream_objects=streams,uninitialized_streams=streams[1:] if config.get('observe_name_hash') else streams,uninitialized_stream_bytes_uncompared=24 if config.get('observe_name_hash') else sum(row['width'] for row in streams))
    if config.get('observe_name_hash'):
        if config.get('observe_name_hash_complete'):
            assert name_hash_first_frame,'missing actual namehash before-fill frame'
            state['name_hash_frame']=name_hash_first_frame
            owner=frame.older();assert owner.name()=='write_publics'
            arena=owner.read_var('names');raw=bytes(actual_inferior.read_memory(arena.address,24));words=struct.unpack('<6I',raw);blockbytes=bytes(actual_inferior.read_memory(words[2],12));used,address,nextblock=struct.unpack('<3I',blockbytes);assert used==564
            state['name_hash_complete']=dict(arena_address=int(arena.address),arena=raw.hex(),block_address=words[2],block=blockbytes.hex(),payload_address=address,payload=bytes(actual_inferior.read_memory(address,used)).hex())
        else:
            owner=frame.older();assert owner is not None and owner.name()=='build_symbol_name_hash' and owner.older().name()=='write_publics','namehash actual frame owner differs'
            state['name_hash_frame']=name_hash_frame_snapshot(owner,actual_inferior)
    state['file_calls']=file_calls
    state['mapping_calls']=mapping_calls
    state['file_reads']=file_reads
    state['directory_allocation']=directory_allocation
    state['main_free_calls']=main_free_calls
    state['source_index_allocations']=source_index_allocations
    state['source_family_allocations']=source_family_allocations
    state['source_family_frees']=source_family_frees
    state['output_buffer_allocations']=output_buffer_allocations
    state['subsection_calls']=subsection_calls
    state['public_padding']=public_padding
    state['name_hash_allocations']=name_hash_allocations
    state['name_hash_entry']=name_hash_entry
    state['name_hash_reused_calls']=name_hash_reused_calls
    state['name_hash_stream_calls']=name_hash_stream_calls
    state['name_hash_temp_frees']=name_hash_temp_frees
    state['directory_before_writer']=output_directory_before
    state['directory_sort_input']=output_directory_sort
    state['directory_sort_complete']=output_directory_complete
    state['sort_calls']=sort_calls
    state['sort_environments']=sort_environments
    if config.get('observe_source_index_complete'):
        assert len(source_family_allocations)==3 and len(source_family_frees)==2 and all(row.get('returned') for row in source_family_frees),'source family completion inventory differs'
        allocation=source_family_allocations[2];address=int(gdb.parse_and_eval('source_file_index'));size=int(gdb.parse_and_eval('source_file_index_size'))
        assert address==allocation['address'] and size==allocation['size']==160,'source family live output allocation identity differs'
        state['source_family_output']=dict(address=address,bytes=bytes(actual_inferior.read_memory(address,size)).hex())
    state['widen_sequence']=widen_sequence
    state['widen_release_calls']=widen_release_calls
    state['widen_emit_calls']=widen_emit_calls
    if config.get('observe_output_buffer_init'):
        address=int(gdb.parse_and_eval('output_buffer'));capacity=int(gdb.parse_and_eval('output_capacity'));used=int(gdb.parse_and_eval('output_used'));file=int(gdb.parse_and_eval('output_file'))
        assert address and used==(5056 if config.get('observe_subsection_output') else 8) and used<=capacity and output_buffer_allocations and output_buffer_allocations[-1]['address']==address and output_buffer_allocations[-1]['size']==capacity,'output buffer initialized-prefix observation differs'
        state['output_buffer_state']=dict(address=address,capacity=capacity,used=used,file=file,bytes=bytes(actual_inferior.read_memory(address,used)).hex(),environment=sort_environment())
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
        if config.get('observe_output_directory') and item['name']=='directory_entries':size=output_directory_before['count']*12
        elif size is None:size=int(gdb.parse_and_eval(spec['count']))*spec['element_size']
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
                heads=historical_table(root,16384,item['name']);seen=set()
                if not released(root):memory[str(root)]=heads.hex()
                for offset in range(0,16384,4):
                    head=int.from_bytes(heads[offset:offset+4],'little')
                    while head:
                        assert head not in seen and len(seen)<4096,'recursive-type chain is cyclic/unbounded'
                        seen.add(head);node=bytes(inferior.read_memory(head,12));memory[str(head)]=node.hex()
                        head=int.from_bytes(node[8:12],'little')
        if item.get('interned_types'):
            root=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
            if root:
                heads=historical_table(root,16384,item['name']);seen=set()
                if not released(root):memory[str(root)]=heads.hex()
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
                records=historical_table(address,size,item['name'])
                if not released(address):memory[str(address)]=records.hex()
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
