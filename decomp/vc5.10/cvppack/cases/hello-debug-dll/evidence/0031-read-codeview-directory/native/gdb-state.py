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
                local=gdb.parse_and_eval(site['local']) if site.get('global_buffer') or site.get('heap_buffer') else owner.read_var(site['local'])
                if site.get('heap_buffer'):
                    local=gdb.parse_and_eval(site['local']);count=int(gdb.parse_and_eval(site['count']))
                    capacity=(count+site['spare_elements'])*site['element_size']
                    matches=int(local)==buffer and size==count*site['element_size'] and directory_allocation==dict(address=buffer,size=capacity)
                else:
                    capacity=site['capacity'];matches=int(local.address)==buffer and local.type.sizeof==capacity and size<=capacity
                if matches:
                    role=dict(owner=site['owner'],local=site['local'],capacity=capacity)
                    if role not in roles:roles.append(role)
            assert len(roles)==1,'read buffer actual caller/local/extent is unobserved'
            call=dict(**roles[0],file=file,size=size,entry_position=int(gdb.parse_and_eval('cached_files[%d]->position'%file)),
                entry_last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
            file_reads.append(call);ReadReturned(call,buffer)
            return False
    ReadEntered()
    class AllocationReturned(gdb.FinishBreakpoint):
        def __init__(self):super().__init__(gdb.newest_frame(),internal=True)
        def stop(self):
            directory_allocation['address']=int(gdb.parse_and_eval('$eax'))&0xffffffff
            return False
    class AllocationEntered(gdb.Breakpoint):
        def __init__(self):super().__init__('*malloc',internal=True)
        def stop(self):
            frame=gdb.newest_frame();owner=frame.older()
            if owner is None or owner.name()!='read_codeview_directory':return False
            assert not directory_allocation,'directory allocation generation needs an explicit model'
            stack=int(gdb.parse_and_eval('$esp'))
            directory_allocation['size']=int.from_bytes(bytes(gdb.selected_inferior().read_memory(stack+4,4)),'little')
            AllocationReturned();return False
    AllocationEntered()


def capture(configuration,data_file,output):
    config=json.loads(open(configuration).read());data=json.loads(open(data_file).read())
    frame=gdb.newest_frame();inferior=gdb.selected_inferior()
    state=dict(function=frame.name(),pc=int(gdb.parse_and_eval('$pc')),signal=gdb.execute('info program',to_string=True))
    state['global_addresses']={item['name']:int(gdb.parse_and_eval('&'+item['name'])) for item in data}
    record=next((row for row in config['functions'] if row['native_symbol']==frame.name()),None)
    parameters={}
    for item in (record or {}).get('parameters',[]):
        value=frame.read_var(item['name'])
        if item['kind']=='string':parameters[item['name']]=dict(null=int(value)==0,bytes=(value.string(encoding='latin1').encode('latin1')+b'\0').hex() if int(value) else None)
        elif item['kind']=='argv':parameters[item['name']]=[(value[i].string(encoding='latin1').encode('latin1')+b'\0').hex() for i in range(int(frame.read_var('argc')))]
        elif item['kind']=='integer-pointer':parameters[item['name']]=dict(null=int(value)==0,bytes=bytes(inferior.read_memory(int(value),4)).hex() if int(value) else None)
        elif item['kind']=='blob-element-pointer':
            root=int(gdb.parse_and_eval(item['root']));count=int(gdb.parse_and_eval(item['count']));address=int(value)
            assert root<=address<root+count*item['element_size'] and (address-root)%item['element_size']==0,'argument is not a directory element'
            parameters[item['name']]=dict(root=item['root'],index=(address-root)//item['element_size'],
                bytes=bytes(inferior.read_memory(address,item['element_size'])).hex())
        elif item['kind']=='output-buffer':
            owner=frame.older()
            while owner is not None and owner.name()!=item['owner']:owner=owner.older()
            assert owner is not None,'output buffer owner frame missing'
            local=owner.read_var(item['local']);size=int(frame.read_var(item['count_name']))
            assert int(value)==int(local.address) and local.type.sizeof==item['capacity'] and 0<=size<=item['capacity'],'output buffer caller-local identity/extent differs'
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],capacity=item['capacity'],size=size)
        elif item['kind']=='object-pointer':parameters[item['name']]=dict(null=int(value)==0,address=int(value))
        elif item['kind']=='global-pointer':
            address=int(value)
            if not address:
                assert item.get('nullable');parameters[item['name']]=dict(null=True)
            else:
                names=[name for name in item['allowed_globals'] if state['global_addresses'][name]==address]
                assert len(names)==1,'argument is not the specified global address'
                parameters[item['name']]=dict(null=False,global_name=names[0])
        else:parameters[item['name']]=int(value)&0xffffffff
    state['parameters']=parameters
    state['file_calls']=file_calls
    state['mapping_calls']=mapping_calls
    state['file_reads']=file_reads
    state['directory_allocation']=directory_allocation
    state['globals']={item['name']:bytes(inferior.read_memory(gdb.parse_and_eval('&'+item['name']),item['size'])).hex() for item in data}
    memory={};strings={};handles={};mapped_views={}
    state["last_error"]=int(gdb.parse_and_eval("'win32.c'::last_error"))
    def linked(head,shape):
        visited=set()
        while head:
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
        if not head:continue
        spec=item['blob'];size=spec.get('size')
        if size is None:size=int(gdb.parse_and_eval(spec['count']))*spec['element_size']
        assert 0<=size<=65536
        raw=bytes(inferior.read_memory(head,size));memory[str(head)]=raw.hex()
        for pointer in spec.get('pointers',[]):
            offset=pointer['offset'];child=int.from_bytes(raw[offset:offset+4],'little')
            if child:memory[str(child)]=bytes(inferior.read_memory(child,pointer['size'])).hex()
    state['memory']=memory
    state['strings']=strings
    state['file_handles']=handles
    state['mapped_views']=mapped_views
    open(output,'w').write(json.dumps(state,indent=2)+'\n')
