"""GDB-side typed state capture, copied into each raw native evidence directory."""
import json
import gdb


def capture(configuration,data_file,output):
    config=json.loads(open(configuration).read());data=json.loads(open(data_file).read())
    frame=gdb.newest_frame();inferior=gdb.selected_inferior()
    state=dict(function=frame.name(),pc=int(gdb.parse_and_eval('$pc')),signal=gdb.execute('info program',to_string=True))
    record=next((row for row in config['functions'] if row['native_symbol']==frame.name()),None)
    parameters={}
    for item in (record or {}).get('parameters',[]):
        value=frame.read_var(item['name'])
        if item['kind']=='string':parameters[item['name']]=dict(null=int(value)==0,bytes=(value.string(encoding='latin1').encode('latin1')+b'\0').hex() if int(value) else None)
        elif item['kind']=='argv':parameters[item['name']]=[(value[i].string(encoding='latin1').encode('latin1')+b'\0').hex() for i in range(int(frame.read_var('argc')))]
        else:parameters[item['name']]=int(value)&0xffffffff
    state['parameters']=parameters
    state['globals']={item['name']:bytes(inferior.read_memory(gdb.parse_and_eval('&'+item['name']),item['size'])).hex() for item in data}
    memory={}
    def linked(head,shape):
        visited=set()
        while head:
            if head in visited:return
            assert len(visited)<128,'unbounded target object graph'
            visited.add(head)
            raw=bytes(inferior.read_memory(head,shape['size']));memory[str(head)]=raw.hex()
            head=int.from_bytes(raw[shape['link']:shape['link']+4],'little')
    for item in data:
        if 'shape' not in item:continue
        shape=item['shape'];head=int.from_bytes(bytes.fromhex(state['globals'][item['name']]),'little')
        if not head:continue
        if 'count' not in shape:linked(head,shape)
        else:
            count=int(gdb.parse_and_eval(shape['count']));assert 0<=count<=128
            raw=bytes(inferior.read_memory(head,count*4));memory[str(head)]=raw.hex()
            for i in range(count):linked(int.from_bytes(raw[i*4:i*4+4],'little'),shape)
    state['memory']=memory
    open(output,'w').write(json.dumps(state,indent=2)+'\n')
