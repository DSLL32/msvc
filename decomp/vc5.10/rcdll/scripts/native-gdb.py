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
            pointer=int(frame.older().read_var('new_paths'))&0xffffffff
            arguments['new_paths']=pointer
            arguments['new_payload']=bytes(gdb.selected_inferior().read_memory(pointer-8,528)).hex()
        if name=='rc_allocate':arguments['bytes']=int(frame.read_var('bytes'))
        if name=='rc_is_option':arguments['value']=int(frame.read_var('value'))
        if name=='rc_emit':
            message=frame.read_var('message');length=len(message.string())+1
            arguments['message']=bytes(gdb.selected_inferior().read_memory(int(message),length)).hex()
        if name=='rc_message':arguments['id']=int(frame.read_var('id'))
        if name=='rc_prepare_input':arguments['path']=frame.read_var('path').string()
        if name in ['rc_preprocess','rc_widen_arguments','rc_preprocessor_main','rc_next_argument']:
            caller=frame.older()if name=='rc_next_argument'else frame
            count=int(caller.read_var('count'));preprocess_vector=caller.read_var('arguments');assert 0<count<=256
            wide=name in ['rc_preprocessor_main','rc_next_argument']
            arguments.update(count=count,argv=[]if wide else[preprocess_vector[i].string()for i in range(count)],prepared_lines=int(gdb.parse_and_eval('rc_prepared_lines')),current_line=int(gdb.parse_and_eval('rc_preparing_line')),filename=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('rc_filename')),514)).hex())
            if name=='rc_preprocess':arguments['unused']=int(frame.read_var('unused'))
            if wide:
                wide_vector=[]
                for index in range(count):
                    pointer=int(preprocess_vector[index]);data=bytearray()
                    for offset in range(4177):
                        word=bytes(gdb.selected_inferior().read_memory(pointer+2*offset,2));data.extend(word)
                        if word==b'\0\0':break
                    else:raise AssertionError('unterminated wide argument')
                    wide_vector.append(dict(pointer=pointer,bytes=data.hex()))
                arguments.update(wide_vector=wide_vector,wide_terminator=int(preprocess_vector[count]),vector_pointer=int(preprocess_vector))
                arguments['argv']=[bytes.fromhex(value['bytes']).decode('utf-16le')[:-1]for value in wide_vector]
            frees=json.loads((output.parent/'locals.json').read_text());assert frees[-1]['operation']=='free'
            # Revisit the actual FILE pointer observed at the first line read;
            # descriptor numbers and CRT array slots need not equal Wine's.
            file=frees[0]['input_file']
            arguments['closed_input']=bytes(gdb.selected_inferior().read_memory(file,32)).hex()
            if name=='rc_next_argument':
                from importlib.machinery import SourceFileLoader
                observer=SourceFileLoader('native_preprocess',str(Path(os.environ['RCDLL_NATIVE_SCRIPT']).parent/'native-preprocess.py')).load_module()
                arguments['preprocessor']=observer.capture_preprocessor(config_path.parent/'preprocess-data.json')
        if name=='rc_notify_progress':
            parent=frame.older();assert parent.name()=='rc_prepare_input'
            arguments.update(kind=int(frame.read_var('kind')),value=int(frame.read_var('value')),current_line=int(gdb.parse_and_eval('rc_preparing_line')),towupper=json.loads((output.parent/'towupper.json').read_text()),prefix_results=json.loads((output.parent/'prefix-results.json').read_text()),line_format=json.loads((output.parent/'line-format.json').read_text()))
            for label,variable,reading in [('input','input',True),('output','output',False)]:
                file=parent.read_var(variable);state=file.dereference();start=int(state['_base']);size=int(state['_ptr'])-start+(int(state['_cnt'])if reading else 0);assert 0<=size<=4096
                arguments[label+'_state']=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex()
                arguments[label+'_buffer']=bytes(gdb.selected_inferior().read_memory(start,size)).hex()
            pointer=int(parent.read_var('line').address);line=bytearray()
            for index in range(4177):
                word=bytes(gdb.selected_inferior().read_memory(pointer+2*index,2));line.extend(word)
                if word==b'\0\0':break
            else:raise AssertionError('unterminated progress source line')
            arguments['line']=line.hex()
        if name=='rc_prefix_equal':
            def wide(pointer):
                result=bytearray()
                for i in range(4177):
                    word=bytes(gdb.selected_inferior().read_memory(int(pointer)+2*i,2));result.extend(word)
                    if word==b'\0\0':return result.hex()
                raise AssertionError('unterminated native wide string')
            parent=frame.older();assert parent.name()=='rc_prepare_input'
            file=parent.read_var('input');state=file.dereference();start=int(state['_base']);size=int(state['_ptr'])-start+int(state['_cnt']);assert 0<size<=4096
            arguments.update(file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),file_buffer=bytes(gdb.selected_inferior().read_memory(start,size)).hex())
            arguments.update(prefix=wide(frame.read_var('prefix')),text=wide(frame.read_var('text')),current_line=int(gdb.parse_and_eval('rc_preparing_line')),locals=json.loads((output.parent/'locals.json').read_text()),ctype=json.loads((output.parent/'ctype.json').read_text()))
        if name=='rc_read_wide_line':
            file=frame.read_var('file')
            arguments.update(capacity=int(frame.read_var('capacity')),wide=int(frame.read_var('wide')),file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),cpinfo=json.loads((output.parent/'cpinfo.json').read_text()),unicode_results=json.loads((output.parent/'unicode-results.json').read_text()))
        if name=='rc_is_text_unicode':
            sample=frame.read_var('sample');size=int(frame.read_var('bytes'));flags=frame.read_var('flags')
            parent=frame.older();assert parent.name()=='rc_detect_encoding'
            file=parent.read_var('file');state=file.dereference();start=int(state['_base']);length=int(state['_ptr'])-start+int(state['_cnt']);assert 0<=length<=4096
            arguments.update(sample=bytes(gdb.selected_inferior().read_memory(int(sample),size+2)).hex(),bytes=size,flags=int(flags.dereference()),position=int(parent.read_var('position')),file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),file_buffer=bytes(gdb.selected_inferior().read_memory(start,length)).hex())
        if name=='rc_detect_encoding':
            file=frame.read_var('file');parent=frame.older();assert parent.name()=='rc_prepare_input'
            output_file=parent.read_var('output');state=output_file.dereference()
            start=int(state['_base']);length=int(state['_ptr'])-start;assert 0<=length<=8196
            arguments.update(input_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex(),output_state=bytes(gdb.selected_inferior().read_memory(int(output_file),32)).hex(),output_buffer=bytes(gdb.selected_inferior().read_memory(start,length)).hex(),write_returns=json.loads((output.parent/'writes.json').read_text()))
        if name=='rc_write_bytes':
            file=frame.read_var('file');data=frame.read_var('data');size=int(frame.read_var('bytes'))
            assert 0<size<=8196
            arguments.update(file=int(file),data=int(data),bytes=size,payload=bytes(gdb.selected_inferior().read_memory(int(data),size)).hex(),file_state=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex())
            parent=frame.older();assert parent.name()=='rc_prepare_input'
            sp=registers['esp'];bp=int(parent.read_register('ebp'))&0xffffffff
            gdb.execute('printf "RCDLL_PARENT_FRAME=%08x %08x %08x\\n", '+str(int(parent.pc())&0xffffffff)+', '+str(sp)+', '+str(bp))
            objects=[]
            for field,size in [('escaped',520),('directive',8196),('line',8354)]:
                value=parent.read_var(field);address=int(value.address)&0xffffffff
                assert value.type.sizeof==size and sp<=address<address+size<=bp
                objects.append(dict(name=field,address=address,bytes=size))
                gdb.execute('printf "RCDLL_FRAME_OBJECT '+field+'=%08x %u\\n", '+str(address)+', '+str(size))
            maps=Path('/proc/%d/maps'%gdb.selected_inferior().pid).read_text().splitlines()
            stack=[line for line in maps if line.endswith('[stack]')];assert len(stack)==1
            low,high=[int(value,16)for value in stack[0].split()[0].split('-')];assert low<=sp<bp<high
            ordered=sorted(objects,key=lambda value:value['address'])
            assert all(a['address']+a['bytes']<=b['address']for a,b in zip(ordered,ordered[1:]))
            descriptor=int(file.dereference()['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            arguments.update(parent_frame=dict(caller=parent.name(),pc=int(parent.pc())&0xffffffff,esp=sp,ebp=bp,objects=objects,stack_mapping=stack[0]),file_host_path=os.readlink('/proc/%d/fd/%d'%(gdb.selected_inferior().pid,handle)),file_fdinfo=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text(),filename=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('rc_filename')),514)).hex())
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
        environment=[];environment_vector=gdb.parse_and_eval('fixture_target_environment')
        if int(environment_vector):
            for index in range(16385):
                if not int(environment_vector[index]):break
                environment.append(environment_vector[index].string())
            else:raise AssertionError('unterminated target environment')
        case_inputs=dict(environment=environment,host_cwd=gdb.parse_and_eval('fixture_host_cwd').string(),include=gdb.parse_and_eval('fixture_input_include').string()if int(gdb.parse_and_eval('fixture_input_include'))else None,codepage=gdb.parse_and_eval('fixture_input_codepage').string()if int(gdb.parse_and_eval('fixture_input_codepage'))else None)
        rich={}
        if name=='rc_prepare_input':
            rich['buffers']={}
            for field,size in [('rc_output_path',260),('rc_input_path',260),('rc_input_name',260),('rc_preprocessed_path',260),('rc_notice_text',520)]:
                value=gdb.parse_and_eval(field)
                rich['buffers'][field]=bytes(gdb.selected_inferior().read_memory(int(value.address),size)).hex()
            rich['temporaries']={field:gdb.parse_and_eval(field).string()for field in ['rc_input_temporary','rc_output_temporary']}
            count=int(frame.older().read_var('argument_count'));assert 0<count<=256
            rich['preprocessor_argv']=[gdb.parse_and_eval('rc_preprocessor_arguments[%d]'%index).string()for index in range(count)]
            file=gdb.parse_and_eval('rc_output_file');rich['output_file']=bytes(gdb.selected_inferior().read_memory(int(file),32)).hex()
            descriptor=int(file.dereference()['_file']);handle=int(gdb.parse_and_eval('recon_osfhnd[%d]'%descriptor))-1
            rich['output_file_host_path']=os.readlink('/proc/%d/fd/%d'%(gdb.selected_inferior().pid,handle))
            rich['output_file_fdinfo']=Path('/proc/%d/fdinfo/%d'%(gdb.selected_inferior().pid,handle)).read_text()
        result=dict(rich=rich,case_inputs=case_inputs,export_vector=vector,heap_identity=heap_identity,heap=heap,diagnostic_callback=int(gdb.parse_and_eval("(void *)fixture_diagnostic"))&0xffffffff,jump=jump,exit_array=exit_array,allocations=json.loads((output.parent/'allocations.json').read_text()),module=module,globals=globals,kind='completion' if complete else 'frontier',function=raw_name,rva=f['start'],raw_pc=pc,executed_byte=instruction.hex(),registers=registers,arguments=arguments,config_sha256=hashlib.sha256(config_path.read_bytes()).hexdigest())
    except Exception as error:
        result=dict(kind='error',error=str(error))
    output.write_text(json.dumps(result,indent=2)+'\n')

if gdb.selected_inferior().pid:
    capture()
else:
    assert output.exists() and json.loads(output.read_text())["kind"]=="completion", "inferior exited without a completion state"
