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
            matches=[p for p in policies if all(entry[key]==p[key]for key in ('name','mode','share'))]
            if len(matches)!=1 or active:
                entry['error']='unsupported native conditional CRT invocation';save();return True
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
                assert 0<=count<=4096
                position,fd,flags,host_position=state(handle)
                entry=dict(site=site['call'],handle=handle,address=address,count=count,entry_errno=int(gdb.parse_and_eval('(int)errno')),entry_position=position,descriptor=fd,entry_osfile=flags,entry_host_position=host_position,
                    native_return_pc=int(caller.pc()),prebytes=bytes(gdb.selected_inferior().read_memory(address,count)).hex())
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
    active=events[-1];current=gdb.newest_frame();caller=current.older();outer=caller.older()
    assert current.name()=='advance_file_page'and caller.name()=='read_buffered_file'and outer.name()=='read_file','unreviewed pending read consumer'
    sal=gdb.find_pc_line(int(caller.pc())-1)
    assert Path(sal.symtab.fullname()).resolve()==Path(source).resolve()and sal.line==line,'pending read caller site differs'
    slot=current.read_var('slot');address=int(caller.read_var('buffer'));destination=int(caller.read_var('destination'))
    copied=int(caller.read_var('copied'));counter=int(caller.read_var('remaining'));size=int(caller.read_var('size'))
    assert int(slot)==int(caller.read_var('slot'))and int(slot)==int(gdb.parse_and_eval('file_slots[%d]'%active['handle']))
    assert outer.older().name()=='read_publics'and int(outer.read_var('buffer'))==address and int(outer.read_var('size'))==size
    assert address==active['address']==destination and size==counter==active['count']==209 and copied==197
    assert int(gdb.parse_and_eval('current_module->module'))==3
    page=slot.dereference()['cursor']['page'].dereference()
    assert int(page['position'])==int(page['limit'])
    return [dict(site=active['site'],address=address,count=size,copied=copied,remaining=counter-copied,counter=counter,
        position=int(page['position']),slot=int(slot),caller=0x2a3dd,readreturn=0x17e6e,destination=destination,cursor=int(page['cursor']),module=3,
        pending=True,returned=False,native_return_pc=int(caller.pc()),native_source_line=sal.line,bytes=bytes(gdb.selected_inferior().read_memory(address,size)).hex())]
