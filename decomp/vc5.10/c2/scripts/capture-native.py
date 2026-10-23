#!/usr/bin/env python3
"""Linux/GDB-only frontier capture. No original transport imports."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import shutil
import time
import textwrap
from state import BINARY, TARGET, load, sha, source_hashes, trial_configuration, audit_input_hashes

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    begin=time.monotonic();base,_,_=load()
    base_snapshot=(BINARY/'functions.json').read_bytes();config,snapshot=trial_configuration(base_snapshot)
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    (output/'base_configuration.json').write_bytes(base_snapshot);(output/'configuration.json').write_bytes(snapshot)
    executable=BINARY/'build/native/c2'
    shutil.copyfile(executable,output/'native.elf')
    from case_inputs import manifest,materialize,artifacts
    case_directory,case_value,target_environment=manifest(config)
    work=output/'work';input_names=materialize(case_directory,case_value,work)
    if config.get('data_model'):
        (output/'data-model.json').write_bytes((BINARY/config['data_model']).read_bytes())
    prefix_program="""import gdb,json
trace=[]
class Entered(gdb.Breakpoint):
    def __init__(self,symbol,logical):
        self.logical=logical
        super().__init__(symbol,internal=True)
    def stop(self):
        if self.logical not in trace:
            trace.append(self.logical)
            gdb.write('C2-SOURCE-ENTRY %%s 0x%%x\\n'%%(self.logical,int(gdb.parse_and_eval('$pc'))))
        return False
for record in %r:
    Entered(record.get('native',record['name'].lstrip('_')),record['name'])
""" % [r for r in config['driver']+config['support']if r['name']in config['accepted']and r.get('kind')!='import']
    prefix_program=prefix_program.replace('trace=[]','trace=[]\nentry_input={}\nexit_status=None')
    observation='''    def stop(self):
        if not entry_input:
            from pathlib import Path
            pid=gdb.selected_inferior().pid
            arguments=Path('/proc/%d/cmdline'%pid).read_bytes().rstrip(b'\\0').split(b'\\0')
            environment=Path('/proc/%d/environ'%pid).read_bytes().split(b'\\0')
            flags=[v[14:].decode()for v in environment if v.startswith(b'MSC_CMD_FLAGS=')]
            assert not flags,'host leaked target compiler flags'
            vector=gdb.parse_and_eval('recon_crt_environment');values=[];index=0
            while int(vector[index]):
                values.append(vector[index].string());index+=1
                assert index<4096
            target_flags=[value[14:]for value in values if value.startswith('MSC_CMD_FLAGS=')]
            entry_input.update(arguments=[v.decode()for v in arguments[1:]],MSC_CMD_FLAGS=target_flags[0]if target_flags else None)
            Path(TARGET_ENVIRONMENT_PATH).write_bytes(b'\\0'.join(value.encode()for value in values)+b'\\0\\0')
            Path(MAPS_PATH).write_text(Path('/proc/%d/maps'%pid).read_text())
            gdb.write('C2-PROCESS-INPUT '+json.dumps(entry_input,sort_keys=True)+'\\n')
'''.replace('MAPS_PATH',repr(str(output/'native-maps.txt'))).replace('TARGET_ENVIRONMENT_PATH',repr(str(output/'target-environment.bin')))
    prefix_program=prefix_program.replace('    def stop(self):\n',observation)
    prefix_program+='''
def process_exited(event):
    global exit_status
    if hasattr(event,'exit_code')and event.exit_code is not None:
        exit_status=int(event.exit_code)
        gdb.write('C2-PROCESS-EXIT %d\\n'%exit_status)
gdb.events.exited.connect(process_exited)
'''
    (output/'gdb-prefix.py').write_text(prefix_program)
    program="""import gdb,json
from pathlib import Path
frame=gdb.newest_frame()
info={'function':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'signal':gdb.execute('info program',to_string=True),'backtrace':gdb.execute('bt',to_string=True),'trace':trace,'globals':{}}
pid=gdb.selected_inferior().pid
actual_args=Path('/proc/%%d/cmdline'%%pid).read_bytes().rstrip(b'\\0').split(b'\\0')
actual_env=Path('/proc/%%d/environ'%%pid).read_bytes().split(b'\\0')
Path(%r).write_text(Path('/proc/%%d/maps'%%pid).read_text())
gdb.execute('info registers eip')
gdb.execute('info symbol $pc')
gdb.execute('x/1ib $pc-1')
gdb.execute('info program')
info['arguments']=[v.decode()for v in actual_args[1:]]
flags=[v[14:].decode()for v in actual_env if v.startswith(b'MSC_CMD_FLAGS=')]
assert not flags,'host compiler flags leaked'
info['MSC_CMD_FLAGS']=entry_input['MSC_CMD_FLAGS']
records=[r for r in %r if r['native']==frame.name()]
if len(records)==1:
    for view in records[0].get('state',[]):
        info['globals'][view['original']]=int(gdb.parse_and_eval(view['native'])) & 0xffffffff
    if records[0].get('state_policy')in ('command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate'):
        ptr=gdb.parse_and_eval('c2_argv');values=[];i=0
        while int(ptr[i]):
            values.append(ptr[i].string());i+=1
            assert i<4096
        info['command_state']={'argc':int(gdb.parse_and_eval('c2_argc')),'argv':values}
        if records[0]['state_policy']=='environment-flags':
            info['flags']=None if int(gdb.parse_and_eval('flags'))==0 else gdb.parse_and_eval('flags').string()
    if records[0].get('state_policy')in ('alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate'):
        vector=gdb.parse_and_eval('recon_crt_environment');index=0;origin=None
        while int(vector[index]):
            entry=vector[index];prefix=gdb.selected_inferior().read_memory(entry,14).tobytes()
            if prefix==b'MSC_CMD_FLAGS=':origin=int(entry)+14;break
            index+=1
        assert origin is not None
        alignment=gdb.parse_and_eval('alignment'if records[0]['state_policy']=='alignment-option'else'c2_alignment_options');current=gdb.parse_and_eval('c2_current_option');operand=gdb.parse_and_eval('c2_option_operand')
        array=gdb.parse_and_eval('environment_arguments');argv=gdb.parse_and_eval('c2_argv')
        info['alignment_option']={'alignment':None if not int(alignment)else alignment.string(),'current':current.string(),
                                  'current_offset':int(current)-origin,'operand':operand.string(),'operand_offset':int(operand)-origin,
                                  'argv_offset':int(argv)-int(array.address),'argv_value_alias':int(argv[0])==int(current)}
    if records[0].get('state_policy')in ('symbol-table-empty','allocate-node','node-size','pool-allocate'):
        import hashlib
        table=gdb.parse_and_eval('c2_symbol_hash')
        data=gdb.selected_inferior().read_memory(table.address,4096).tobytes()
        info['symbol_hash']={'slots':int(gdb.parse_and_eval('sizeof(c2_symbol_hash)/sizeof(c2_symbol_hash[0])')),'sha256':hashlib.sha256(data).hexdigest()}
    if records[0].get('state_policy')=='allocate-node':
        info['allocate_node']={'kind':int(gdb.parse_and_eval('kind')),'pool':int(gdb.parse_and_eval('pool'))}
    if records[0].get('state_policy')=='node-size':
        caller=frame.older()
        info['node_size']={'kind':int(gdb.parse_and_eval('kind')),'caller_kind':int(caller.read_var('kind')),'pool':int(caller.read_var('pool'))}
    if records[0].get('state_policy')=='pool-allocate':
        caller=frame.older()
        info['pool_allocate']={'pool':int(gdb.parse_and_eval('pool')),'bytes':int(gdb.parse_and_eval('bytes')),
                               'caller_kind':int(caller.read_var('kind')),'caller_pool':int(caller.read_var('pool'))}
    if records[0].get('state_policy')in ('header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate'):
        first=gdb.parse_and_eval('c2_input');second=gdb.parse_and_eval('c2_globals_input');iob=gdb.parse_and_eval('&recon_iob[0]')
        info['header_streams']={'input_index':(int(first)-int(iob))//32,'globals_index':(int(second)-int(iob))//32,
                                'input':gdb.selected_inferior().read_memory(first,32).tobytes().hex(),
                                'globals':gdb.selected_inferior().read_memory(second,32).tobytes().hex(),
                                'distinct':int(first)!=int(second),'x87_control':int(gdb.parse_and_eval('$fctrl'))}
        if records[0]['state_policy']in ('il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate'):
            info['header_streams']['active_index']=(int(gdb.parse_and_eval('c2_current_il'))-int(iob))//32
        if records[0]['state_policy']in ('il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate'):
            stream=second.dereference();valid=int(stream['_cnt'])+int(stream['_ptr'])-int(stream['_base'])
            assert 0<valid<=int(stream['_bufsiz'])<=4096
            info['header_streams']['valid_buffer']=gdb.selected_inferior().read_memory(stream['_base'],valid).tobytes().hex()
            if records[0]['state_policy']=='il-long-read':info['il_word_result']=int(frame.older().read_var('machine'))

    if records[0].get('state_policy')=='intermediate-open':
        base=gdb.parse_and_eval('base')
        info['intermediate_open']={'base':base.string(),'suffix':gdb.parse_and_eval('suffix').string(),
                                   'mode':gdb.parse_and_eval('mode').string(),'description':gdb.parse_and_eval('description').string(),
                                   'base_alias':int(base)==int(gdb.parse_and_eval('c2_intermediate_base')),'x87_control':int(gdb.parse_and_eval('$fctrl'))}
    if records[0].get('state_policy')=='integer-option':
        vector=gdb.parse_and_eval('recon_crt_environment');index=0;origin=None
        while int(vector[index]):
            entry=vector[index];prefix=gdb.selected_inferior().read_memory(entry,14).tobytes()
            if prefix==b'MSC_CMD_FLAGS=':origin=int(entry)+14;break
            index+=1
        assert origin is not None
        value=gdb.parse_and_eval('value');current=gdb.parse_and_eval('c2_current_option');operand=gdb.parse_and_eval('c2_option_operand')
        array=gdb.parse_and_eval('environment_arguments');argv=gdb.parse_and_eval('c2_argv')
        info['integer_option']={'text':value.string(),'input_offset':int(value)-origin,'current':current.string(),
                                'current_offset':int(current)-origin,'operand_alias':int(operand)==int(value),
                                'argv_offset':int(argv)-int(array.address),'argv_value_alias':int(argv[0])==int(value),
                                'mb_cur_max':int(gdb.parse_and_eval('recon_mb_cur_max')),'character_flags':int(gdb.parse_and_eval('recon_pctype[49]'))}
    if records[0].get('state_policy')=='environment-copy':
        flags=frame.older().read_var('flags');vector=gdb.parse_and_eval('recon_crt_environment');index=0;identity=None
        while int(vector[index]):
            if vector[index].string().startswith('MSC_CMD_FLAGS='):identity=int(vector[index])+14;break
            index+=1
        assert identity is not None
        info['environment_copy']={'flags':flags.string(),'environment_alias':int(flags)==identity}
    if records[0].get('state_policy')=='format-integer':
        output=gdb.parse_and_eval('buffer')
        origin=frame.older().read_var('buffer')
        info['integer_format']={'value':int(gdb.parse_and_eval('value')),'base':int(gdb.parse_and_eval('base')),
                                'output_offset':int(output)-int(origin),'prefix':gdb.selected_inferior().read_memory(output-13,13).tobytes().hex()}
    if records[0].get('state_policy')in ('format-prefix','format-prefix-args'):
        prefix_args=records[0]['state_policy']=='format-prefix-args'
        arguments=gdb.parse_and_eval('(int *)arguments'if prefix_args else '(int *)((char *)&format+4)')
        buffer=gdb.parse_and_eval('buffer')
        caller=frame.older().older()if prefix_args else frame.older()
        origin=caller.read_var('text').address
        info['format_prefix']={'format':gdb.parse_and_eval('format').string(),'character':int(arguments[0]),'number':int(arguments[1]),
                               'output_offset':int(buffer)-int(origin),'prefix':gdb.selected_inferior().read_memory(buffer-11,12).tobytes().hex()}
    if records[0].get('state_policy')in ('diagnostic-emit','cached-message'):
        info['message_cache']={'initialized':int(gdb.parse_and_eval('messages_initialized')),
                              'cache':[[int(gdb.parse_and_eval('messages[%%d].id'%%i)),gdb.parse_and_eval('messages[%%d].text'%%i).string()if int(gdb.parse_and_eval('messages[%%d].text'%%i))else None]for i in range(7)],
                              'bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('message_buffer'),384).tobytes().hex()}
        if records[0]['state_policy']=='diagnostic-emit':
            arguments=gdb.parse_and_eval('(char **)arguments')
            info['diagnostic']={'category':int(gdb.parse_and_eval('category')),'message':int(gdb.parse_and_eval('message')),
                                'format':gdb.parse_and_eval('format').string(),'format_alias':int(gdb.parse_and_eval('format == message_buffer')),
                                'argument':arguments[0].string(),'phase':arguments[1].string(),
                                'argument_alias':int(arguments[0]==gdb.parse_and_eval('c2_unexpected_argument'))}
        else:
            info['cached_message_id']=int(gdb.parse_and_eval('message'))
    if records[0].get('state_policy')in ('message-load','message-unescape'):
        info['message_load']={'message':int(gdb.parse_and_eval('message'))if records[0]['state_policy']=='message-load'else 0,'buffer':int(gdb.parse_and_eval('buffer')),
                              'capacity':int(gdb.parse_and_eval('capacity'))if records[0]['state_policy']=='message-load'else 384,'allow_missing':int(gdb.parse_and_eval('allow_missing'))if records[0]['state_policy']=='message-load'else 1,
                              'initialized':int(gdb.parse_and_eval('messages_initialized')),
                              'cache':[[int(gdb.parse_and_eval('messages[%%d].id'%%i)),int(gdb.parse_and_eval('messages[%%d].text'%%i))]for i in range(7)],
                              'bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('message_buffer'),384).tobytes().hex()}
    if records[0].get('state_policy')=='match-option':
        info['matcher']={'table':int(gdb.parse_and_eval('table')),'input':gdb.parse_and_eval('input').string(),
                         'next':int(gdb.parse_and_eval('next')),'mode':int(gdb.parse_and_eval('mode')),
                         'record_size':int(gdb.parse_and_eval('sizeof(C2Option)')),
                         'storage_records':int(gdb.parse_and_eval('sizeof(c2_options)/sizeof(C2Option)'))}
        gdb.write('C2-MATCHER table=0x%%x next=0x%%x mode=%%d\\n'%%(info['matcher']['table'],info['matcher']['next'],info['matcher']['mode']))
    if records[0].get('state_policy')=='exit-callback':
        info['callback']=int(gdb.parse_and_eval('callback'))
        gdb.write('C2-CALLBACK 0x%%x\\n'%%info['callback'])
    if records[0].get('state_policy')=='main-arguments-and-initial-state':
        def vector(expression):
            pointer=gdb.parse_and_eval(expression);result=[];i=0
            while int(pointer[i]):
                result.append(pointer[i].string());i+=1
                assert i<4096
            return result
        info['main']={'argc':int(gdb.parse_and_eval('argc')),'argv':vector('argv'),'environment':vector('environment'),
                      'initenv_alias':int(gdb.parse_and_eval('recon_crt_initenv == environment')),
                      'x87_control':int(gdb.parse_and_eval('$fctrl'))}
open(%r,'w').write(json.dumps(info,indent=2)+'\\n')
""" % (str(output/'native-maps.txt'),config['stubs'],str(output/'state.json'))
    if config.get('data_model'):
        data_program="""if records and records[0].get('state_policy')in ('wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate'):
    if records[0]['state_policy']=='wildcard-match':
        info['wildcard']={'pattern':gdb.parse_and_eval('pattern').string(),'input':gdb.parse_and_eval('input').string(),
                      'kind':int(gdb.parse_and_eval('kind')),'current_alias':int(gdb.parse_and_eval('c2_current_option == input'))}
    if records[0]['state_policy']=='apply-option':
        info['apply']={'option':int(gdb.parse_and_eval('option')),'value':gdb.parse_and_eval('value').string(),
                       'mode':int(gdb.parse_and_eval('mode')),'current_alias':int(gdb.parse_and_eval('c2_current_option == value'))}
    if records[0]['state_policy']=='message-initialize':
        info['message_initialization']={'current':gdb.parse_and_eval('c2_current_option').string(),
                                       'current_alias':int(gdb.parse_and_eval('c2_current_option == c2_unexpected_argument')),
                                       'current_pointer':int(gdb.parse_and_eval('c2_current_option'))}
    if records[0]['state_policy']=='message-lookup':
        info['lookup']={'message':int(gdb.parse_and_eval('message')),'allow_missing':int(gdb.parse_and_eval('allow_missing')),
                        'current':gdb.parse_and_eval('c2_current_option').string(),
                        'current_alias':int(gdb.parse_and_eval('c2_current_option == c2_unexpected_argument')),
                        'current_pointer':int(gdb.parse_and_eval('c2_current_option'))}
    if records[0]['state_policy']=='fatal-option':
        arguments=gdb.parse_and_eval('(char **)((char *)&message+4)')
        info['fatal']={'message':int(gdb.parse_and_eval('message')),'argument':arguments[0].string(),'phase':arguments[1].string(),
                       'current_alias':int(arguments[0]==gdb.parse_and_eval('c2_current_option')),
                       'unexpected_alias':int(arguments[0]==gdb.parse_and_eval('c2_unexpected_argument')),
                       'argument_pointer':int(arguments[0])}
    model=json.loads(Path(%r).read_text())
    table=gdb.parse_and_eval('c2_options');options=[];selectors={};objects={}
    for i in range(len(model['options'])+1):
        row=table[i]
        options.append({'pattern':row['pattern'].string()if int(row['pattern'])else None,'value':int(row['value']),
                        'result':int(row['result']),'kind':int(row['kind']),'reserved':int(row['reserved'])})
    for item in model['selectors']:
        rows=[];table=gdb.parse_and_eval(item['native'])
        for i in range(len(item['entries'])+1):
            row=table[i];rows.append({'selector':int(row['selector']),'kind':int(row['kind']),'value':int(row['value'])})
        selectors[item['native']]=rows
    for item in model['objects']:
        if item['kind']=='selector':continue
        value=gdb.parse_and_eval(item['native'])
        objects[item['native']]=(value.string()if int(value)else None)if item['kind']=='string'else int(value)&0xffffffff
    info['data_state']={'options':options,'selectors':selectors,'objects':objects}
""" % str(output/'data-model.json')
        program=program.replace('open(',data_program+'open(',1)
    completion='''import gdb,json
from pathlib import Path
if exit_status is not None:
    assert entry_input
    info=dict(entry_input,completed=True,exit_status=exit_status,trace=trace)
    Path(STATE_PATH).write_text(json.dumps(info,indent=2)+'\\n')
else:
'''.replace('STATE_PATH',repr(str(output/'state.json')))
    program=completion+textwrap.indent(program,'    ')
    (output/'gdb-state.py').write_text(program)
    command=['/usr/bin/gdb','-nx','-batch','-ex','set pagination off','-ex','set confirm off',
             '-ex','source '+str(output/'gdb-prefix.py'),
             '-ex','run '+ ' '.join(config['case']['arguments'])+' > '+str(output/'stdout.bin')+' 2> '+str(output/'stderr.bin'),
             '-ex','source '+str(output/'gdb-state.py'),str(executable)]
    env=dict(os.environ);env.pop('MSC_CMD_FLAGS',None)
    environment_file=case_directory/case_value['environment']['raw_vector']
    assert environment_file.read_bytes()==target_environment
    env['C2_ENVIRONMENT_FILE']=str(environment_file)
    env['C2_CASE']=config['case']['id']
    resources=BINARY/'build/messages.rmr';resource_sha256=sha(resources.read_bytes())
    env['C2_RESOURCE_FILE']=str(resources)
    start=time.monotonic();result=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=30)
    execution=time.monotonic()-start
    (output/'gdb.stdout').write_bytes(result.stdout);(output/'gdb.stderr').write_bytes(result.stderr)
    provenance=BINARY/'build/native/input-provenance.json'
    build_inputs=json.loads(provenance.read_text())
    assert build_inputs['binary_sha256']==sha(executable.read_bytes()),'native build provenance does not match executable'
    shutil.copyfile(provenance,output/'compiled-inputs.json')
    shutil.copytree(build_inputs['snapshot'],output/'compiled-inputs')
    assert sha(resources.read_bytes())==resource_sha256,'prepared resources changed during capture'
    receipt=dict(case_id=config['case']['id'],case_inputs={name:sha((work/name).read_bytes())for name in input_names},artifacts=artifacts(work,case_value,output),target_environment_sha256=sha((output/'target-environment.bin').read_bytes()),data_model_sha256=sha((output/'data-model.json').read_bytes())if config.get('data_model')else None,resource_sha256=resource_sha256,kind='native-linux-gdb',command=command,candidates=config['trial'],base_configuration_sha256=sha(base_snapshot),configuration_sha256=sha(snapshot),
                 target_sha256=sha(TARGET.read_bytes()),native_sha256=sha(executable.read_bytes()),
                 binary_snapshot_sha256=sha((output/'native.elf').read_bytes()),
                 rebuilt_sha256=sha((BINARY/'build/c2.exe').read_bytes()),
                 audit_inputs=audit_input_hashes(),source_hashes=source_hashes(),state_sha256=sha((output/'state.json').read_bytes())if(output/'state.json').exists()else None,
                 provenance_sha256=sha((output/'compiled-inputs.json').read_bytes()),
                 gdb_inputs={name:sha((output/name).read_bytes())for name in (('gdb-prefix.py','gdb-state.py','data-model.json')if config.get('data_model')else('gdb-prefix.py','gdb-state.py'))},
                 gdb_outputs={name:sha((output/name).read_bytes())for name in ('gdb.stdout','gdb.stderr','native-maps.txt')},
                 environment=config['case']['environment'],helper_status=result.returncode,
                 seconds={'execution':execution,'total':time.monotonic()-begin})
    for stream in ('stdout','stderr'):
        path=output/(stream+'.bin');receipt[stream]={'bytes':path.stat().st_size,'sha256':sha(path.read_bytes())}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    assert receipt['binary_snapshot_sha256']==receipt['native_sha256'],'native executable changed during capture'
    assert result.returncode==0 and (output/'state.json').is_file(),result.stderr
    assert base_snapshot==(BINARY/'functions.json').read_bytes()
    print((output/'state.json').read_text())
    print(output/'receipt.json')

if __name__=='__main__':main()
