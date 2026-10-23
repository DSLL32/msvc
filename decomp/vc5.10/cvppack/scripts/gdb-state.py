"""GDB-side typed state capture, copied into each raw native evidence directory."""
import json
import os
import stat
import hashlib
from pathlib import Path
import gdb


file_calls=[]
def install():
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
    state['globals']={item['name']:bytes(inferior.read_memory(gdb.parse_and_eval('&'+item['name']),item['size'])).hex() for item in data}
    memory={};strings={};handles={}
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
    state['memory']=memory
    state['strings']=strings
    state['file_handles']=handles
    open(output,'w').write(json.dumps(state,indent=2)+'\n')
