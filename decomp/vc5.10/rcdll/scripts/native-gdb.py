"""Loaded only by native GDB; raw trap-derived frontier, no Wine dependency."""
import gdb
import hashlib
import json
import os
import sys
from pathlib import Path
output=Path(os.environ['RCDLL_NATIVE_SNAPSHOT'])
config_path=Path(os.environ['RCDLL_NATIVE_CONFIG'])
sys.path.insert(0,str(Path(os.environ['RCDLL_NATIVE_SCRIPT']).parent))
from protocol import read_configuration,logical_name
config=read_configuration(config_path)
def capture():
    try:
        frame=gdb.newest_frame()
        raw_name=frame.name();name=logical_name(raw_name)
        matches=[f for f in config['functions'] if f['name']==name]
        pc=int(gdb.parse_and_eval('$eip'))
        instruction=bytes(gdb.selected_inferior().read_memory(pc-1,1))
        complete=name=='fixture_complete'
        if not complete:
            assert instruction==b'\xcc','stop is not an executed INT3'
            assert len(matches)==1,'trap has no unique indexed logical function'
        f=dict(start=None) if complete else matches[0]
        gdb.execute('printf "RCDLL_PC=%08x\\n", $pc')
        gdb.execute('bt')
        gdb.execute('info address '+raw_name)
        registers={n:int(gdb.parse_and_eval('$'+n))&0xffffffff for n in ['eip','esp','ebp','eax','ecx','edx','ebx','esi','edi']}
        arguments={'result':int(frame.read_var('result'))} if complete else {}
        if name in ['_DllMainCRTStartup','DllMain','_CRT_INIT']:
            arguments={n:int(frame.read_var(n))&0xffffffff for n in ['instance','reason','reserved']}
        globals={}
        for item in json.loads((config_path.parent/'data.json').read_text()):
            value=gdb.parse_and_eval(item['native_expression'])
            globals[item['name']]=bytes(gdb.selected_inferior().read_memory(int(value.address),item['bytes'])).hex()
        identity=gdb.parse_and_eval('fixture_registered_identity').dereference()
        module=dict(token=int(gdb.parse_and_eval('fixture_module'))&0xffffffff,
            name=identity['name'].string(),path=identity['path'].string(),sha256=identity['sha256'].string(),flags=int(identity['flags']))
        exit_base=int(gdb.parse_and_eval('dll_exit_base'))&0xffffffff
        exit_end=int(gdb.parse_and_eval('dll_exit_end'))&0xffffffff
        exit_array=dict(base=exit_base,end=exit_end)
        if exit_base:exit_array['first']=bytes(gdb.selected_inferior().read_memory(exit_base,4)).hex()
        if name=='RC':
            arguments={n:int(frame.read_var(n))&0xffffffff for n in ['window','unused','diagnostic','resource','count']}
            vector=frame.read_var('arguments');arguments['argv']=[vector[n].string()for n in range(arguments['count'])]
            arguments['terminator']=int(vector[arguments['count']])
            arguments['host_callback']=int(gdb.parse_and_eval('(void *)fixture_diagnostic'))&0xffffffff
        if name=='main':
            arguments['count']=int(frame.read_var('count'))
            vector=frame.read_var('arguments');arguments['argv']=[vector[n].string()for n in range(arguments['count'])]
            arguments['terminator']=int(vector[arguments['count']])
            arguments['host_callback']=int(gdb.parse_and_eval('(void *)fixture_diagnostic'))&0xffffffff
        jump=None
        if any(f['name']=='main' and not f.get('stub')for f in config['functions']):
            cursor=gdb.parse_and_eval('jump_contexts')
            legacy=int(gdb.parse_and_eval('&rc_abort_context'))
            while int(cursor):
                entry=cursor.dereference()
                if int(entry['legacy'])==legacy:
                    jump=dict(legacy=legacy,opaque_setjmp3=int(entry['opaque_setjmp3']),host_bytes=bytes(gdb.selected_inferior().read_memory(int(entry['host'].address),entry['host'].type.sizeof)).hex())
                    break
                cursor=entry['next']
        vector=[]
        for i in range(int(gdb.parse_and_eval('fixture_argument_count'))):
            pointer=int(gdb.parse_and_eval('fixture_argument_vector[%d]'%i))&0xffffffff
            storage=int(gdb.parse_and_eval('fixture_argument_storage[%d]'%i))&0xffffffff
            size=int(gdb.parse_and_eval('fixture_argument_lengths[%d]'%i))
            raw=bytes(gdb.selected_inferior().read_memory(storage,size)).hex()
            vector.append(dict(pointer=pointer,storage=storage,bytes=raw,text=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string()))
        if name=='rc_release':
            pointer=int(frame.read_var('block'))&0xffffffff
            arguments['block']=pointer
            arguments['payload']=bytes(gdb.selected_inferior().read_memory(pointer-8,528)).hex()
            arguments['include']=gdb.parse_and_eval('rc_include_paths').string()
        if name=='rc_allocate':arguments['bytes']=int(frame.read_var('bytes'))
        if name=='rc_is_option':arguments['value']=int(frame.read_var('value'))
        if name=='rc_emit':
            message=frame.read_var('message');length=len(message.string())+1
            arguments['message']=bytes(gdb.selected_inferior().read_memory(int(message),length)).hex()
        if name=='rc_message':arguments['id']=int(frame.read_var('id'))
        if name=='rc_format':
            arguments['capacity']=int(frame.read_var('capacity'))
            template=frame.read_var('format').cast(gdb.lookup_type('char').pointer())
            arguments['template']=template.string()
            base=int(gdb.parse_and_eval('$ebp'))
            version=int.from_bytes(bytes(gdb.selected_inferior().read_memory(base+20,4)),'little')
            arguments['version']=gdb.Value(version).cast(gdb.lookup_type('char').pointer()).string()
            arguments['build']=int.from_bytes(bytes(gdb.selected_inferior().read_memory(base+24,4)),'little')
            arguments['output']=int(frame.read_var('output'))&0xffffffff
            arguments['expected_output']=int(gdb.parse_and_eval('&rc_error_text'))&0xffffffff
        module['thread_calls_disabled']=bool(gdb.parse_and_eval('fixture_thread_calls_disabled'))
        heap_identity=None
        token=int(gdb.parse_and_eval('rc_heap'))&0xffffffff
        if token:
            cursor=gdb.parse_and_eval('native_heaps')
            while int(cursor):
                if int(cursor)==token:
                    item=cursor.dereference();blocks=[];block=item['blocks']
                    while int(block):
                        record=block.dereference();size=int(record['requested_size'])
                        blocks.append(dict(owner=int(record['owner'])&0xffffffff,payload=int(record['payload'])&0xffffffff,bytes=size,flags=int(record['flags']),contents=bytes(gdb.selected_inferior().read_memory(int(record['payload']),size)).hex()))
                        block=record['next']
                    heap_identity=dict(handle=token,options=int(item['options']),initial=int(item['initial_size']),maximum=int(item['maximum_size']),blocks=blocks)
                    break
                cursor=cursor.dereference()['next']
        heap=json.loads((output.parent/'heap.json').read_text()) if (output.parent/'heap.json').exists() else []
        result=dict(export_vector=vector,heap_identity=heap_identity,heap=heap,diagnostic_callback=int(gdb.parse_and_eval("(void *)fixture_diagnostic"))&0xffffffff,jump=jump,exit_array=exit_array,allocations=json.loads((output.parent/'allocations.json').read_text()),module=module,globals=globals,kind='completion' if complete else 'frontier',function=raw_name,rva=f['start'],raw_pc=pc,executed_byte=instruction.hex(),registers=registers,arguments=arguments,config_sha256=hashlib.sha256(config_path.read_bytes()).hexdigest())
    except Exception as error:
        result=dict(kind='error',error=str(error))
    output.write_text(json.dumps(result,indent=2)+'\n')

if gdb.selected_inferior().pid:
    capture()
else:
    assert output.exists() and json.loads(output.read_text())["kind"]=="completion", "inferior exited without a completion state"
