"""GDB-only CRT observations without calling functions in the inferior."""
import json
import os
from pathlib import Path
import stat
import hashlib
import gdb


def install_sopen_observer(output,work,master):
    events=[]
    output=Path(output)
    def save():output.write_text(json.dumps(events,indent=2)+'\n')
    class Returned(gdb.FinishBreakpoint):
        def __init__(self,entry):
            self.entry=entry
            super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            try:
                fd=int(self.return_value)
                assert 0<=fd<int(gdb.parse_and_eval('recon_nhandle')),'conditional CRT descriptor out of bounds'
                handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%fd))
                pid=gdb.selected_inferior().pid
                host=handle-1
                path=Path('/proc')/str(pid)/'fd'/str(host)
                actual=os.stat(path)
                expected=os.stat(Path(work)/self.entry['name'])
                owned=(actual.st_dev,actual.st_ino)==(expected.st_dev,expected.st_ino)
                info=(Path('/proc')/str(pid)/'fdinfo'/str(host)).read_text()
                position=int(next(line.split()[1]for line in info.splitlines()if line.startswith('pos:')))
                data=(Path(work)/self.entry['name']).read_bytes()
                input_matches=data==Path(master).read_bytes()
                self.entry['return']=dict(descriptor=fd,errno=int(gdb.parse_and_eval('(int)errno')),
                    osfile=int(gdb.parse_and_eval('recon_osfile[%d]'%fd)),type=1 if stat.S_ISREG(actual.st_mode)else -1,
                    position=position,owned=owned,host_descriptor=host,input_matches=input_matches,input_sha256=hashlib.sha256(data).hexdigest())
                save()
                assert owned and input_matches and position==0 and self.entry['return']['osfile']==1 and self.entry['return']['type']==1
                return False
            except Exception as error:
                self.entry['error']=str(error);save();return True
    class Entered(gdb.Breakpoint):
        def __init__(self):super().__init__('recon_sopen',internal=True)
        def stop(self):
            frame=gdb.newest_frame()
            entry=dict(name=frame.read_var('name').string(),mode=int(frame.read_var('mode')),
                share=int(frame.read_var('sharing')),
                permission=int.from_bytes(bytes(gdb.selected_inferior().read_memory(int(frame.read_var('sharing').address)+4,4)),'little'))
            events.append(entry);save()
            if entry['name']!='hello-debug.dll' or entry['mode']!=0x8002 or entry['share']!=0x40:
                entry['error']='unsupported native conditional CRT invocation';save();return True
            Returned(entry)
            return False
    Entered()
    save()


def install_read_observer(output,sites):
    events=[];output=Path(output)
    by_line={r['native_line']:r for r in sites}
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
                site=by_line[gdb.find_pc_line(int(caller.pc())-1).line]
                if caller.name()!=site.get('owner','read_image_debug'):raise RuntimeError('unobserved read caller')
                handle=int(f.read_var('handle'));address=int(f.read_var('buffer'));count=int(f.read_var('size'))
                local=gdb.parse_and_eval(site['native_local'])if site.get('global_buffer')else caller.read_var(site['native_local'])
                if site.get('heap_buffer'):
                    assert int(local)==address and count==int(gdb.parse_and_eval(site['count_global']))*site['element_size'],'unidentified read heap allocation'
                else:assert int(local.address)==address and local.type.sizeof==count==site['size'],'unidentified read output local'
                assert 0<=count<=4096
                position,fd,flags,host_position=state(handle)
                entry=dict(site=site['call'],handle=handle,address=address,count=count,entry_errno=int(gdb.parse_and_eval('(int)errno')),entry_position=position,descriptor=fd,entry_osfile=flags,entry_host_position=host_position,
                    native_return_pc=int(caller.pc()),prebytes=bytes(gdb.selected_inferior().read_memory(address,count)).hex())
                events.append(entry);save();Returned(entry);return False
            except Exception as error:events.append(dict(error=str(error)));save();return True
    Entered();save()
