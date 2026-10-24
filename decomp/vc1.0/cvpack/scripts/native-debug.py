"""GDB-only CRT observations without calling functions in the inferior."""
import json
import os
from pathlib import Path
import stat
import hashlib
import gdb


def install_sopen_observer(output,work,policies):
    events=[];active=[];output=Path(output)
    def save():output.write_text(json.dumps(events,indent=2)+'\n')
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,entry,policy):
            self.entry,self.policy=entry,policy
            super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            try:
                fd=int(self.return_value)
                error=int(gdb.parse_and_eval('(int)errno'))
                last_error=int(gdb.parse_and_eval("'win32.c'::last_error"))
                assert 0<=fd<int(gdb.parse_and_eval('recon_nhandle')),'conditional CRT descriptor out of bounds'
                handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%fd));pid=gdb.selected_inferior().pid;host=handle-1
                path=Path('/proc')/str(pid)/'fd'/str(host)
                actual=os.stat(path);expected=os.stat(self.policy['input'])
                owned=(actual.st_dev,actual.st_ino)==(expected.st_dev,expected.st_ino)
                info=(Path('/proc')/str(pid)/'fdinfo'/str(host)).read_text()
                position=int(next(line.split()[1]for line in info.splitlines()if line.startswith('pos:')))
                flags=int(next(line.split()[1]for line in info.splitlines()if line.startswith('flags:')),8)
                data=Path(self.policy['input']).read_bytes();input_matches=data==Path(self.policy['master']).read_bytes()
                self.entry['return']=dict(descriptor=fd,errno=error,last_error=last_error,
                    osfile=int(gdb.parse_and_eval('recon_osfile[%d]'%fd)),type=1 if stat.S_ISREG(actual.st_mode)else -1,
                    position=position,owned=owned,host_descriptor=host,access_mode=flags&3,input_matches=input_matches,input_sha256=hashlib.sha256(data).hexdigest())
                assert self.entry.get('createfile')and active==[self.entry],'missing actual native CreateFile call'
                active.clear();save()
                assert owned and input_matches and position==0 and self.entry['return']['osfile']==1 and self.entry['return']['type']==1
                return False
            except Exception as error:self.entry['error']=str(error);save();return True
    class Entered(gdb.Breakpoint):
        def __init__(self):super().__init__('recon_sopen',internal=True)
        def stop(self):
            frame=gdb.newest_frame()
            entry=dict(name=frame.read_var('name').string(),mode=int(frame.read_var('mode')),share=int(frame.read_var('sharing')),
                permission=int.from_bytes(bytes(gdb.selected_inferior().read_memory(int(frame.read_var('sharing').address)+4,4)),'little'))
            events.append(entry);save()
            matches=[p for p in policies if p['phase']==len(events)and all(entry[key]==p[key]for key in ('name','mode','share'))]
            if len(matches)!=1 or active:
                entry['error']='unsupported native conditional CRT invocation';save();return True
            if Path(matches[0]['input']).read_bytes()!=Path(matches[0]['master']).read_bytes():
                entry['error']='conditional input mismatches exact next phase before open';save();return True
            active.append(entry);Returned(entry,matches[0]);return False
    class Created(gdb.Breakpoint):
        def __init__(self):super().__init__('recon_CreateFileA',internal=True)
        def stop(self):
            if not active:return False
            entry=active[0]
            try:
                assert 'createfile'not in entry,'multiple native CreateFile calls'
                frame=gdb.newest_frame();security=frame.read_var('security')
                call=dict(name=frame.read_var('name').string(),access=int(frame.read_var('access')),share=int(frame.read_var('share')),
                    disposition=int(frame.read_var('disposition')),attrs=int(frame.read_var('attributes')),
                    inherit=bool(int(security)and int(security.dereference()['bInheritHandle'])))
                assert not int(frame.read_var('template'))
                entry['createfile']=call;save();return False
            except Exception as error:entry['error']=str(error);save();return True
    Entered();Created();save()


def install_read_observer(output,sites):
    events=[];output=Path(output)
    gdb.cvpack_read_events=events
    by_line={(r.get('owner','read_image_debug'),r['native_line']):r for r in sites}
    def save():output.write_text(json.dumps(events,indent=2)+'\n')
    def state(handle):
        slot=gdb.parse_and_eval('file_slots[%d]'%handle).dereference()
        assert not(int(slot['flags'])&8),'unobserved mapped read'
        fd=int(slot['source']['descriptor']);assert 0<=fd<int(gdb.parse_and_eval('recon_nhandle'))
        host=int(gdb.parse_and_eval('recon_osfhnd[%d]'%fd))-1
        info=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,host)).read_text()
        position=int(next(line.split()[1]for line in info.splitlines()if line.startswith('pos:')))
        return int(slot['cursor']['page'].dereference()['position']),fd,int(gdb.parse_and_eval('recon_osfile[%d]'%fd)),position
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,entry):self.entry=entry;super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            try:
                position,fd,flags,host_position=state(self.entry['handle'])
                self.entry.update(returned=int(self.return_value),errno=int(gdb.parse_and_eval('(int)errno')),position=position,descriptor_return=fd,osfile=flags,host_position=host_position,
                    postbytes=bytes(gdb.selected_inferior().read_memory(self.entry['address'],self.entry['count'])).hex())
                save();return False
            except Exception as error:self.entry['error']=str(error);save();return True
    class Entered(gdb.Breakpoint):
        def __init__(self):super().__init__('read_file',internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();caller=f.older()
                site=by_line[(caller.name(),gdb.find_pc_line(int(caller.pc())-1).line)]
                if caller.name()!=site.get('owner','read_image_debug'):raise RuntimeError('unobserved read caller')
                handle=int(f.read_var('handle'));address=int(f.read_var('buffer'));count=int(f.read_var('size'))
                local=gdb.parse_and_eval(site['native_local'])if site.get('global_buffer')else caller.read_var(site['native_local'])
                if site.get('heap_buffer'):
                    expected=int(caller.read_var(site['count_local_value']))if 'count_local_value'in site else int(gdb.parse_and_eval(site['count_native']))if 'count_native'in site else int(gdb.parse_and_eval(site['count_global']))*site['element_size']if 'count_global'in site else int(caller.read_var(site['count_local']).dereference()[site['count_field']])&((1<<site['count_bits'])-1)
                    assert int(local)==address and count==expected,'unidentified read heap allocation'
                else:assert int(local.address)==address and local.type.sizeof==count==site['size'],'unidentified read output local'
                assert 0<=count<=site.get('maximum_count',4096)
                position,fd,flags,host_position=state(handle)
                entry=dict(site=site['call'],handle=handle,address=address,count=count,entry_errno=int(gdb.parse_and_eval('(int)errno')),entry_position=position,descriptor=fd,entry_osfile=flags,entry_host_position=host_position,
                    native_return_pc=int(caller.pc()),prebytes=bytes(gdb.selected_inferior().read_memory(address,count)).hex())
                if site['call']==0x17dc9 and count==4746:
                    page=gdb.parse_and_eval('file_slots[%d]'%handle).dereference()['cursor']['page']
                    entry['page_entry']=dict(site=site['call'],slot=int(gdb.parse_and_eval('file_slots[%d]'%handle)),page=int(page),flags=int(page.dereference()['flags']),position=int(page.dereference()['position']),address=address)
                events.append(entry);save();Returned(entry);return False
            except Exception as error:events.append(dict(error=str(error)));save();return True
    Entered();save()

def install_pdb_header_observer(output,master,prepared):
    events=[];output=Path(output)
    def save():output.write_text(json.dumps(events,indent=2)+'\n')
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,fd,address,size):
            self.fd,self.address,self.size=fd,address,size
            super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            try:
                error=int(gdb.parse_and_eval('(int)errno'));last_error=int(gdb.parse_and_eval("'win32.c'::last_error"))
                assert 0<=self.fd<int(gdb.parse_and_eval('recon_nhandle'))
                host=int(gdb.parse_and_eval('recon_osfhnd[%d]'%self.fd))-1;pid=gdb.selected_inferior().pid
                info=Path('/proc/%d/fdinfo/%d'%(pid,host)).read_text()
                position=int(next(line.split()[1]for line in info.splitlines()if line.startswith('pos:')))
                actual=os.stat('/proc/%d/fd/%d'%(pid,host));expected=os.stat(prepared)
                event=dict(descriptor=self.fd,returned=int(self.return_value),errno=error,last_error=last_error,position=position,
                    osfile=int(gdb.parse_and_eval('recon_osfile[%d]'%self.fd)),type=1 if stat.S_ISREG(actual.st_mode)else -1,
                    owned=int((actual.st_dev,actual.st_ino)==(expected.st_dev,expected.st_ino)),input_matches=int(Path(master).read_bytes()==Path(prepared).read_bytes()),
                    address=self.address,header=bytes(gdb.selected_inferior().read_memory(self.address,self.size)).hex())
                events.append(event);save();assert event['returned']==self.size;return False
            except Exception as error:events.append(dict(error=str(error)));save();return True
    class Entered(gdb.Breakpoint):
        def __init__(self):super().__init__('recon_read',internal=True)
        def stop(self):
            frame=gdb.newest_frame();caller=frame.older()
            if caller.name()not in('type_server_open_file','type_server_read_records'):return False
            try:
                address=int(frame.read_var('buffer'));fd=int(frame.read_var('fd'));size=int(frame.read_var('count'))
                server=caller.read_var('server');parent=int(server)
                assert server.type.target().sizeof==116
                if caller.name()=='type_server_open_file':
                    assert size==64 and address==parent,'unreviewed PDB header destination'
                    owner=caller.older()
                else:
                    block=server.dereference()['type_storage']['last']
                    assert size==4140 and address==int(block)+12 and int(block.dereference()['cursor'])==address+size and int(block.dereference()['available'])==12288-size,'unreviewed PDB payload allocation'
                    assert caller.older().name()=='type_server_open_file'and parent==int(caller.older().read_var('server'))
                    owner=caller.older().older()
                assert owner.name()=='type_server_open'and parent==int(owner.read_var('server')),'unreviewed PDB constructor lifetime'
                Returned(fd,address,size);return False
            except Exception as error:events.append(dict(error=str(error)));save();return True
    Entered();save()


def install_pdb_consumed_observer(output,source,line):
    """Read the actual field at its native consumer, without initialization."""
    events=[];output=Path(output)
    def save():output.write_text(json.dumps(events,indent=2)+'\n')
    class Consumed(gdb.Breakpoint):
        def __init__(self):super().__init__(source+':'+str(line),internal=True)
        def stop(self):
            try:
                caller=gdb.newest_frame();assert caller.name()=='type_server_read_records'
                server=caller.read_var('server');address=int(server)
                assert server.type.target().sizeof==116
                assert caller.older().name()=='type_server_open_file'and address==int(caller.older().read_var('server'))
                owner=caller.older().older();assert owner.name()=='type_server_open'and address==int(owner.read_var('server'))
                events.append(dict(address=address,value=int(server.dereference()['incremental'])&0xffffffff,site=0x2b1a7));save();return False
            except Exception as error:events.append(dict(error=str(error)));save();return True
    Consumed();save()


def capture_read_progress(source,line):
    events=getattr(gdb,'cvpack_read_events',[])
    if not events or 'returned'in events[-1]:return []
    active=events[-1];current=gdb.newest_frame();caller=current.older();nested=0;inner_return=0;shift=0;nested_line=0
    typed=active['site']==0x17dc9
    assert active['site']in(0x17e69,0x17dc9),'unreviewed pending read site'
    count,copied_expected=(4746,3076)if typed else(209,197)
    if current.name()in('flush_file_page','release_page'):
        assert caller.name()=='advance_file_page','unreviewed nested pending-read caller'
        assert current.name()==('release_page'if typed else 'flush_file_page')
        nested_sal=gdb.find_pc_line(int(caller.pc())-1)
        call=('release_page'if typed else 'flush_file_page')+'(slot->cursor.page);'
        nested_line=next(i for i,text in enumerate(Path(source).read_text().splitlines(),1)if text.strip()==call)
        assert Path(nested_sal.symtab.fullname()).resolve()==Path(source).resolve()and nested_sal.line==nested_line
        slot=caller.read_var('slot');assert int(current.read_var('page'))==int(slot.dereference()['cursor']['page'])
        caller=caller.older();nested=1;inner_return=0x2a5ef if typed else 0x2a606;shift=5
    else:
        assert not typed and current.name()=='advance_file_page','unreviewed pending-read consumer'
        slot=current.read_var('slot')
    outer=caller.older()
    assert caller.name()=='read_buffered_file'and outer.name()=='read_file','unreviewed pending read loop'
    sal=gdb.find_pc_line(int(caller.pc())-1)
    assert Path(sal.symtab.fullname()).resolve()==Path(source).resolve()and sal.line==line,'pending read caller site differs'
    address=int(caller.read_var('buffer'));destination=int(caller.read_var('destination'))
    copied=int(caller.read_var('copied'));counter=int(caller.read_var('remaining'));size=int(caller.read_var('size'))
    assert int(slot)==int(caller.read_var('slot'))and int(slot)==int(gdb.parse_and_eval('file_slots[%d]'%active['handle']))
    assert outer.older().name()==('read_types'if typed else 'read_publics')and int(outer.read_var('buffer'))==address and int(outer.read_var('size'))==size
    assert address==active['address']==destination and size==counter==active['count']==count and copied==copied_expected
    assert int(gdb.parse_and_eval('current_module->module'))==3
    page=slot.dereference()['cursor']['page'].dereference()
    assert int(page['position'])==int(page['limit'])
    return [dict(site=active['site'],address=address,count=size,copied=copied,remaining=counter-copied,counter=counter,
        position=int(page['position']),slot=int(slot),caller=0x2a3dd,readreturn=0x17dce if typed else 0x17e6e,destination=destination,cursor=int(page['cursor']),module=3,
        frontier=0x2a850 if typed else 0x2a120 if nested else 0x2a5c0,innerreturn=inner_return,shift=shift,nested_source_line=nested_line,
        pending=True,returned=False,native_return_pc=int(caller.pc()),native_source_line=sal.line,bytes=bytes(gdb.selected_inferior().read_memory(address,size)).hex())]


def capture_type_cache(node):
    raw=bytes.fromhex(node['bytes']);assert len(raw)==2048
    objects={};seen=set()
    for bucket in range(512):
        pointer=int.from_bytes(raw[bucket*4:bucket*4+4],'little');index=0
        while pointer:
            assert pointer not in seen and len(seen)<4096,'type cache ownership cycle/limit'
            seen.add(pointer)
            data=bytes(gdb.selected_inferior().read_memory(pointer,16))
            objects['cache-%d-%d'%(bucket,index)]=dict(address=pointer,bytes=data.hex())
            pointer=int.from_bytes(data[12:16],'little');index+=1
    if objects:node['objects']=objects


def install_symbol_allocation_observer(source,line):
    gdb.cvpack_symbol_allocations=[]
    class Allocated(gdb.Breakpoint):
        def __init__(self):super().__init__(str(source)+':'+str(line),internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();assert f.name()=='pack_new_types'
                base=int(f.read_var('buffer'));size=int(f.read_var('output_size'))
                input=int(f.read_var('types'));entry=f.read_var('entry');module=f.read_var('module')
                padding=int(f.read_var('padding_bytes').dereference());skipped=int(f.read_var('skipped_bytes').dereference())
                assert not gdb.cvpack_symbol_allocations and base and size==5113
                assert int(entry.dereference()['size'])==4746 and int(entry.dereference()['subsection'])==0x124 and int(entry.dereference()['module'])==3
                assert int(module.dereference()['module'])==3 and padding==367 and skipped==0 and size==4746+padding-skipped
                gdb.cvpack_symbol_allocations.append(dict(base=base,size=size,input=input,entry=int(entry),module=int(module),number=3,padding=padding,skipped=skipped,
                    before=bytes(gdb.selected_inferior().read_memory(base,size)).hex()))
                return False
            except Exception as error:gdb.cvpack_symbol_allocations.append(dict(error=str(error)));return True
    Allocated()


def capture_symbol_output():
    rows=getattr(gdb,'cvpack_symbol_allocations',[])
    if not rows:return []
    assert len(rows)==1 and not rows[0].get('error'),'invalid actual symbol allocation'
    entry=rows[0].copy();f=gdb.newest_frame();caller=f.older()
    if f.name()=='address_in_module':
        assert caller.name()=='convert_procedure32'
        entry['module_argument']=int(f.read_var('module'));entry['segment_argument']=int(f.read_var('segment'));entry['offset_argument']=int(f.read_var('offset'))
        assert entry['module_argument']==int(gdb.parse_and_eval('current_module'))==entry['module']
        assert entry['segment_argument']==1 and entry['offset_argument']==0x120
        caller=caller.older();entry['nested_callers']=['convert_procedure32']
    if f.name()=='publish_packed_type':
        assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4199
        entry['type_record']=int(caller.read_var('entry'));entry['type_index']=4199
        entry['record']=int(f.read_var('record'));entry['global_type']=int(f.read_var('type'))
        assert entry['record']==int(caller.read_var('entry').dereference()['data'])and entry['global_type']==0x1049
        entry['fixed_field']=int(gdb.parse_and_eval('*(unsigned short *)'+str(entry['record']+6)))
        assert entry['fixed_field']==0x1047
        caller=caller.older();assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4208
        caller=caller.older();assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4209
        entry['root_type_record']=int(caller.read_var('entry'))
        caller=caller.older();assert caller.name()=='intern_type'and int(caller.read_var('type'))==4209
        assert int(caller.read_var('entry'))==entry['root_type_record']
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        assert int(caller.read_var('entry'))==entry['root_type_record']
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['nested_callers']=['commit_recursive_type','commit_recursive_type','commit_recursive_type','intern_type','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='hash_referenced_type':
        assert caller.name()=='hash_recursive_type'and int(f.read_var('number'))==0
        entry['type_record']=int(f.read_var('entry'));entry['type_index']=4208
        assert int(caller.read_var('entry'))==entry['type_record']
        entry['partial_hash']=int(caller.read_var('hash'))
        caller=caller.older();assert caller.name()=='publish_recursive_type'and int(caller.read_var('entry'))==entry['type_record']
        caller=caller.older();assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4208
        assert int(caller.read_var('entry'))==entry['type_record']
        caller=caller.older();assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4209
        entry['root_type_record']=int(caller.read_var('entry'))
        caller=caller.older();assert caller.name()=='intern_type'and int(caller.read_var('type'))==4209
        assert int(caller.read_var('entry'))==entry['root_type_record']
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        assert int(caller.read_var('entry'))==entry['root_type_record']
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['nested_callers']=['hash_recursive_type','publish_recursive_type','commit_recursive_type','commit_recursive_type','intern_type','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='publish_recursive_type':
        assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4209
        entry['type_record']=int(f.read_var('entry'));entry['type_index']=4209
        assert entry['type_record']==int(caller.read_var('entry'))
        entry['packed_original']=int(caller.read_var('original'));entry['named_lookup_result']=int(caller.read_var('result'))
        assert entry['named_lookup_result']==0 and entry['packed_original']==int(f.read_var('entry').dereference()['data'])
        caller=caller.older();assert caller.name()=='intern_type'and int(caller.read_var('type'))==4209
        entry['hash_bucket']=int(caller.read_var('bucket'))
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        assert int(caller.read_var('entry'))==entry['type_record']
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['nested_callers']=['commit_recursive_type','intern_type','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='commit_recursive_type':
        assert caller.name()=='intern_type'and int(caller.read_var('type'))==4209
        entry['type_record']=int(caller.read_var('entry'));entry['type_index']=int(f.read_var('type'))
        assert entry['type_index']==4209
        entry['hash_bucket']=int(caller.read_var('bucket'))
        assert int(caller.read_var('entry').dereference()['hash_value'])==entry['hash_bucket']
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        assert int(caller.read_var('entry'))==entry['type_record']
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['nested_callers']=['intern_type','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='hash_recursive_type':
        assert caller.name()=='intern_type'and int(caller.read_var('type'))==4209
        entry['type_record']=int(f.read_var('entry'));entry['type_index']=int(caller.read_var('type'))
        assert entry['type_record']==int(caller.read_var('entry'))
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        assert int(caller.read_var('entry'))==entry['type_record']
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['nested_callers']=['intern_type','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='intern_type':
        assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        entry['type_record']=int(f.read_var('entry'));entry['type_index']=int(f.read_var('type'))
        assert entry['type_record']==int(caller.read_var('entry'))and entry['type_index']==4209
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['nested_callers']=['compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='add_recursive_offset':
        assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4208 and int(caller.read_var('count'))==6
        entry['type_record']=int(f.read_var('entry'));entry['recursive_offset']=int(f.read_var('offset'))
        assert entry['type_record']==int(caller.read_var('entry'))and entry['recursive_offset']==int(caller.read_var('recursive_offset'))==54
        assert int(gdb.parse_and_eval('recursive_type_reference'))==1
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['type_index']=4208;entry['nested_callers']=['compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='get_type_record':
        assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4218
        entry['type_record']=int(caller.read_var('entry'))
        assert int(f.read_var('type'))==126 and int(f.read_var('flag'))==int(caller.read_var('unused').address)
        caller=caller.older();assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4219 and int(caller.read_var('count'))==2
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4220
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['type_index']=4218;entry['nested_callers']=['compact_new_type','compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='record_derived_types':
        assert caller.name()=='store_type';entry['type_record']=int(caller.read_var('entry'))
        assert int(f.read_var('type'))==0x1007 and int(f.read_var('list'))==0x1006
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4192
        assert int(caller.read_var('entry'))==entry['type_record']
        caller=caller.older();assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4184 and int(caller.read_var('count'))==6
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4185
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['type_index']=4192;entry['nested_callers']=['store_type','compact_new_type','compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='find_local_type':
        assert caller.name()=='find_named_type'and int(caller.read_var('forward'))==1
        entry['type_record']=int(caller.read_var('entry'));entry['name']=int(f.read_var('name'));entry['kind']=int(f.read_var('kind'))
        assert int(f.read_var('property'))==0 and entry['kind']==5 and int(f.read_var('named'))==int(caller.read_var('named'))
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4097
        assert int(caller.read_var('entry'))==entry['type_record']
        caller=caller.older();assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4184 and int(caller.read_var('count'))==6
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4185
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
        entry['type_index']=4097;entry['nested_callers']=['find_named_type','compact_new_type','compact_type_list','compact_new_type','convert_new_type','convert_udt_symbol']
    if f.name()=='find_global_type':
        assert caller.name()=='find_named_type'and int(caller.read_var('forward'))==0
        entry['type_record']=int(caller.read_var('entry'))
        entry['name']=int(f.read_var('name'));entry['kind']=int(f.read_var('kind'))
        assert entry['kind']==5 and int(f.read_var('named'))==int(caller.read_var('named'))
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4185
        assert entry['type_record']==int(caller.read_var('entry'))
        entry['type_index']=4185;entry['nested_callers']=['find_named_type','compact_new_type','convert_new_type','convert_udt_symbol']
        caller=caller.older();assert caller.name()=='convert_new_type'
        caller=caller.older();assert caller.name()=='convert_udt_symbol';caller=caller.older()
    if f.name()=='find_named_type':
        assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4185
        entry['type_record']=int(f.read_var('entry'));entry['type_index']=int(caller.read_var('type'))
        assert entry['type_record']==int(caller.read_var('entry'))
        entry['nested_callers']=['compact_new_type','convert_new_type','convert_udt_symbol']
        caller=caller.older();assert caller.name()=='convert_new_type'and int(caller.read_var('type'))==4185
        caller=caller.older();assert caller.name()=='convert_udt_symbol'
        caller=caller.older()
    if f.name()=='finish_recursive_types':
        assert caller.name()=='convert_new_type'and int(caller.read_var('type'))==4134
        entry['converted']=int(caller.read_var('converted'))
        entry['type_index']=int(caller.read_var('type'))
        entry['nested_callers']=['convert_new_type','convert_udt_symbol']
        caller=caller.older();assert caller.name()=='convert_udt_symbol'
        caller=caller.older()
    if f.name()=='intern_global':
        assert caller.name()=='convert_udt_symbol','unreviewed nested symbol output caller'
        entry['nested_caller']=caller.name();caller=caller.older()
    if f.name()=='append_buffer':
        names=[]
        assert caller.name()=='save_type','unreviewed append buffer caller'
        entry['type_record']=int(caller.read_var('entry'));entry['page']=int(caller.read_var('page'))
        assert entry['page']==int(gdb.parse_and_eval('type_pages[0]'))and int(caller.read_var('page_index'))==int(caller.read_var('slot'))==0
        assert int(f.read_var('data'))==int(caller.read_var('original'))and int(f.read_var('size'))==8
        assert int(f.read_var('buffer'))==int(gdb.parse_and_eval('&type_symbol_storage'))
        names.append(caller.name());caller=caller.older()
        assert caller.name()=='store_type'and int(caller.read_var('entry'))==entry['type_record']
        names.append(caller.name());caller=caller.older()
        assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4115
        entry['type_index']=int(caller.read_var('type'));entry['list_count']=int(caller.read_var('count'))
        assert entry['list_count']==1
        for expected in('compact_type_list','compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol'):
            assert caller.name()==expected,'unreviewed append buffer frame chain'
            names.append(caller.name());caller=caller.older()
        entry['nested_callers']=names
    if f.name()in('hash_type','save_type'):
        names=[]
        assert caller.name()=='store_type','unreviewed type hash caller'
        owner=caller.read_var('entry');entry['type_record']=int(owner)
        if f.name()=='hash_type':
            entry['record']=int(f.read_var('record'))
            assert entry['record']==int(owner.dereference()['data'])
        else:
            assert int(f.read_var('entry'))==entry['type_record']
            entry['bucket']=int(caller.read_var('bucket'))
        names.append(caller.name());caller=caller.older()
        assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4115,'unreviewed hashed list owner'
        assert entry['type_record']==int(caller.read_var('entry'))
        entry['type_index']=int(caller.read_var('type'));entry['list_count']=int(caller.read_var('count'))
        assert entry['list_count']==1,'unreviewed hashed list extent'
        for expected in('compact_type_list','compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol'):
            assert caller.name()==expected,'unreviewed type hash frame chain'
            names.append(caller.name());caller=caller.older()
        entry['nested_callers']=names
    if f.name()=='store_type':
        names=[]
        assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4115,'unreviewed store type owner'
        entry['type_record']=int(f.read_var('entry'))
        assert entry['type_record']==int(caller.read_var('entry'))
        entry['type_index']=int(caller.read_var('type'));entry['list_count']=int(caller.read_var('count'))
        assert entry['list_count']==1,'unreviewed stored list extent'
        for expected in('compact_type_list','compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol'):
            assert caller.name()==expected,'unreviewed stored list frame chain'
            names.append(caller.name());caller=caller.older()
        entry['nested_callers']=names
    if f.name()=='compact_type_list':
        names=[]
        assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4133,'unreviewed list compact caller/type'
        entry['type_index']=int(caller.read_var('type'))
        entry['list_index']=int(f.read_var('type'));entry['list_count']=int(f.read_var('count'))
        assert (entry['list_index'],entry['list_count'])==(4115,1),'unreviewed compact list arguments'
        for expected in('compact_new_type','compact_pointer','compact_new_type','convert_new_type','convert_udt_symbol'):
            assert caller.name()==expected,'unreviewed compact list frame chain'
            names.append(caller.name());caller=caller.older()
        entry['nested_callers']=names
    if f.name()in('compact_new_type','compact_pointer'):
        names=[]
        if f.name()=='compact_pointer':
            assert caller.name()=='compact_new_type','unreviewed pointer compactor caller'
            entry['type_record']=int(f.read_var('entry'))
            assert entry['type_record']==int(caller.read_var('entry'))
            entry['type_index']=int(caller.read_var('type'))
            names.append(caller.name());caller=caller.older()
        for expected in('convert_new_type','convert_udt_symbol'):
            assert caller.name()==expected,'unreviewed compact symbol output caller'
            names.append(caller.name());caller=caller.older()
        entry['nested_callers']=names
        if f.name()=='compact_new_type':entry['type_index']=int(f.read_var('type'))
        assert entry['type_index']==4134,'unreviewed compact symbol type'
    if f.name()=='create_symbol_reference':
        assert caller.name()=='record_symbol_offsets'and int(caller.read_var('offset'))==108 and int(caller.read_var('size'))==608 and int(caller.read_var('module'))==3
        entry['reference_record']=int(f.read_var('record'));entry['reference_offset']=int(f.read_var('offset'));entry['reference_kind']=int(f.read_var('kind'));entry['reference_module']=int(f.read_var('module'));entry['reference_buckets']=int(f.read_var('buckets'))
        assert entry['reference_record']==entry['base']+108 and entry['reference_offset']==108 and entry['reference_kind']==0x400 and entry['reference_module']==3 and entry['reference_buckets']==int(gdb.parse_and_eval('global_symbol_hash'))
        caller=caller.older()
    assert f.name()in('write_search_symbol','convert_object_name','convert_udt_symbol','intern_global','compact_new_type','compact_pointer','compact_type_list','store_type','hash_type','save_type','append_buffer','finish_recursive_types','find_named_type','find_global_type','find_local_type','record_derived_types','convert_global_data32','get_type_record','add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type','convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes','record_symbol_offsets','create_symbol_reference')and caller.name()=='pack_new_types','unreviewed symbol output consumer'
    base,size=entry['base'],entry['size'];cursor=int(gdb.parse_and_eval('symbol_output'))
    assert int(caller.read_var('buffer'))==int(caller.read_var('start'))==base
    assert int(caller.read_var('end'))==entry['input']+4746 and int(caller.read_var('output_size'))==(608 if f.name()in('link_symbol_scopes','record_symbol_offsets','create_symbol_reference')else size)
    if f.name()in('link_symbol_scopes','record_symbol_offsets','create_symbol_reference'):
        assert cursor==base+608
        if f.name()!='create_symbol_reference':assert int(f.read_var('symbols'))==base and int(f.read_var('size'))==608
        if f.name()=='record_symbol_offsets':assert int(f.read_var('module'))==3
        entry['completed_input']=dict(end=int(caller.read_var('end')))
        assert int(gdb.parse_and_eval('symbol_input'))==entry['completed_input']['end']==entry['input']+4746
    elif f.name()=='write_search_symbol':assert cursor==base+4==int(f.read_var('buffer'))
    else:
        expected=(292,6,4,8)if f.name()=='end_symbol_block'else(244,2,11,23)if f.name()=='convert_register_symbol'else(164,0x200,17,1)if f.name()=='convert_bp_relative32'else(108,0x205,53,7)if f.name()in('convert_procedure32','address_in_module')else(40,4,24,27)if f.name()in('add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type')else(40,4,29,27)if f.name()=='get_type_record'else(40,0x202,43,12)if f.name()=='convert_global_data32'else(40,4,21,27)if f.name()in('find_named_type','find_global_type','find_local_type','record_derived_types')else((16,9,24,31)if f.name()=='convert_object_name'else((40,4,28,27)if f.name()in('compact_new_type','compact_pointer','compact_type_list','store_type','hash_type','save_type','append_buffer','finish_recursive_types')else(40,4,34,27)))
        assert cursor==base+expected[0] and int(caller.read_var('kind'))==expected[1] and int(caller.read_var('length'))==expected[2] and int(caller.read_var('index'))==expected[3]
        entry['record_kind']=int(caller.read_var('kind'));entry['record_length']=int(caller.read_var('length'));entry['callback_index']=int(caller.read_var('index'))
    entry.update(cursor=cursor,frontier=f.name(),parent='pack_new_types',postbytes=bytes(gdb.selected_inferior().read_memory(base,size)).hex())
    return [entry]


def install_type_page_observer(source,line):
    import gdb
    gdb.cvpack_type_pages=[]
    class Allocated(gdb.Breakpoint):
        def __init__(self):super().__init__(source+':'+str(line),internal=True)
        def stop(self):
            try:
                f=gdb.newest_frame();assert f.name()=='save_type'
                base=int(f.read_var('page'));owner=int(f.read_var('entry'))
                index=int(gdb.parse_and_eval('next_type_index'));capacity=int(gdb.parse_and_eval('type_page_capacity'))
                roots=gdb.parse_and_eval('type_pages');assert roots.type.sizeof==480
                assert not gdb.cvpack_type_pages and base and index==0x1000 and capacity==512
                raw_roots=bytes(gdb.selected_inferior().read_memory(roots.address,480));assert raw_roots==b'\0'*480
                gdb.cvpack_type_pages.append(dict(base=base,owner=owner,size=2048,index=index,capacity=capacity,roots_before=raw_roots.hex(),before=bytes(gdb.selected_inferior().read_memory(base,2048)).hex()))
                return False
            except Exception as error:gdb.cvpack_type_pages.append(dict(error=str(error)));return True
    Allocated()


def capture_type_page():
    import gdb
    rows=getattr(gdb,'cvpack_type_pages',[])
    if not rows:return []
    assert len(rows)==1 and not rows[0].get('error'),'invalid type page allocation observations'
    result=rows[0].copy();f=gdb.newest_frame();caller=f.older()
    if f.name()=='finish_recursive_types':
        assert caller.name()=='convert_new_type'and int(caller.read_var('type'))==4134
        assert int(gdb.parse_and_eval('next_type_index'))==0x1003 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='convert_global_data32':
        assert caller.name()=='pack_new_types'
        assert int(gdb.parse_and_eval('next_type_index'))==0x101a and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()in('link_symbol_scopes','record_symbol_offsets','create_symbol_reference'):
        if f.name()=='create_symbol_reference':assert caller.name()=='record_symbol_offsets';caller=caller.older()
        assert caller.name()=='pack_new_types'
        assert int(gdb.parse_and_eval('next_type_index'))==0x1074 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='end_symbol_block':
        assert caller.name()=='pack_new_types'
        assert int(gdb.parse_and_eval('next_type_index'))==0x1071 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='convert_register_symbol':
        assert caller.name()=='pack_new_types'
        assert int(gdb.parse_and_eval('next_type_index'))==0x1071 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='convert_bp_relative32':
        assert caller.name()=='pack_new_types'
        assert int(gdb.parse_and_eval('next_type_index'))==0x1071 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='address_in_module':
        assert caller.name()=='convert_procedure32'
        assert int(gdb.parse_and_eval('next_type_index'))==0x1071 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='convert_procedure32':
        assert caller.name()=='pack_new_types'
        assert int(gdb.parse_and_eval('next_type_index'))==0x1071 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='publish_packed_type':
        assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4199
        assert int(gdb.parse_and_eval('next_type_index'))==0x104a and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='hash_referenced_type':
        assert caller.name()=='hash_recursive_type'and int(caller.read_var('entry'))==int(f.read_var('entry'))
        assert int(gdb.parse_and_eval('next_type_index'))==0x1049 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='publish_recursive_type':
        assert caller.name()=='commit_recursive_type'and int(caller.read_var('type'))==4209
        assert int(gdb.parse_and_eval('next_type_index'))==0x1048 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()in('intern_type','hash_recursive_type','commit_recursive_type'):
        if f.name()in('hash_recursive_type','commit_recursive_type'):
            assert caller.name()=='intern_type'and int(caller.read_var('type'))==4209
            caller=caller.older()
        assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4209
        assert int(gdb.parse_and_eval('next_type_index'))==0x1047 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='add_recursive_offset':
        assert caller.name()=='compact_type_list'and int(caller.read_var('type'))==4208
        assert int(gdb.parse_and_eval('next_type_index'))==0x1046 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='get_type_record':
        assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4218
        assert int(gdb.parse_and_eval('next_type_index'))==0x1023 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='record_derived_types':
        assert caller.name()=='store_type'
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4192
        assert int(gdb.parse_and_eval('next_type_index'))==0x1008 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()=='find_local_type':
        assert caller.name()=='find_named_type'and int(caller.read_var('forward'))==1
        caller=caller.older();assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4097
        assert int(gdb.parse_and_eval('next_type_index'))==0x1006 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    if f.name()in('find_named_type','find_global_type'):
        if f.name()=='find_global_type':
            assert caller.name()=='find_named_type'and int(caller.read_var('forward'))==0
            caller=caller.older()
        assert caller.name()=='compact_new_type'and int(caller.read_var('type'))==4185
        assert int(gdb.parse_and_eval('next_type_index'))==0x1006 and int(gdb.parse_and_eval('type_page_capacity'))==512
        roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
        result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
        return [result]
    assert f.name()=='append_buffer'and caller.name()=='save_type','unreviewed type page consumer'
    assert result['owner']==int(caller.read_var('entry'))and result['base']==int(caller.read_var('page'))
    assert int(caller.read_var('page_index'))==int(caller.read_var('slot'))==0
    assert int(gdb.parse_and_eval('next_type_index'))==0x1000 and int(gdb.parse_and_eval('type_page_capacity'))==512
    roots=gdb.parse_and_eval('type_pages');assert int(roots[0])==result['base']and all(int(roots[i])==0 for i in range(1,120))
    assert int(f.read_var('size'))==8 and int(f.read_var('data'))==int(caller.read_var('original'))
    assert int(f.read_var('buffer'))==int(gdb.parse_and_eval('&type_symbol_storage'))
    result.update(frontier=f.name(),postbytes=bytes(gdb.selected_inferior().read_memory(result['base'],2048)).hex(),roots_after=bytes(gdb.selected_inferior().read_memory(roots.address,480)).hex())
    return [result]
