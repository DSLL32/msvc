"""Typed application-frontier state from debugger memory, never executable helpers.

Readers record every addressed byte range. Re-decoding the immutable transcript,
not trusting a parallel JSON projection, is the acceptance check.
"""
import hashlib
import json
from pathlib import Path
import struct

PROJECT=Path(__file__).resolve().parents[1]
PROJECTION='cl-application-frontier-v1: typed parameters and reconstructed driver state'
SCALARS={'dll_options_enabled':0x40902c,'default_macros':0x409028,'response_depth':0x409680,'response_first_line':0x409034,'no_logo':0x409660,'echo_response':0x40963c,'help_requested':0x40965c,
 'cleanup_temporaries':0x40901c,'is_dll':0x409658,'first_option_pass':0x409030,'map_requested':0x40960c,'bk_requested':0x409668,'reproducible':0x409604,'timing_requested':0x40964c,'bz_requested':0x4095c4,'minimal_rebuild':0x40966c,'allow_pch':0x409670,'keep_preprocessor_output':0x409024,'use_standard_include':0x409010,'source_count':0x409630,'options_after_link':0x409678,'link_warning':0x40967c,'show_commands':0x409014,'message_capacity':0x4091a8,'debug_exceptions':0x4095c8,'error_count':0x409650,
 'output_produced':0x4095cc,'default_c_language':0x4095d0,'first_dependency_scan':0x409018,'first_stage_notice':0x40918c}
POINTERS={'object_name':0x4095c0,'image_name':0x409644,'ir_prefix':0x4095f8,'map_name':0x409638,'pdb_name':0x4095bc,'browser_name':0x409648,'free_options':0x409674,'driver':0x409020,'unrecognized_options':0x409634,'source_language':0x4095b8,'format_buffer':0x4097a4}
PARAMETERS={
 'install_control_handler':[], 'skip_space':[('text','string')],
 'split_arguments':[('text','string'),('mode','signed')],
 'count_arguments':[('args','vector')],
 'duplicate':[('text','string')],
 'program_name':[('name','string')], 'program_directory':[('name','string')],
 'initialize_driver':[('name','string'),('state','driver')],
 'show_usage':[], 'has_nologo':[('args','vector'),('count','signed')],
 'show_banner':[('force','signed')], 'load_string':[('id','unsigned')], 'parse_arguments':[('args','vector'),('count','signed')],
 'write_message':[('fd','signed'),('text','string')],
 'write_link_message':[('stream','response_file'),('text','string')], 'finish_options':[],
 'final_pass':[('state','driver')],
 'link_pass':[('state','driver')],
 'write_link_response':[('state','driver')],
 'execute_pass':[('mode','signed'),('pass','pass'),('name','string'),('environment','string'),('args','vector')],
 'unload_pass':[('pass','pass')],
 'cleanup_files':[('preserve','signed')],
 'delete_temporary':[('kind','signed')],
 'quoted_length':[('text','string')],
 'quote_argument':[('out','output_buffer'),('text','string')],
 'expanded_length':[('text','string'),('state','driver')],
 'configure_pass':[('state','driver')],
 'find_file':[('directory','string'),('name','string'),('found','output_unsigned')],
 'search_path':[('name','string'),('out','output_buffer'),('capacity','signed')],
 'prepare_inputs':[('state','driver')], 'prepare_environment':[('state','driver')],
 'format_message':[('length','output_unsigned'),('text','string'),('args','format_arguments')],
 'read_response':[('name','string')], 'classify_input':[('name','string')],
 'create_input':[('name','string'),('kind','signed')],
 'add_input':[('name','string'),('kind','signed'),('explicit_kind','signed')],
 'has_prefix':[('prefix','string'),('text','string')], 'find_special':[('name','string')], 'apply_special':[('special','special'),('argument','string')],
 'match_option':[('args','vector'),('index','index_pointer')],
 'find_rule':[('args','vector'),('index','signed')],
 'match_pattern':[('rule','rule'),('args','vector'),('index','signed')],
 'read_option':[('rule','rule'),('args','vector'),('index','index_pointer')],
 'new_option':[('name','string'),('value','string'),('matching','string'),('flags','unsigned')],
 'validate_value':[('name','string'),('value','string'),('constraints','string')],
 'append_option':[('head','option_head'),('option','option')],
 'expand_options':[('pattern','string'),('context','option')],
 'expand_word':[('pattern','string'),('out','output_buffer'),('replacement','string')],
 'append_literal':[('out','string'),('text','string')],
 'append_escaped':[('out','string'),('text','string')],
 'detach_options':[('head','option_head'),('patterns','options'),('defaults','optional_option_head')],
 'delete_options':[('options','options')],
 'options_match':[('option','option'),('pattern','option')],
 'format_number':[('value','unsigned'),('out','output_buffer'),('base','unsigned')],
 'delete_option':[('option','option')],
 'expand_alternative':[('pattern','string'),('out','output_buffer'),('replacement','string'),('alternative','output_buffer')],
 'apply_extension':[('pattern','string'),('filename','string'),('alternative','string')],
 'find_last_separator':[('text','string'),('separators','string')],
 'unlink_option':[('head','option_head'),('option','option')],
 'copy_option':[('option','option')],
 'option_conflict':[('id','unsigned'),('old','option'),('replacement','option')],
 'define_macro':[('args','vector'),('index','index_pointer')],
 'link_arguments':[('args','vector'),('index','index_pointer')],
 'c_input':[('args','vector'),('index','index_pointer')],
 'cpp_input':[('args','vector'),('index','index_pointer')],
 'object_input':[('args','vector'),('index','index_pointer')],
 'typed_input':[('option','string'),('args','vector'),('index','index_pointer'),('kind','signed')],
 'undefine_macro':[('option','option')],
 'configure_assembly':[('option','option')],
 'enable_pass':[('option','option')],
 'toggle_pass':[('option','option')],
 'disable_pass':[('option','option')],
 'set_output_option':[('option','option')],
 'set_pass_suffix':[('option','option')],
 'set_pass_name':[('option','option')],
 'undefine_defaults':[('option','option')],
 'enable_dll':[('option','option')],
 'has_any_option':[('options','options'),('patterns','options')],
 'option_for_pass':[('option','option'),('pass','pass')],
 'expand_filename':[('out','output_buffer'),('text','string'),('state','driver')],
 'file_basename':[('out','output_buffer'),('name','string')],
 'file_name':[('out','output_buffer'),('name','string')],
 'file_directory':[('out','output_buffer'),('name','string')],
 'make_map_name':[('out','output_buffer'),('name','string')],
 'vwrite_message':[('fd','signed'),('text','string'),('args','diagnostic_arguments')],
 'warning':[('id','unsigned')], 'fatal':[('id','unsigned')],
 'show_help':[], 'allocate':[('bytes','unsigned')],
 'append_text':[('out','output_buffer'),('text','string')],
 'join_text':[('left','string'),('right','string')],
 'release':[('memory','owned_string')], 'prepare_execution':[],
 'run_driver':[('state','driver')], 'source_jobs':[('state','driver')],
 'create_job':[('input','inputs')], 'advance_jobs':[('jobs','jobs')],
 'prepare_jobs':[('state','driver'),('jobs','jobs'),('pending','output_buffer')],
 'run_jobs':[('state','driver'),('jobs','jobs')],
 'run_passes':[('passes','pass_sequence'),('state','driver'),('callback','callback')],
 'next_jobs':[('state','driver'),('pending','unread_pending')],
 'driver_exit':[('status','signed')]
}

def format_types(text):
    result=[];i=0
    while i<len(text):
        if text[i]=='%' and i+1<len(text):
            i+=1
            if text[i]=='s':result.append('string')
            elif text[i] in 'dcx':result.append('unsigned')
        i+=1
    return result

class Memory:
    def __init__(self,read,rule_base,pass_base=None):
        self.read=read;self.rule_base=rule_base;self.pass_base=pass_base
        self.option_ids={};self.input_ids={};self.job_ids={}
        self.word_reads={};self.vector_slots={}
    def words(self,address,count):
        value=struct.unpack('<%dI'%count,self.read(address,4*count));key=(address,count)
        assert key not in self.word_reads or self.word_reads[key]==value,'stopped memory changed between reads'
        self.word_reads[key]=value;return value
    def string(self,address):
        if not address:return None
        data=bytearray()
        for i in range(32768):
            c=self.read(address+i,1)
            if c==b'\0':return data.decode('latin1')
            data+=c
        raise AssertionError('unterminated frontier string')
    def vector(self,address):
        if not address:return None
        result=[];self.vector_slots[address]=[]
        for i in range(4096):
            pointer=self.words(address+4*i,1)[0]
            if not pointer:return result
            self.vector_slots[address].append(pointer)
            result.append(self.string(pointer))
        raise AssertionError('unterminated frontier vector')
    def rule(self,address):
        assert self.rule_base<=address<self.rule_base+174*20 and (address-self.rule_base)%20==0,'unknown grammar record'
        words=self.words(address,5);strings=[self.string(v)for v in words[:4]]
        extra=('callback' if '@' in strings[0] else self.string(words[4])) if words[4] else None
        return {'index':(address-self.rule_base)//20,'pattern':strings[0],'add':strings[1],
            'conflict':strings[2],'replace':strings[3],'extra':extra}
    def option(self,address):
        if not address:return None
        if address in self.option_ids:return {'ref':self.option_ids[address]}
        identity=len(self.option_ids)+1;self.option_ids[address]=identity
        v=self.words(address,6);origin=v[5]
        if origin and self.rule_base<=origin<self.rule_base+174*20:source={'rule':self.rule(origin)}
        else:source=self.option(origin)
        return {'id':identity,'flags':v[1],'matching':self.string(v[2]),'name':self.string(v[3]),
            'value':self.string(v[4]),'origin':source}
    def options(self,address):
        result=[];seen=set()
        while address:
            assert address not in seen and len(seen)<4096,'cyclic or oversized option list'
            seen.add(address);result.append(self.option(address));address=self.words(address,1)[0]
        return result
    def freelist(self,address):
        seen=set()
        while address:
            assert address not in seen and len(seen)<4096,'invalid free option list'
            seen.add(address);address=self.words(address,1)[0]
        return {'available_nodes':len(seen)}
    def inputs(self,address):
        result=[];seen=set()
        while address:
            assert address not in seen and len(seen)<4096
            seen.add(address);v=self.words(address,4)
            if address in self.input_ids:result.append({'ref':self.input_ids[address]})
            else:
                identity=len(self.input_ids)+1;self.input_ids[address]=identity
                result.append({'id':identity,'name':self.string(v[1]),'kind':v[2],'explicit_kind':v[3]&0xffff,
                    'sequence':struct.unpack('<h',struct.pack('<I',v[3])[2:])[0]})
            address=v[0]
        return result
    def jobs(self,address):
        rows=[];seen=set()
        while address:
            assert address not in seen and len(seen)<4096
            seen.add(address);v=self.words(address,3)
            if address in self.job_ids:rows.append({'ref':self.job_ids[address]})
            else:
                identity=len(self.job_ids)+1;self.job_ids[address]=identity
                rows.append({'id':identity,'input':self.inputs(v[1]),'status':v[2]})
            address=v[0]
        return rows
    def driver(self,address):
        if not address:return None
        v=self.words(address,11)
        if v[2]:assert self.pass_base<=v[2]<self.pass_base+180 and (v[2]-self.pass_base)%36==0
        assert not any(v[5:7]),'MRE deferred queue fields require an explicit producer/alias decoder'
        return {'options':self.options(v[0]),'other_options':self.options(v[1]),
            'current_pass':{'index':(v[2]-self.pass_base)//36}if v[2]else None,
            'inputs':self.inputs(v[3]),'current_jobs':self.jobs(v[4]),
            'current_input':self.inputs(v[7]),'temporary':self.string(v[8]),
            'include_path':self.string(v[9]),'directory':self.string(v[10])}
    def passes(self,address):
        rows=[]
        for i in range(5):
            v=self.words(address+36*i,9)
            module=invoke=abort=None
            if any(v[5:8]):
                identity=getattr(self,'compiler_module',None)
                assert identity is not None and i==1 and identity['provider_state']==3,'unknown or released compiler DLL/cache slots'
                assert v[5:8]==tuple(identity[name]for name in ('module','invoke','abort')),'compiler DLL/cache identities differ'
                module={'name':'C1.DLL','sha256':identity['sha256'],'state':'loaded'}
                invoke={'name':'_InvokeCompilerPass@12','ordinal':2,'stdcall_bytes':12}
                abort={'name':'_AbortCompilerPass@4','ordinal':1,'stdcall_bytes':4}
            rows.append({'name':self.string(v[0]),'path':self.string(v[1]),
                'environment':self.string(v[2]),'prefix':struct.pack('<I',v[3]).split(b'\0')[0].decode('latin1'),
                'flags':v[4],'module':module,'invoke':invoke,'abort':abort,'options':self.string(v[8])})
        return rows
    def value(self,kind,value):
        # Reached release calls in help own strdup-created option strings.
        # Other allocation shapes require a new explicit projection.
        if kind=='owned_string':return self.string(value)
        if kind=='response_file':return None if not value else {'descriptor':self.words(value+16,1)[0]}
        if kind=='string':return self.string(value)
        if kind=='vector':return self.vector(value)
        if kind=='driver':return self.driver(value)
        if kind=='inputs':return self.inputs(value)
        if kind=='jobs':return self.jobs(value)
        if kind=='pass_sequence':
            result=[]
            for i in range(6):
                item=self.words(value+4*i,1)[0];assert item<=5
                result.append(item)
                if item==5:return result
            raise AssertionError('unterminated compiler-pass sequence')
        if kind=='callback':
            names=[name for name,address in self.callbacks.items()if address==value]
            assert len(names)==1,'unknown driver callback identity'
            return names[0]
        if kind in ('output_unsigned','format_arguments','output_buffer'):
            assert value,'null required output/va_list pointer'
            self.words(value,1) # Validate addressed storage, not uninitialized output contents.
            return {'nonnull':True}
        if kind=='index_pointer':return self.words(value,1)[0]
        if kind=='pass':
            values=self.words(value,9)
            return {'name':self.string(values[0]),'path':self.string(values[1]),'environment':self.string(values[2]),'prefix':struct.pack('<I',values[3]).split(b'\0')[0].decode('latin1'),'flags':values[4]}
        if kind=='rule':return self.rule(value)
        if kind=='option':return self.option(value)
        if kind=='options':return self.options(value)
        if kind=='optional_option_head':return None if not value else self.options(self.words(value,1)[0])
        if kind=='option_head':return self.options(self.words(value,1)[0])
        if kind=='special':
            name,pattern=self.words(value,2)
            return {'name':self.string(name),'pattern':self.string(pattern)}
        if kind=='unread_pending':return {'role':'caller-local PendingWork','payload':'unconsumed: both minimal-rebuild guards are false'}
        if kind=='diagnostic_arguments':
            assert value,'null diagnostic varargs slot'
            return {'nonnull':True}
        if kind=='unsigned':return value
        if kind=='signed':return struct.unpack('<i',struct.pack('<I',value))[0]
        assert kind!='pointer','opaque pointer input requires an explicit pointee/alias contract'
        raise AssertionError('unknown typed frontier input '+kind)


def snapshot(read,context):
    memory=Memory(read,context['rule_base'],context.get('compiler_passes_address'));memory.callbacks=context.get('callbacks',{});symbol=context['symbol'];parameters=PARAMETERS[symbol]
    memory.compiler_module=context.get('compiler_module')
    assert len(parameters)==len(context['arguments'])
    fields={'frontier':symbol,'parameters':{name:memory.value(kind,value)
        for (name,kind),value in zip(parameters,context['arguments'])}}
    if symbol=='append_text' and not context['arguments'][1]:
        fields['parameters']['out']=memory.string(context['arguments'][0])
    if symbol=='vwrite_message':
        caller=context['diagnostic_caller']
        assert context['arguments'][2]==caller['args_address'],'diagnostic va_list is not caller firstvariadic slot'
        assert caller['name']in ('warning','fatal')
        types=format_types(fields['parameters']['text'])
        values=memory.words(context['arguments'][2],len(types))if types else ()
        fields['variable_arguments']=[memory.value(kind,value)for kind,value in zip(types,values)]
        fields['diagnostic_caller']={'name':caller['name'],'id':caller['id'],'first_variadic_slot_alias':True}
    if symbol in ('warning','fatal'):
        types=format_types(context['diagnostic_format']);values=context['varargs']
        assert len(types)==len(values)
        fields['variable_arguments']=[memory.value(kind,value)for kind,value in zip(types,values)]
    if symbol in ('write_message','write_link_message','format_message'):
        types=format_types(memory.string(context['arguments'][1]))
        if symbol=='format_message':values=memory.words(context['arguments'][2],len(types))
        else:values=context['varargs']
        assert len(types)<=len(values)
        fields['variable_arguments']=[memory.value(kind,value) for kind,value in zip(types,values)]
    if 'crt_mb' in context:fields['crt_mb']=context['crt_mb']
    fields['globals']={name:memory.words(context['globals'][name],1)[0] for name in SCALARS}
    for name in POINTERS:
        pointer=memory.words(context['globals'][name],1)[0]
        if name=='driver':driver_address=pointer
        fields['globals'][name]={'object_name':memory.string,'image_name':memory.string,'ir_prefix':memory.string,'map_name':memory.string,'pdb_name':memory.string,'browser_name':memory.string,'free_options':memory.freelist,'driver':memory.driver,'unrecognized_options':memory.options,
            'source_language':memory.string,'format_buffer':memory.string}[name](pointer)
    assert context['globals']['ir_prefix']==driver_address+32,'driver temporary/IR-prefix storage alias differs'
    if 'compiler_passes_address' in context:
        fields['compiler_passes']=memory.passes(context['compiler_passes_address'])
    if symbol in ('unload_pass','cleanup_files','delete_temporary','driver_exit','next_jobs'):
        base=context['compiler_passes_address'];index=1 if symbol=='unload_pass' else 3
        record=base+36*index
        if symbol=='unload_pass':assert context['arguments'][0]==record,'unload argument is not C1 pass'
        elif symbol=='cleanup_files':assert context['arguments']==[1],'unknown successful cleanup argument'
        elif symbol=='next_jobs':
            assert context['arguments']==[driver_address,context['pending_address']],'next_jobs formal/caller-local alias differs'
            assert fields['globals']['minimal_rebuild']==0 and fields['globals']['first_dependency_scan']==0,'PendingWork is consumed outside this captured guard'
        else:assert context['arguments']==[0],'unknown helper/normal-exit argument'
        assert memory.word_reads[(driver_address,11)][2]==record,'pass/current-driver alias differs'
        identity=context['compiler_module']
        assert identity['sha256']=='7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d'
        assert identity['provider_state']==(3 if index==1 else 5),'dependency module lifetime differs'
        addresses=context['execution_addresses']
        interrupted,active,running=[memory.words(addresses[name],1)[0]for name in ('interrupted','active_pass','pass_running')]
        assert interrupted==0 and active==record and running==0,'post-Invoke execution state differs'
        fields['execution_aliases']={'pass_index':index,'pass_is_current':True,'active_pass_is_current':True,
                                    'interrupted':interrupted,'pass_running':running}
        fields['dependency_crt']=dependency_crt(memory,context['crt_addresses'],after_child=index==3)
    if symbol=='delete_temporary':
        rows=[]
        for index in range(8):
            pointer,per_input=memory.words(context['temporary_suffixes_address']+8*index,2)
            if index==7:
                assert (pointer,per_input)==(0,0),'cleanup suffix sentinel differs'
            else:rows.append({'suffix':memory.string(pointer),'per_input':per_input})
        assert rows==[{'suffix':name,'per_input':int(name!='lk')}for name in ('ex','sy','gl','in','st','db','lk')]
        fields['temporary_suffixes']=rows
    if symbol=='execute_pass':
        mode,record,name,environment,argv=context['arguments']
        base=context['compiler_passes_address'];assert base<=record<base+180 and (record-base)%36==0
        driver=memory.word_reads[(driver_address,11)];passed=memory.word_reads[(record,9)]
        assert driver[2]==record and name==passed[0] and environment==passed[2],'pass/name/environment argument aliases differ'
        strings=fields['parameters']['args'];slots=memory.vector_slots[argv]
        assert strings and len(slots)==len(strings)
        live=b''.join(text.encode('latin1')+b'\0'for text in strings)
        offset=0;offsets=[]
        for pointer,text in zip(slots,strings):
            assert pointer==slots[0]+offset,'argument strings do not occupy the caller storage contiguously'
            offsets.append(offset);offset+=len(text.encode('latin1'))+1
        assert argv+4*(len(slots)+1)<=slots[0] or slots[0]+len(live)<=argv,'argv table overlaps string storage'
        fields['execution_aliases']={'pass_index':(record-base)//36,'pass_is_current':True,
            'name_is_pass_name':True,'environment_is_pass_environment':True,
            'argc':len(strings),'argv_string_offsets':offsets,'live_storage_hex':live.hex(),
            'argv_table_disjoint_from_strings':True,'name_is_argv0':name==slots[0],
            'environment_aliases_argument':[i for i,pointer in enumerate(slots)if pointer==environment]}
    return fields


def dependency_crt(memory,addresses,after_child=False):
    scalar_names=('fmode','commode','mb_cur_max','mb_codepage','lc_codepage')
    result={name:memory.words(addresses[name],1)[0]for name in scalar_names}
    result['lc_handles']=list(memory.words(addresses['lc_handles'],6))
    result['mbctype_hex']=memory.read(addresses['mbctype'],257).hex()
    pctype=memory.words(addresses['pctype_slot'],1)[0]
    assert pctype and pctype!=addresses['initial_pctype'],'dependency pctype allocation transition missing'
    result['ctype_hex']=memory.read(pctype-2,514).hex()
    current=memory.words(addresses['environment_slot'],1)[0]
    initial=memory.words(addresses['initenv_slot'],1)[0]
    assert current and initial and current!=initial,'dependency CRT vector ownership transition missing'
    result['environment']=memory.vector(current);result['initial_environment']=memory.vector(initial)
    if after_child:
        assert len(result['environment'])==len(result['initial_environment'])+1
        assert result['environment'][:-1]==result['initial_environment']
        assert result['environment'][-1].startswith('MSC_CMD_FLAGS=')
    else:assert result['environment']==result['initial_environment'],'unexpected first-Invoke namespace mutation'
    assert set(memory.vector_slots[current]).isdisjoint(memory.vector_slots[initial]),'initial environment strings were not copied'
    result['environment_vector_moved']=True;result['initial_strings_copied']=True
    os_pointer=memory.words(addresses['os_environment_slot'],1)[0]
    assert os_pointer,'unbound dependency OS namespace'
    os_bytes=bytearray();previous=None
    for i in range(65536):
        unit=memory.read(os_pointer+2*i,2);os_bytes+=unit
        if unit==previous==b'\0\0':break
        previous=unit
    else:raise AssertionError('unterminated dependency OS namespace')
    text=os_bytes.decode('utf-16le');assert '\0\0' not in text[:-2]
    result['os_environment']=text[:-2].split('\0')
    return result


def original_context(directory,address,stack):
    index=json.loads((directory/'config.json').read_text())
    record=next(r for r in index['driver'] if r['start']==address)
    symbol=record['native_symbol'];parameters=PARAMETERS[symbol]
    context={'symbol':symbol,'arguments':list(stack[1:1+len(parameters)]),
        'globals':{**SCALARS,**POINTERS},'rule_base':0x406340,'compiler_passes_address':0x4090a8}
    if symbol=='next_jobs':context['pending_address']=struct.unpack_from('<I',(directory/'context.bin').read_bytes(),180)[0]-24
    if symbol=='delete_temporary':context['temporary_suffixes_address']=0x406300
    if symbol=='run_passes':context['callbacks']={'final_pass':next(row['start']for row in index['driver']if row['name']=='_final_pass')}
    if symbol in ('write_message','write_link_message'):context['varargs']=list(stack[3:])
    if symbol=='vwrite_message':
        raw=(directory/'context.bin').read_bytes();sp=struct.unpack_from('<I',raw,196)[0]
        if stack[0]==0x4053eb:caller,slot,arg='warning',5,sp+24
        elif stack[0]==0x40539f:caller,slot,arg='fatal',6,sp+28
        else:raise AssertionError('unknown diagnostic forwarding caller')
        context['diagnostic_caller']={'name':caller,'id':stack[slot],'args_address':arg}
    if symbol in ('warning','fatal'):
        from resources import extract
        identifier=stack[1]or(2000 if symbol=='fatal'else 0)
        context['diagnostic_format']=extract(identifier)['text']
        context['varargs']=list(stack[2:2+len(format_types(context['diagnostic_format']))])
    if (directory/'mb-state.bin').exists():
        mb=(directory/'mb-state.bin').read_bytes();crt=(directory/'crt-state.bin').read_bytes()
        assert len(mb)==261 and all(not flag&4 for flag in mb[5:133]),'unreviewed ASCII lead-byte state'
        context['crt_mb']={'codepage':struct.unpack_from('<I',mb)[0],'mb_cur_max':struct.unpack_from('<I',crt,12)[0]}
    if symbol in ('unload_pass','cleanup_files','delete_temporary','driver_exit','next_jobs'):
        import re
        from binary import PE
        from dependency_packet import exports,C1_SHA
        from mktemp_state import CRT_SHA
        capture=json.loads((directory/'capture.json').read_text()) if (directory/'capture.json').exists() else None
        # During the stopped-process capture its receipt is not written yet;
        # actual loaded hFile copies and observer events are already frozen.
        if capture is not None:
            assert capture['dependency_response'] and not capture['baseline'] and not capture['dependency_analysis']
        c1_path=directory/'observer/loaded-c1.bin';crt_path=directory/'loaded-msvcrt.bin'
        assert hashlib.sha256(c1_path.read_bytes()).hexdigest()==C1_SHA
        assert hashlib.sha256(crt_path.read_bytes()).hexdigest()==CRT_SHA
        c1=PE(c1_path);crt=PE(crt_path);ce=exports(c1);crte=exports(crt)
        events=(directory/'observer/events.txt').read_text()
        loaded=re.findall(r'^c1-load event=(\d+) base=10600000 entry=1061202a entry_byte=53$',events,re.M)
        assert len(loaded)==1
        unloaded=re.findall(r'^module-unload event=(\d+) base=10600000$',events,re.M)
        assert len(unloaded)==(0 if symbol=='unload_pass' else 1)
        words=struct.unpack('<32I',(directory/'observer/frontier-crt-words.bin').read_bytes())
        assert words[0]==crt.base==0x78000000 and words[22]==c1.base==0x10600000
        assert words[21]==(0x4090cc if symbol=='unload_pass' else 0x409114)
        if symbol=='unload_pass':assert stack[1]==words[21]
        elif symbol=='next_jobs':assert stack[1]==0x4095d8 and stack[2]==context['pending_address'] and words[20]==0
        else:assert stack[1]==(1 if symbol=='cleanup_files' else 0) and words[20]==0
        context['crt_mb']={'codepage':words[6],'mb_cur_max':words[5]}
        assert ce['_InvokeCompilerPass@12']=={'ordinal':2,'rva':0x12229}
        assert ce['_AbortCompilerPass@4']=={'ordinal':1,'rva':0x43975}
        context['compiler_module']={'sha256':C1_SHA,'module':c1.base,'invoke':c1.base+0x12229,
                                    'abort':c1.base+0x43975,'provider_state':3 if symbol=='unload_pass' else 5}
        context['execution_addresses']={'interrupted':0x4097a8,'active_pass':0x4097ac,'pass_running':0x4097b0}
        addresses={name:crt.base+crte[export]['rva']for name,export in {
            'fmode':'_fmode','commode':'_commode','mb_cur_max':'__mb_cur_max',
            'lc_codepage':'__lc_codepage','lc_handles':'__lc_handle',
            'pctype_slot':'_pctype','environment_slot':'_environ'}.items()}
        addresses.update(mb_codepage=crt.base+0x3f1c0,mbctype=crt.base+0x3f1c8,
            initenv_slot=crt.base+0x3fde8,os_environment_slot=words[18]+0x48,
            initial_pctype=struct.unpack('<I',crt.read(addresses['pctype_slot'],4))[0])
        context['crt_addresses']=addresses
    return context


def capture_original(directory,address,stack,process):
    context=original_context(directory,address,stack);ranges=[]
    def read(address,length):
        process.stdin.write(('%x %d\n'%(address,length)).encode());process.stdin.flush()
        line=process.stdout.readline().decode('ascii').strip().split()
        assert len(line)==4 and line[:3]==['DATA','%08x'%address,str(length)],'original memory request failed: '+repr(line)
        data=bytes.fromhex(line[3]);assert len(data)==length
        ranges.append({'address':address,'data':data.hex()});return data
    fields=snapshot(read,context)
    raw={'context':context,'ranges':ranges}
    (directory/'frontier-memory.json').write_text(json.dumps(raw,indent=2)+'\n')
    return fields


def capture_native(directory,gdb):
    frame=gdb.newest_frame();symbol=frame.name();inferior=gdb.selected_inferior();ranges=[]
    context={'symbol':symbol,'arguments':[int(frame.read_var(name))&0xffffffff for name,kind in PARAMETERS[symbol]],
        'globals':{name:int(gdb.parse_and_eval('(unsigned)&'+('driver_storage.temporary' if name=='ir_prefix' else name))) for name in [*SCALARS,*POINTERS]}}
    context['rule_base']=int(gdb.parse_and_eval('(unsigned)option_rules'))
    context['compiler_passes_address']=int(gdb.parse_and_eval('(unsigned)compiler_passes'))
    if symbol=='next_jobs':
        assert frame.older().name()=='run_driver'
        context['pending_address']=int(frame.older().read_var('pending').address)&0xffffffff
    if symbol=='delete_temporary':context['temporary_suffixes_address']=int(gdb.parse_and_eval('(unsigned)temporary_suffixes'))
    if symbol=='run_passes':context['callbacks']={'final_pass':int(gdb.parse_and_eval('(unsigned)final_pass'))}
    context['crt_mb']={name:int(gdb.parse_and_eval(expression)) for name,expression in
        [('codepage','recon_mb_codepage'),('mb_cur_max','recon_mb_cur_max')]}
    if symbol in ('unload_pass','cleanup_files','delete_temporary','driver_exit','next_jobs'):
        value=lambda expression:int(gdb.parse_and_eval(expression))&0xffffffff
        context['compiler_module']={'sha256':gdb.parse_and_eval('((NativeModule *)cl_c1_module)->sha256').string(),
            'module':value('(unsigned)cl_c1_module'),'invoke':value('(unsigned)invoke_compiler_pass'),
            'abort':value('(unsigned)abort_compiler_pass'),
            'provider_state':value('((NativeModule *)cl_c1_module)->provider_state')}
        assert value('((NativeModule *)cl_c1_module)->export_count')==2
        for index,name,ordinal,callback in ((0,'_InvokeCompilerPass@12',2,'invoke_compiler_pass'),
                                             (1,'_AbortCompilerPass@4',1,'abort_compiler_pass')):
            expression='((NativeModule *)cl_c1_module)->exports[%d]'%index
            assert gdb.parse_and_eval(expression+'.name').string()==name
            assert value(expression+'.ordinal')==ordinal and value(expression+'.lookups')==1
            assert value('(unsigned)'+expression+'.address')==value('(unsigned)'+callback)
        context['execution_addresses']={name:value('(unsigned)&'+name)for name in ('interrupted','active_pass','pass_running')}
        context['crt_addresses']={name:value('(unsigned)&'+expression)for name,expression in {
            'fmode':'recon_fmode','commode':'recon_commode','mb_cur_max':'recon_mb_cur_max',
            'mb_codepage':'recon_mb_codepage','lc_codepage':'recon_lc_codepage',
            'lc_handles':'recon_lc_handle','mbctype':'recon_mbctype','pctype_slot':'recon_pctype',
            'environment_slot':'recon_crt_environment','initenv_slot':'recon_crt_initenv',
            'os_environment_slot':'recon_crt_os_environment'}.items()}
        context['crt_addresses']['initial_pctype']=value('(unsigned)(character_flags+1)')
    if symbol=='vwrite_message':
        caller=frame.older();assert caller.name()in ('warning','fatal')
        context['diagnostic_caller']={'name':caller.name(),'id':int(caller.read_var('id'))&0xffffffff,
            'args_address':int(caller.read_var('id').address)+4}
    if symbol in ('warning','fatal'):
        identifier=context['arguments'][0]or(2000 if symbol=='fatal'else 0)
        formats=json.loads((directory/'diagnostic-formats.json').read_text())
        context['diagnostic_format']=formats['formats'][str(identifier)]
        slot=int(frame.read_var('id').address)+4
        count=len(format_types(context['diagnostic_format']))
        context['varargs']=list(struct.unpack('<%dI'%count,bytes(inferior.read_memory(slot,4*count))))if count else []
    if symbol in ('write_message','write_link_message'):
        # The i386 cdecl variable arguments follow the final named parameter.
        slot=int(frame.read_var('text').address)+4
        context['varargs']=list(struct.unpack('<13I',bytes(inferior.read_memory(slot,52))))
    def read(address,length):
        data=bytes(inferior.read_memory(address,length));ranges.append({'address':address,'data':data.hex()});return data
    fields=snapshot(read,context)
    (directory/'frontier-memory.json').write_text(json.dumps({'context':context,'ranges':ranges},indent=2)+'\n')
    return fields


def decode(directory,platform):
    directory=Path(directory)
    raw=json.loads((directory/'frontier-memory.json').read_text());ranges=iter(raw['ranges'])
    if platform=='original':
        import re
        addresses=re.findall(r'^trap .*address=([0-9a-f]{8}) ',(directory/'events.txt').read_text(),re.M)
        assert len(addresses)==1
        stack=struct.unpack_from('<16I',(directory/'context.bin').read_bytes(),204)
        assert raw['context']==original_context(directory,int(addresses[0],16),stack),'frontier context not derived from raw original trap/stack'
    if raw['context']['symbol']in ('warning','fatal'):
        from resources import extract
        identifier=raw['context']['arguments'][0]or(2000 if raw['context']['symbol']=='fatal'else 0)
        text=extract(identifier)['text']
        assert raw['context']['diagnostic_format']==text,'diagnostic argument types differ from target resource'
        if platform=='native':
            formats=json.loads((directory/'diagnostic-formats.json').read_text())
            config=json.loads((directory/'config.json').read_text())
            assert formats=={'target_sha256':config['target']['sha256'],'formats':{str(identifier):text}}
    def read(address,length):
        item=next(ranges);data=bytes.fromhex(item['data'])
        assert item['address']==address and len(data)==length,'frontier memory transcript differs from explicit decoder requests'
        return data
    fields=snapshot(read,raw['context'])
    assert next(ranges,None) is None,'unused frontier memory capture'
    raw_names=((['frontier-memory.json','context.bin','events.txt','mb-state.bin','crt-state.bin'] if (directory/'mb-state.bin').exists() else ['frontier-memory.json','context.bin','events.txt']) if platform=='original' else ['frontier-memory.json','debugger.txt','gdb-capture.py','gdb_capture.py','frontier_state.py'])
    if platform=='original' and 'compiler_module'in raw['context']:
        raw_names+=['observer/loaded-c1.bin','loaded-msvcrt.bin','observer/events.txt','observer/frontier-crt-words.bin','capture.json']
    if platform=='native'and raw['context']['symbol']in ('warning','fatal'):raw_names+=['diagnostic-formats.json','config.json']
    return {'projection':PROJECTION,'fields':fields,
        'decoder_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'raw_inputs':{name:hashlib.sha256((directory/name).read_bytes()).hexdigest()for name in raw_names}}
