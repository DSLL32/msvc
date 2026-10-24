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
    if config.get('writer_callback_boundary'):
        content=(BINARY/config['writer_callback_boundary']).read_bytes()
        assert sha(content)==config['writer_callback_boundary_sha256']
        (output/'writer-callback-boundary.json').write_bytes(content)
    if config.get('output_callback_boundary'):
        content=(BINARY/config['output_callback_boundary']).read_bytes()
        assert sha(content)==config['output_callback_boundary_sha256']
        (output/'output-callback-boundary.json').write_bytes(content)
    if config.get('section_callback_storage_contract'):
        content=(BINARY/config['section_callback_storage_contract']).read_bytes()
        assert sha(content)==config['section_callback_storage_contract_sha256']
        (output/'section-callback-storage-contract.json').write_bytes(content)
    if config.get('directive_callback_contract'):
        content=(BINARY/config['directive_callback_contract']).read_bytes()
        assert sha(content)==config['directive_callback_contract_sha256']
        (output/'directive-callback-contract.json').write_bytes(content)
    executable=BINARY/'build/native/c2'
    shutil.copyfile(executable,output/'native.elf')
    from case_inputs import manifest,materialize,artifacts
    case_directory,case_value,target_environment=manifest(config)
    work=output/'work';input_names=materialize(case_directory,case_value,work)
    if config.get('data_model'):
        (output/'data-model.json').write_bytes((BINARY/config['data_model']).read_bytes())
    if config.get('intrinsic_model'):
        data=(BINARY/config['intrinsic_model']).read_bytes();assert sha(data)==config['intrinsic_model_sha256']
        (output/'intrinsic-model.json').write_bytes(data)
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
    symbol=record.get('native',record['name'].lstrip('_'))
    # Stepping a breakpointed PUSHF can save debugger TF into the naked
    # helper's real flags frame. Observe immediately after its normal PUSHF.
    Entered('*store_extended+1'if symbol=='store_extended'else symbol,record['name'])
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
    directive_callsite_proof=None
    if config.get('directive_callback_contract'):
        contract=json.loads((BINARY/config['directive_callback_contract']).read_text())
        import importlib.util
        spec=importlib.util.spec_from_file_location('native_observer_proof',BINARY/'scripts/native-proof.py')
        observer=importlib.util.module_from_spec(spec);spec.loader.exec_module(observer)
        site=contract['native_callsite'];assert site['owner']=='write_output_directives'and site['offset']==73 and site['bytes']=='ffd0'and site['length']==2
        directive_callsite_proof=observer.verify_callsite(executable,site['owner'],site['offset'],bytes.fromhex(site['bytes']))
        prefix_program+='''
directive_abi=[]
class DirectiveABI(gdb.Breakpoint):
    def __init__(self,spec,role):
        self.role=role
        super().__init__(spec,internal=True)
    def stop(self):
        regs={name:int(gdb.parse_and_eval('$'+name))for name in ('pc','ecx','ebx','ebp','esi','edi','esp','eax')}
        regs['role']=self.role
        import struct
        regs['stack']=list(struct.unpack('<4I',gdb.selected_inferior().read_memory(regs['esp'],16).tobytes()))
        regs['slot_target']=int(gdb.parse_and_eval('output_slot_49971c.target'))
        regs['directive_base']=int(gdb.parse_and_eval('c2_pools[1].blocks'))
        directive_abi.append(regs)
        gdb.write('C2-DIRECTIVE-ABI '+json.dumps(regs,sort_keys=True)+'\\n')
        return False
DirectiveABI('*write_output_directives+%d','call')
DirectiveABI('*emit_output_directive','entry')
''' % contract['native_callsite']['offset']
    if any(r.get('state_policy')=='extended-store'for r in config['stubs'])or '_store_extended'in config['trial']:
        import re
        decoded=subprocess.check_output(['objdump','-d','-Mintel','--disassemble=store_extended',str(executable)],text=True)
        starts=re.findall(r'^([0-9a-f]+) <store_extended>:$',decoded,re.M)
        stores=re.findall(r'^\s*([0-9a-f]+):\s+db 3e\s+fstp\s+TBYTE PTR \[esi\]$',decoded,re.M)
        store_offsets=[]
        if '_store_extended'in config['accepted']:
            assert len(starts)==len(stores)==1,'need exactly the native private FSTP80'
            at=int(stores[0],16)-int(starts[0],16)
            store_offsets=[(at,'before'),(at+2,'after')]
        prefix_program+='''
store_abi_entry={}
store_abi_entries=[]
store_steps=[]
store_breakpoints=[]
class StoreStep(gdb.Breakpoint):
    def __init__(self,spec,phase):
        self.phase=phase
        super().__init__(spec,internal=True)
    def stop(self):
        pointer=int(gdb.parse_and_eval('$esi'))
        item=dict(phase=self.phase,pointer=pointer,pc=int(gdb.parse_and_eval('$pc')),
            flags=int(gdb.parse_and_eval('$eflags')),
            payload=gdb.selected_inferior().read_memory(pointer,12).tobytes().hex(),
            control=int(gdb.parse_and_eval('$fctrl')),status=int(gdb.parse_and_eval('$fstat')),tag=int(gdb.parse_and_eval('$ftag')),
            instruction=int(gdb.parse_and_eval('$fioff'))&0xffffffff,data_pointer=int(gdb.parse_and_eval('$fooff'))&0xffffffff,
            registers=[gdb.selected_frame().read_register('st%d'%i).bytes.hex()for i in range(8)])
        store_steps.append(item)
        gdb.write('C2-STORE-STEP '+json.dumps(item,sort_keys=True)+'\\n')
        return False
class StoreABI(gdb.Breakpoint):
    def stop(self):
        if not store_breakpoints:
            base=int(gdb.parse_and_eval('&store_extended'))
            for offset,phase in STORE_OFFSETS:
                store_breakpoints.append(StoreStep('*'+hex(base+offset),phase))
        store_abi_entry.update(destination=int(gdb.parse_and_eval('$esi')),
            saved_flags=int.from_bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$esp')),4).tobytes(),'little'),
            control=int(gdb.parse_and_eval('$fctrl')),status=int(gdb.parse_and_eval('$fstat')),tag=int(gdb.parse_and_eval('$ftag')),
            instruction=int(gdb.parse_and_eval('$fioff'))&0xffffffff,data_pointer=int(gdb.parse_and_eval('$fooff'))&0xffffffff,
            registers=[gdb.selected_frame().read_register('st%d'%i).bytes.hex()for i in range(8)])
        gdb.write('C2-STORE-ABI-ENTRY '+json.dumps(store_abi_entry,sort_keys=True)+'\\n')
        store_abi_entries.append(dict(store_abi_entry))
        return False
StoreABI('*store_extended+1',internal=True)
'''.replace('STORE_OFFSETS',repr(store_offsets))
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
    if records[0].get('state_policy')in ('command-line-state','environment-flags','match-option','wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
        ptr=gdb.parse_and_eval('c2_argv');values=[];i=0
        while int(ptr[i]):
            values.append(ptr[i].string());i+=1
            assert i<4096
        info['command_state']={'argc':int(gdb.parse_and_eval('c2_argc')),'argv':values}
        if records[0]['state_policy']=='environment-flags':
            info['flags']=None if int(gdb.parse_and_eval('flags'))==0 else gdb.parse_and_eval('flags').string()
    if records[0].get('state_policy')in ('alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
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
    if records[0].get('state_policy')in ('symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        import hashlib
        table=gdb.parse_and_eval('global_scope.buckets')
        data=gdb.selected_inferior().read_memory(table.address,4096).tobytes()
        info['symbol_hash']={'slots':int(gdb.parse_and_eval('sizeof(global_scope.buckets)/sizeof(global_scope.buckets[0])')),'sha256':hashlib.sha256(data).hexdigest()}
        if records[0]['state_policy']in ('section-linked','section-defaults'):
            section=gdb.parse_and_eval('section');values=[int(table[i])for i in range(1024)]
            info['symbol_hash']['section_slot']=5
            info['symbol_hash']['section_alias']=values[5]==int(section)
            altered=bytearray(data);altered[20:24]=bytes(4)
            info['symbol_hash']['normalized_sha256']=hashlib.sha256(altered).hexdigest()
            info['symbol_hash'].pop('sha256')

    if records[0].get('state_policy')in ('symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        caller=frame.older()
        if records[0]['state_policy']=='type-code-input':
            symbol=caller.read_var('symbol');caller=caller.older()
        elif records[0]['state_policy']in ('section-lookup-input','section-serial-input'):
            symbol=caller.read_var('header');caller=caller.older()
        elif records[0]['state_policy']=='symbol-storage-input':symbol=gdb.parse_and_eval('symbol')
        elif records[0]['state_policy']in ('compact-value-input','compact-short-input'):symbol=caller.read_var('symbol')
        else:symbol=gdb.parse_and_eval('symbol')
        head=int(gdb.parse_and_eval('c2_pools[0].blocks'));file=gdb.parse_and_eval('filenames')
        template=bytearray(gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes());template[24:28]=bytes(4)
        info['symbol_type_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
            'symbol_offset':int(symbol)-head,'caller_symbol_alias':int(symbol)==int(caller.read_var('symbol')),
            'caller_kind':(int(symbol['kind'])if records[0]['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input')else int(caller.read_var('kind'))),'retain_locals':int(caller.read_var('retain_locals')),
            'symbol_bytes':gdb.selected_inferior().read_memory(symbol,110).tobytes().hex(),
            'filename_offset':int(file)-head,'filename_serial':int(file['serial']),
            'filename_name_offset':int(file['name'])-head,'filename_next_null':not int(file['next']),
            'filename_name_alias':int(file['name'])==int(gdb.parse_and_eval('symbol_template.symbol.name')),
            'filename_size':int(gdb.parse_and_eval('sizeof(C2Filename)')),
            'template_bytes':template.hex(),
            'input_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
            'decorated_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&decorated_name'),2059).tobytes().hex()}
        if records[0]['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            info['symbol_type_input']['caller_table_global']=int(caller.read_var('table'))==int(gdb.parse_and_eval('&global_scope'))
            raw=bytearray.fromhex(info['symbol_type_input']['symbol_bytes']);raw[24:28]=bytes(4);info['symbol_type_input']['symbol_bytes']=raw.hex()
            info['symbol_type_input']['symbol_name_offset']=int(symbol['name'])-head
            info['symbol_type_input']['symbol_name']=symbol['name'].string()
        if records[0]['state_policy']=='section-lookup-input':info['symbol_type_input']['lookup_serial']=int(frame.read_var('serial'))
        if records[0]['state_policy']=='section-serial-input':
            info['symbol_type_input']['selected_section_code_alias']=int(frame.older().read_var('section'))==int(gdb.parse_and_eval('code_section'))
        if records[0]['state_policy']=='symbol-put-input':
            scope=gdb.parse_and_eval('scope');section=gdb.parse_and_eval('section_tail');group=section['symbol']['association']['group']
            info['symbol_type_input'].update(scope_global_alias=int(scope)==int(gdb.parse_and_eval('&global_scope')),
                scope_caller_alias=int(scope)==int(caller.read_var('table')),
                pending_function_alias=int(gdb.parse_and_eval('pending_functions'))==int(symbol),
                pending_insertion_head_alias=int(gdb.parse_and_eval('pending_insertion'))==int(gdb.parse_and_eval('&pending_functions')),
                section_offset=int(section)-head,section_owner_alias=int(section['link']['owner'])==int(symbol),
                section_parent_code_alias=int(section['parent'])==int(gdb.parse_and_eval('code_section')),
                section_name_alias=int(section['symbol']['name'])==int(symbol['name']),group_offset=int(group)-head,
                group_bytes=gdb.selected_inferior().read_memory(group,12).tobytes().hex(),group_size=int(gdb.parse_and_eval('sizeof(C2SectionGroup)')))
        gdb.write('C2-SYMBOL-TYPE-INPUT '+json.dumps(info['symbol_type_input'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')=='filename-record':
        symbol=gdb.parse_and_eval('symbol');head=int(gdb.parse_and_eval('c2_pools[0].blocks'))
        template=bytearray(gdb.selected_inferior().read_memory(symbol,64).tobytes());template[24:28]=bytes(4)
        info['filename_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
            'symbol_alias':int(symbol)==int(gdb.parse_and_eval('&symbol_template.symbol')),
            'name_pool_offset':int(symbol['name'])-head,'name':symbol['name'].string(),
            'template_bytes':template.hex(),
            'input_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
            'decorated_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&decorated_name'),2059).tobytes().hex(),
            'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
        gdb.write('C2-FILENAME-INPUT '+json.dumps(info['filename_input'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')=='symbol-name-copy':
        caller=frame.older();name=gdb.parse_and_eval('name')
        info['symbol_copy_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
            'name_alias':int(name)==int(gdb.parse_and_eval('&decorated_name[1]')),
            'pool':int(gdb.parse_and_eval('pool')),'caller_pool':int(caller.read_var('pool')),
            'caller_symbol_alias':int(caller.read_var('symbol'))==int(gdb.parse_and_eval('&symbol_template.symbol')),
            'template_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes().hex(),
            'input_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
            'decorated_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&decorated_name'),2059).tobytes().hex(),
            'workspace_size':int(gdb.parse_and_eval('sizeof(decorated_name)')),
            'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
        gdb.write('C2-SYMBOL-COPY-INPUT '+json.dumps(info['symbol_copy_input'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')=='symbol-name-intern':
        caller=frame.older();symbol=gdb.parse_and_eval('symbol');name=gdb.parse_and_eval('name')
        info['symbol_intern_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
            'symbol_alias':int(symbol)==int(gdb.parse_and_eval('&symbol_template.symbol')),
            'name_alias':int(name)==int(gdb.parse_and_eval('&symbol_name[0]')),
            'pool':int(gdb.parse_and_eval('pool')),'kind':int(caller.read_var('kind')),
            'retain_locals':int(caller.read_var('retain_locals')),
            'template_bytes':gdb.selected_inferior().read_memory(symbol,64).tobytes().hex(),
            'name_bytes':gdb.selected_inferior().read_memory(name,2058).tobytes().hex(),
            'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
        gdb.write('C2-SYMBOL-INTERN-INPUT '+json.dumps(info['symbol_intern_input'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')=='symbol-name-read':
        caller=frame.older();buffer=gdb.parse_and_eval('buffer')
        info['symbol_name_input']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
            'buffer_alias':int(buffer)==int(gdb.parse_and_eval('&symbol_name[0]')),
            'capacity':int(gdb.parse_and_eval('capacity')),'kind':int(caller.read_var('kind')),
            'retain_locals':int(caller.read_var('retain_locals')),
            'template_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes().hex(),
            'name_bytes':gdb.selected_inferior().read_memory(buffer,2058).tobytes().hex(),
            'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
        gdb.write('C2-SYMBOL-NAME-INPUT '+json.dumps(info['symbol_name_input'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')=='symbol-serial-read':
        caller=frame.older()
        arguments=[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument]
        info['symbol_serial_input']={'formals':arguments,'kind':int(caller.read_var('kind')),
            'retain_locals':int(caller.read_var('retain_locals')),
            'template_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_template'),64).tobytes().hex(),
            'name_bytes':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&symbol_name'),2058).tobytes().hex(),
            'source_template_size':int(gdb.parse_and_eval('sizeof(symbol_template)')),
            'fifth_pool':gdb.selected_inferior().read_memory(gdb.parse_and_eval('&c2_pools[4]'),12).tobytes().hex()}
        gdb.write('C2-SYMBOL-SERIAL-INPUT '+json.dumps(info['symbol_serial_input'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')=='symbol-record':
        caller=frame.older();table=caller.read_var('table')
        info['symbol_record_input']={'retain_locals':int(gdb.parse_and_eval('retain_locals')),
            'caller_null_scope':not int(caller.read_var('scope')),'caller_retention':int(caller.read_var('retain_locals')),
            'global_scope_alias':int(table)==int(gdb.parse_and_eval('&global_scope')),
            'scope_bytes':int(gdb.parse_and_eval('sizeof(C2ScopeHash)')),
            'bucket_offset':int(gdb.parse_and_eval('&global_scope.buckets'))-int(gdb.parse_and_eval('&global_scope'))}
    if records[0].get('state_policy')=='global-symbols':
        info['global_symbols_input']={'null_scope':not int(gdb.parse_and_eval('scope')),'active_globals_alias':int(gdb.parse_and_eval('c2_current_il'))==int(gdb.parse_and_eval('c2_globals_input'))}
    if records[0].get('state_policy')in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        import hashlib
        table=gdb.parse_and_eval('global_scope.buckets');head=int(gdb.parse_and_eval('c2_pools[0].blocks'))
        nonnull={i:int(table[i])-head for i in range(1024)if int(table[i])}
        info['symbol_hash']={'slots':1024,'section_offsets':nonnull}
    if records[0].get('state_policy')=='allocate-node':
        info['allocate_node']={'kind':int(gdb.parse_and_eval('kind')),'pool':int(gdb.parse_and_eval('pool'))}
    if records[0].get('state_policy')=='node-size':
        caller=frame.older()
        info['node_size']={'kind':int(gdb.parse_and_eval('kind')),'caller_kind':int(caller.read_var('kind')),'pool':int(caller.read_var('pool'))}
    if records[0].get('state_policy')=='pool-allocate':
        caller=frame.older()
        info['pool_allocate']={'pool':int(gdb.parse_and_eval('pool')),'bytes':int(gdb.parse_and_eval('bytes')),
                               'caller_kind':int(caller.read_var('kind')),'caller_pool':int(caller.read_var('pool'))}
    if records[0].get('state_policy')in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        head=gdb.parse_and_eval('section_head');section=head;rows=[];addresses=[]
        while int(section):
            assert len(rows)<(6 if records[0]['state_policy']=='symbol-put-input'else 5)
            raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes())
            name=int(section['symbol']['name']);class_name=int(section['class_name']);context=int(section['symbol']['association']['context']);nxt=int(section['next'])
            for at in (24,36,50,54):raw[at:at+4]=bytes(4)
            if records[0]['state_policy']=='symbol-put-input'and len(rows)==5:
                raw[74:78]=bytes(4);raw[78:82]=bytes(4)
            rows.append({'bytes':raw.hex(),'name':section['symbol']['name'].string(),'class_name':section['class_name'].string(),
                'name_pointer':name,'class_pointer':class_name,'context_alias':context==int(gdb.parse_and_eval('section_context')),
                'pool_offset':int(section)-int(gdb.parse_and_eval('c2_pools[0].blocks'))})
            addresses.append(int(section));section=section['next']
        assert len(rows)==(6 if records[0]['state_policy']=='symbol-put-input'else 5)
        info['sections']={'rows':rows,'tail_alias':int(gdb.parse_and_eval('section_tail'))==addresses[-1],
            'outputs':[addresses.index(int(gdb.parse_and_eval(name)))if int(gdb.parse_and_eval(name))in addresses else None for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')],
            'output_nulls':[not int(gdb.parse_and_eval(name))for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')],
            'context_identity_alias':int(gdb.parse_and_eval('section_context'))==int(gdb.parse_and_eval('&section_context_identity')),
            'default_alignment':int(gdb.parse_and_eval('default_section_alignment'))}
    if records[0].get('state_policy')=='section-symbol':
        symbol=gdb.parse_and_eval('symbol');section=symbol.cast(gdb.lookup_type('C2Section').pointer())
        raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes());raw[24:28]=bytes(4);raw[54:58]=bytes(4)
        info['section_symbol']={'local':int(gdb.parse_and_eval('local')),'bytes':raw.hex(),
            'name':symbol['name'].string(),'class_name':section['class_name'].string(),
            'name_alias':int(symbol['name'])==int(gdb.parse_and_eval('code_section_name')),
            'pool_offset':int(symbol)-int(gdb.parse_and_eval('c2_pools[0].blocks')),
            'initial_sections':[int(gdb.parse_and_eval(name))for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')]}
    if records[0].get('state_policy')=='section-linked':
        section=gdb.parse_and_eval('section');symbol=section.cast(gdb.lookup_type('C2Symbol').pointer())
        raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes());raw[24:28]=bytes(4);raw[54:58]=bytes(4)
        info['section_linked']={'bytes':raw.hex(),
            'name':symbol['name'].string(),'class_name':section['class_name'].string(),
            'name_alias':int(symbol['name'])==int(gdb.parse_and_eval('code_section_name')),
            'pool_offset':int(symbol)-int(gdb.parse_and_eval('c2_pools[0].blocks')),
            'initial_sections':[int(gdb.parse_and_eval(name))for name in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')]}
    if records[0].get('state_policy')=='section-defaults':
        section=gdb.parse_and_eval('section');symbol=section.cast(gdb.lookup_type('C2Symbol').pointer())
        raw=bytearray(gdb.selected_inferior().read_memory(section,86).tobytes());raw[24:28]=bytes(4);raw[54:58]=bytes(4)
        info['section_defaults']={'bytes':raw.hex(),
            'name':symbol['name'].string(),'class_name':section['class_name'].string(),
            'name_alias':int(symbol['name'])==int(gdb.parse_and_eval('code_section_name')),
            'pool_offset':int(symbol)-int(gdb.parse_and_eval('c2_pools[0].blocks')),
            'code_section_alias':int(gdb.parse_and_eval('code_section'))==int(section),
            'other_sections':[int(gdb.parse_and_eval(name))for name in ('data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')],
            'head_alias':int(gdb.parse_and_eval('section_head'))==int(section),'tail_alias':int(gdb.parse_and_eval('section_tail'))==int(section),
            'context_identity_alias':int(gdb.parse_and_eval('section_context'))==int(gdb.parse_and_eval('&section_context_identity'))}
    if records[0].get('state_policy')=='section-constructor':
        name=gdb.parse_and_eval('name');class_name=gdb.parse_and_eval('class_name')
        info['section_constructor']={'name':name.string(),'class_name':class_name.string(),'serial':int(gdb.parse_and_eval('serial')),
            'name_alias':int(name)==int(gdb.parse_and_eval('code_section_name')),
            'initial_sections':[int(gdb.parse_and_eval(item))for item in ('code_section','data_section','const_section','bss_section','debug_symbols_section','debug_types_section','tls_section')]}
    if records[0].get('state_policy')=='real-from-int':
        source=gdb.parse_and_eval('input');destination=gdb.parse_and_eval('output')
        info['real_from_int']={'value':int(source.dereference()),'output_is_zero':int(destination)==int(gdb.parse_and_eval('&c2_real_zero')),
            'caller_local_alias':int(source)==int(frame.older().read_var('value').address),
            'records':[gdb.selected_inferior().read_memory(gdb.parse_and_eval('&'+name),12).tobytes().hex()for name in ('c2_real_zero','c2_real_one','c2_real_two')]}
    if records[0].get('state_policy')=='extended-store':
        destination=int(gdb.parse_and_eval('$esi'))
        info['extended_store']={'destination_is_zero':destination==int(gdb.parse_and_eval('&c2_real_zero')),
            'destination':gdb.selected_inferior().read_memory(destination,12).tobytes().hex(),
            'control':int(gdb.parse_and_eval('$fctrl')),'status':int(gdb.parse_and_eval('$fstat')),'tag':int(gdb.parse_and_eval('$ftag')),
            'instruction':int(gdb.parse_and_eval('$fioff'))&0xffffffff,'data_pointer':int(gdb.parse_and_eval('$fooff'))&0xffffffff,
            'source_alias':(int(gdb.parse_and_eval('$fooff'))&0xffffffff)==int(frame.older().read_var('input')),
            'pointer':destination,'entry':store_abi_entry,
            'registers':[gdb.selected_frame().read_register('st%%d'%%i).bytes.hex()for i in range(8)]}
        gdb.write('C2-STORE-ABI-STOP '+json.dumps(info['extended_store'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')=='extended-results':
        info['extended_results']={'control':int(gdb.parse_and_eval('$fctrl')),'status':int(gdb.parse_and_eval('$fstat')),'tag':int(gdb.parse_and_eval('$ftag')),
            'records':[gdb.selected_inferior().read_memory(gdb.parse_and_eval('&'+name),12).tobytes().hex()for name in ('c2_real_zero','c2_real_one','c2_real_two')],
            'entries':store_abi_entries,'steps':store_steps}
        gdb.write('C2-EXTENDED-RESULTS '+json.dumps(info['extended_results'],sort_keys=True)+'\\n')
    if records[0].get('state_policy')in ('intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
        info['real_results']={'control':int(gdb.parse_and_eval('$fctrl')),'status':int(gdb.parse_and_eval('$fstat')),'tag':int(gdb.parse_and_eval('$ftag')),
            'records':[gdb.selected_inferior().read_memory(gdb.parse_and_eval('&'+name),12).tobytes().hex()for name in ('c2_real_zero','c2_real_one','c2_real_two')]}
    if records[0].get('state_policy')in ('pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        pools=gdb.parse_and_eval('c2_pools');arena=pools[0];head=arena['blocks']
        assert int(head)
        info['pool_result']={'free':int(gdb.parse_and_eval('c2_free_blocks')),
            'types_offset':int(gdb.parse_and_eval('c2_types'))-int(head),
            'cursor_offset':int(arena['cursor'])-int(head),'remaining':int(arena['remaining']),
            'next':int(head.dereference()['next']),'capacity':int(head.dereference()['capacity']),
            'payload':gdb.selected_inferior().read_memory(int(head)+8,4088).tobytes().hex(),
            'other_pools':[gdb.selected_inferior().read_memory(pools[i].address,12).tobytes().hex()for i in (1,2,3)]}
    if records[0].get('state_policy')in ('section-symbol','section-linked','section-defaults'):
        payload=bytearray.fromhex(info['pool_result']['payload']);payload[88:92]=bytes(4);payload[118:122]=bytes(4)
        info['pool_result']['payload']=payload.hex()
    if records[0].get('state_policy')in ('sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        payload=bytearray.fromhex(info['pool_result']['payload'])
        for index in range(5):
            for field in (24,36,50,54):payload[64+index*88+field:68+index*88+field]=bytes(4)
        if records[0]['state_policy']in ('symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):payload[532:536]=bytes(4)
        if records[0]['state_policy']in ('symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):payload[564:568]=bytes(4)
        if records[0]['state_policy']=='symbol-put-input':
            for at in (692,704,718,722,742,746):payload[at:at+4]=bytes(4)
        info['pool_result']['payload']=payload.hex()
    if records[0].get('state_policy')in ('header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
        first=gdb.parse_and_eval('c2_input');second=gdb.parse_and_eval('c2_globals_input');iob=gdb.parse_and_eval('&recon_iob[0]')
        info['header_streams']={'input_index':(int(first)-int(iob))//32,'globals_index':(int(second)-int(iob))//32,
                                'input':gdb.selected_inferior().read_memory(first,32).tobytes().hex(),
                                'globals':gdb.selected_inferior().read_memory(second,32).tobytes().hex(),
                                'distinct':int(first)!=int(second),'x87_control':int(gdb.parse_and_eval('$fctrl'))}
        if records[0]['state_policy']in ('il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
            info['header_streams']['active_index']=(int(gdb.parse_and_eval('c2_current_il'))-int(iob))//32
        if records[0]['state_policy']in ('il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input'):
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
        data_program="""if records and records[0].get('state_policy')in ('wildcard-match','apply-option','fatal-option','message-lookup','message-initialize','environment-copy','integer-option','alignment-option','intermediate-open','header-read','il-byte-read','il-word-read','il-long-read','symbol-reset','symbol-table-empty','allocate-node','node-size','pool-allocate','pool-result','real-from-int','extended-store','extended-results','intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
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
    if config.get('intrinsic_model'):
        intrinsic_program="""if records and records[0].get('state_policy')in ('intrinsics-initialized','machine-initialized','section-constructor','section-symbol','section-linked','section-defaults','sections-initialized','global-symbols','symbol-record','symbol-serial-read','symbol-name-read','symbol-name-intern','symbol-name-copy','filename-record','symbol-type-input','type-code-input','compact-value-input','compact-short-input','symbol-storage-input','section-lookup-input','section-serial-input','symbol-put-input','globals-complete'):
    model=json.loads(Path(%r).read_text())
    descriptions=gdb.parse_and_eval('intrinsics');expected={r['number']:r for r in model['rows']};texts={r['original']:bytes.fromhex(r['bytes'])for r in model['texts']}
    rows=[]
    for i in range(344):
        description=descriptions[i]
        row={key:int(description[key])for key in ('result_type','reserved_type','flags','argument_signature','reserved_flags','commands','runtime_name')}
        for field in ('commands','runtime_name'):
            size=len(texts[expected[i][field]])+1
            row[field+'_bytes']=gdb.selected_inferior().read_memory(row[field],size).tobytes().hex()
        rows.append(row)
    info['intrinsic_table']=rows
""" % str(output/'intrinsic-model.json')
        program=program.replace('open(',intrinsic_program+'open(',1)
    complete_model=(BINARY/'scripts/globals_state.py').read_text()
    complete_frontiers=[r for r in config['stubs']if r.get('state_policy')=='globals-complete']
    if complete_frontiers:assert all(sha(complete_model.encode())==record['state_model_sha256']for record in complete_frontiers)
    complete_program="""if records and records[0].get('state_policy')=='globals-complete':
    exec(MODEL_SOURCE)
    addresses={key:int(gdb.parse_and_eval('&('+item[1]+')'))for key,item in ROOTS.items()}
    info['globals_complete']=complete_globals(lambda address,size:gdb.selected_inferior().read_memory(address,size).tobytes(),addresses, directives=records[0].get('frontier_contract')in ('output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'), writer=records[0].get('frontier_contract')in ('object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'), coff=records[0].get('frontier_contract')in ('coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'),initialized_output=records[0].get('frontier_contract')in ('coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'),list_pending=records[0].get('frontier_contract')=='coff-chunk-input',list_constructed=records[0].get('frontier_contract')in ('coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'),source_pending=records[0].get('frontier_contract')in ('coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'),symbols_complete=records[0].get('frontier_contract')in ('coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'),append_pending=records[0].get('section_caller')=='add_coff_section',append_complete=records[0].get('section_caller')in ('append_output_table','initialize_coff_section','lookup_output_table','initialize_coff_symbols','register_coff_storage','record_output_section','select_coff_section'),section_symbols_pending=records[0].get('section_caller')in ('lookup_output_table','initialize_coff_symbols','register_coff_storage','record_output_section','select_coff_section'),section_symbols_complete=records[0].get('section_caller')in ('initialize_coff_symbols','register_coff_storage','record_output_section','select_coff_section'),storage_complete=records[0].get('section_caller')in ('register_coff_storage','record_output_section','select_coff_section'),section_recorded=records[0].get('section_caller')in ('record_output_section','select_coff_section'),section_selected=records[0].get('section_caller')=='select_coff_section')
    info['globals_complete']['formals']=[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument]
    info['globals_complete']['opaque_context_anchor_alias']=int(gdb.parse_and_eval('section_context'))==int(gdb.parse_and_eval('&section_context_identity'))
    if records[0].get('frontier_contract')=='initializer-input':
        argument=frame.read_var('input');iob=int(gdb.parse_and_eval('&recon_iob[0]'))
        info['globals_complete']['input_file']={'index':(int(argument)-iob)//32,'main_alias':int(argument)==int(gdb.parse_and_eval('c2_input'))}
    first=gdb.parse_and_eval('c2_input');second=gdb.parse_and_eval('c2_globals_input');iob=int(gdb.parse_and_eval('&recon_iob[0]'))
    info['globals_complete']['streams']={'input_index':(int(first)-iob)//32,'globals_index':(int(second)-iob)//32,
        'active_globals_alias':int(gdb.parse_and_eval('c2_current_il'))==int(second),
        'input_bytes':gdb.selected_inferior().read_memory(first,32).tobytes().hex(),
        'closed_globals_bytes':gdb.selected_inferior().read_memory(second,32).tobytes().hex()}
    if records[0].get('frontier_contract')in ('initializers-complete','backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'):
        info['globals_complete']['streams']['active_input_alias']=int(gdb.parse_and_eval('c2_current_il'))==int(first)
        info['globals_complete']['streams']['valid_input_bytes']=gdb.selected_inferior().read_memory(first.dereference()['_base'],15).tobytes().hex()
    if records[0].get('frontier_contract')in ('backend-inputs-open','output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'):
        opened=[]
        for name in ('expression_input','function_symbol_input'):
            file=gdb.parse_and_eval(name)
            opened.append({'index':(int(file)-iob)//32,'bytes':gdb.selected_inferior().read_memory(file,32).tobytes().hex()})
        info['globals_complete']['streams']['backend_inputs']=opened
    if records[0].get('frontier_contract')in ('output-callbacks-selected','output-directive-input','output-directives-complete','object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'):
        info['output_setup']={'group_name':gdb.parse_and_eval('output_group_name').string(),
            'enabled':int(gdb.parse_and_eval('output_enabled')),
            'slots':[{'address':int(gdb.parse_and_eval('&'+name)),
                'target':int(gdb.parse_and_eval(name+'.target'))}for name in ['output_slot_497de4', 'output_slot_499a00', 'output_slot_497e10', 'output_slot_497dbc', 'output_slot_497de0', 'output_slot_497dec', 'output_slot_497db0', 'output_slot_497dd4', 'output_slot_497ce4', 'output_slot_497ddc', 'output_slot_497dcc', 'output_slot_499750', 'output_slot_491070', 'output_slot_497db4', 'output_slot_491078', 'output_slot_497f60', 'output_slot_497dfc', 'output_slot_497f50', 'output_slot_49971c', 'output_slot_497cec']]}
        gdb.write('C2-OUTPUT-SETUP '+json.dumps(info['output_setup'],sort_keys=True)+'\\n')
    if records[0].get('frontier_contract')in ('object-writer-input','coff-table-input','coff-allocate-input','coff-lists-input','coff-list-input','coff-chunk-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'):
        contract=records[0].get('frontier_contract')
        if contract=='coff-section-input':
            section_depth={'select_output_section':1,'create_coff_section':2,'add_coff_section':3,'append_output_table':2,'initialize_coff_section':3,'lookup_output_table':3,'initialize_coff_symbols':3,'register_coff_storage':2,'record_output_section':1,'select_coff_section':1}.get(records[0].get('section_caller'),0)
            section_frame=frame.older().older().older()if section_depth==3 else frame.older().older()if section_depth==2 else frame.older()if section_depth else frame
            writer_owner=section_frame.older().older().older().older();argument=writer_owner.read_var('writer')
        elif contract=='coff-directive-output-input':argument=frame.older().older().older().read_var('writer');writer_owner=frame.older().older().older()
        elif contract=='coff-directive-input':argument=frame.older().older().read_var('writer');writer_owner=frame.older().older()
        elif contract=='coff-symbols-complete':argument=frame.older().read_var('writer');writer_owner=frame.older()
        elif contract=='coff-symbol-input':argument=frame.older().older().read_var('writer');writer_owner=frame.older().older().older()
        elif contract=='object-writer-input':argument=frame.read_var('writer');writer_owner=frame.older()
        elif contract in ('coff-table-input','coff-lists-input','coff-list-input','coff-source-input','coff-symbol-input','coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'):argument=frame.older().read_var('writer');writer_owner=frame.older().older()
        else:argument=frame.older().older().read_var('writer');writer_owner=frame.older().older().older()
        pool=gdb.parse_and_eval('c2_pools[7]');file=gdb.parse_and_eval('object_output')
        info['object_writer']={'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument],
            'pool_offset':int(argument)-int(pool['blocks']),
            'caller_writer_alias':int(argument)==int(writer_owner.read_var('writer')),
            'file_index':(int(file)-iob)//32,
            'file_bytes':gdb.selected_inferior().read_memory(file,32).tobytes().hex()}
        if contract in ('coff-symbols-complete','coff-directive-input','coff-directive-output-input','coff-section-input'):
            info['object_writer'].pop('caller_writer_alias')
            info['object_writer']['caller_writer_pool_alias']=int(argument)==int(pool['blocks'])+8
        if records[0].get('frontier_contract')=='coff-table-input':info['object_writer']['capacity']=int(frame.read_var('capacity'))
        if records[0].get('frontier_contract')=='coff-allocate-input':info['object_writer'].update(capacity=int(frame.older().read_var('capacity')),allocation_bytes=int(frame.read_var('bytes')),caller_function=frame.older().name(),caller_formals=[{'name':item.name,'type':str(item.type)}for item in frame.older().block()if item.is_argument])
        if contract=='coff-list-input':info['object_writer'].update(kind=int(frame.read_var('buffered')),context_null=int(frame.read_var('count_only'))==0)
        if contract=='coff-chunk-input':info['object_writer'].update(buffered=int(frame.older().read_var('buffered')),count_only=int(frame.older().read_var('count_only')),list_offset=int(frame.older().read_var('list'))-int(pool['blocks']),caller_formals=[{'name':item.name,'type':str(item.type)}for item in frame.older().block()if item.is_argument])
        if contract=='coff-source-input':info['object_writer'].update(name=frame.read_var('name').string(),name_global_alias=int(frame.read_var('name'))==int(gdb.parse_and_eval('coff_source_name')),name_pool_offset=int(frame.read_var('name'))-int(pool['blocks']))
        if contract=='coff-symbol-input':info['object_writer'].update(record_offset=int(frame.read_var('record'))-int(pool['blocks']),caller_name=frame.older().read_var('name').string(),caller_name_global_alias=int(frame.older().read_var('name'))==int(gdb.parse_and_eval('coff_source_name')),caller_formals=[{'name':item.name,'type':str(item.type)}for item in frame.older().block()if item.is_argument])
        if contract in ('coff-directive-input','coff-directive-output-input','coff-section-input'):
            directive_frame=section_frame.older().older()if contract=='coff-section-input'else frame.older()if contract=='coff-directive-output-input'else frame
            directive=directive_frame.read_var('directive');node=directive.dereference();base=int(gdb.parse_and_eval('c2_pools[1].blocks'))
            info['directive_callback']={'formals':[{'name':item.name,'type':str(item.type)}for item in directive_frame.block()if item.is_argument],'offset':int(directive)-base,'next_offset':int(node['next'])-base,'kind':int(node['kind']),'value':node['value'].string(),'value_first_node_alias':int(node['value'])==int(gdb.parse_and_eval('output_directives->value')),'slot_target_alias':int(gdb.parse_and_eval('output_slot_49971c.target'))==int(gdb.parse_and_eval('&emit_output_directive')),'caller':directive_frame.older().name(),'caller_formals':[{'name':item.name,'type':str(item.type)}for item in directive_frame.older().block()if item.is_argument]}
            info['directive_callback_abi']=directive_abi
            info['directive_callback_native_pointer']=int(directive)
            gdb.write('C2-DIRECTIVE-POINTER %x\\n'%int(directive))
            gdb.write('C2-DIRECTIVE-CALLBACK '+json.dumps(info['directive_callback'],sort_keys=True)+'\\n')
        if contract in ('coff-directive-output-input','coff-section-input'):
            output_frame=section_frame.older()if contract=='coff-section-input'else frame
            info['coff_directive_output']={'formals':[{'name':item.name,'type':str(item.type)}for item in output_frame.block()if item.is_argument],'prefix':output_frame.read_var('prefix').string(),'value':output_frame.read_var('value').string(),'suffix_null':int(output_frame.read_var('suffix'))==0,'quoted':int(output_frame.read_var('quoted')),'caller_value_alias':int(output_frame.read_var('value'))==int(directive.dereference()['value'])}
            gdb.write('C2-COFF-DIRECTIVE-OUTPUT '+json.dumps(info['coff_directive_output'],sort_keys=True)+'\\n')
        if contract=='coff-section-input':
            section_owner=section_frame if records[0].get('section_caller')in ('record_output_section','select_coff_section')else frame.older().older()if section_depth==3 else frame.older()if section_depth==2 else frame
            section=section_owner.read_var('section');head=int(gdb.parse_and_eval('c2_pools[0].blocks'))
            info['section_callback']={'formals':[{'name':item.name,'type':str(item.type)}for item in section_owner.block()if item.is_argument],'section_offset':int(section)-head,'directive_global_alias':int(section)==int(gdb.parse_and_eval('directive_section')),'stored_target_alias':int(gdb.parse_and_eval('output_slot_497ce4.target'))==int(gdb.parse_and_eval('&select_output_section')),'caller':section_owner.older().name()}
            if section_depth and records[0].get('section_caller')not in ('record_output_section','select_coff_section'):info['section_callback'].update(caller_section_alias=int(section)==int(section_frame.read_var('section')),coff_section=int(section.dereference()['coff_section']))
            if records[0].get('section_caller')in ('record_output_section','select_coff_section'):info['section_callback']['coff_section']=int(section.dereference()['coff_section'])
            gdb.write('C2-SECTION-CALLBACK '+json.dumps(info['section_callback'],sort_keys=True)+'\\n')
            if section_depth>=2 and records[0].get('section_caller')not in ('append_output_table','initialize_coff_section','lookup_output_table','initialize_coff_symbols','register_coff_storage','record_output_section','select_coff_section'):
                output_owner=frame.older()if section_depth==3 else frame
                info['coff_section_output']={'formals':[{'name':item.name,'type':str(item.type)}for item in output_owner.block()if item.is_argument],'name':output_owner.read_var('name').string(),'flags':int(output_owner.read_var('flags')),'flag8':int(output_owner.read_var('flag8')),'reuse':int(output_owner.read_var('reuse')),'caller':output_owner.older().name(),'name_section_alias':int(output_owner.read_var('name'))==int(section.dereference()['symbol']['name'])}
                gdb.write('C2-COFF-SECTION-OUTPUT '+json.dumps(info['coff_section_output'],sort_keys=True)+'\\n')
            if records[0].get('section_caller')in ('append_output_table','initialize_coff_section','lookup_output_table','initialize_coff_symbols'):
                initializer=frame.older()if section_depth==3 else frame
                creator=initializer.older()
                info['coff_section_initialize']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in initializer.block()if arg.is_argument],'section':int(initializer.read_var('section')),'flags':int(initializer.read_var('flags')),'caller':creator.name(),'caller_index_alias':int(initializer.read_var('section'))==int(creator.read_var('section').dereference()['coff_section']),'caller_flags_alias':int(initializer.read_var('flags'))==int(creator.read_var('flags'))}
                gdb.write('C2-COFF-SECTION-INITIALIZE '+json.dumps(info['coff_section_initialize'],sort_keys=True)+'\\n')
            if records[0].get('section_caller')=='record_output_section':
                info['coff_select_input']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in frame.block()if arg.is_argument],'section':int(frame.read_var('section')),'caller':section_frame.name(),'caller_index_alias':int(frame.read_var('section'))==int(section_frame.read_var('index')),'caller_section_alias':int(frame.read_var('section'))==int(section.dereference()['coff_section'])}
                gdb.write('C2-COFF-SELECT-INPUT '+json.dumps(info['coff_select_input'],sort_keys=True)+'\\n')
            if records[0].get('section_caller')=='select_coff_section':
                info['coff_position_input']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in frame.block()if arg.is_argument],'section':int(frame.read_var('section')),'caller':section_frame.name(),'caller_index_alias':int(frame.read_var('section'))==int(section_frame.read_var('index')),'caller_section_alias':int(frame.read_var('section'))==int(section.dereference()['coff_section']),'selected_global_alias':int(frame.read_var('section'))==int(gdb.parse_and_eval('coff_flags'))}
                gdb.write('C2-COFF-POSITION-INPUT '+json.dumps(info['coff_position_input'],sort_keys=True)+'\\n')
            if records[0].get('section_caller')=='register_coff_storage':
                actual=frame.read_var('section')
                info['coff_section_record']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in frame.block()if arg.is_argument],'offset':int(actual)-head,'caller':frame.older().name(),'caller_section_alias':int(actual)==int(section),'coff_index':int(actual.dereference()['coff_section'])}
                gdb.write('C2-COFF-SECTION-RECORD '+json.dumps(info['coff_section_record'],sort_keys=True)+'\\n')
            if records[0].get('section_caller')=='lookup_output_table':
                data=frame.read_var('data')
                info['coff_section_symbols']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in frame.block()if arg.is_argument],'offset':int(data)-int(pool['blocks']),'caller':initializer.name(),'caller_data_alias':int(data)==int(initializer.read_var('data')),'section_offset':int(data.dereference()['section'])-int(pool['blocks']),'section_table_alias':int(data.dereference()['section'])==int(gdb.parse_and_eval('coff_table->first->items[0]')),'section_data_alias':int(data)==int(data.dereference()['section'].dereference()['data'])}
                gdb.write('C2-COFF-SECTION-SYMBOLS '+json.dumps(info['coff_section_symbols'],sort_keys=True)+'\\n')
            if records[0].get('section_caller')=='initialize_coff_symbols':
                data=frame.read_var('data')
                info['coff_storage_input']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in frame.block()if arg.is_argument],'offset':int(data)-int(pool['blocks']),'caller':initializer.name(),'caller_data_alias':int(data)==int(initializer.read_var('data')),'section_offset':int(data.dereference()['section'])-int(pool['blocks']),'section_table_alias':int(data.dereference()['section'])==int(gdb.parse_and_eval('coff_table->first->items[0]')),'section_data_alias':int(data)==int(data.dereference()['section'].dereference()['data'])}
                gdb.write('C2-COFF-STORAGE-INPUT '+json.dumps(info['coff_storage_input'],sort_keys=True)+'\\n')
            if records[0].get('section_caller')=='initialize_coff_section':
                table=frame.read_var('table')
                info['coff_table_lookup']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in frame.block()if arg.is_argument],'table_offset':int(table)-int(pool['blocks']),'table_global_alias':int(table)==int(gdb.parse_and_eval('coff_table')),'index':int(frame.read_var('index')),'index_caller_alias':int(frame.read_var('index'))==int(initializer.read_var('section')),'caller':initializer.name()}
                gdb.write('C2-COFF-TABLE-LOOKUP '+json.dumps(info['coff_table_lookup'],sort_keys=True)+'\\n')
            if section_depth==3 and records[0].get('section_caller')=='add_coff_section':
                table=frame.read_var('table');item=frame.read_var('item');record=item.cast(gdb.lookup_type('C2OutputSection').pointer())
                info['coff_table_append']={'formals':[{'name':arg.name,'type':str(arg.type)}for arg in frame.block()if arg.is_argument],'table_offset':int(table)-int(pool['blocks']),'item_offset':int(item)-int(pool['blocks']),'table_global_alias':int(table)==int(gdb.parse_and_eval('coff_table')),'item_caller_alias':int(item)==int(frame.older().read_var('section')),'caller':frame.older().name(),'record_size':int(gdb.parse_and_eval('sizeof(C2OutputSection)')),'record_bytes':gdb.selected_inferior().read_memory(record,24).tobytes().hex()}
                gdb.write('C2-COFF-TABLE-APPEND '+json.dumps(info['coff_table_append'],sort_keys=True)+'\\n')
        gdb.write('C2-OBJECT-WRITER '+json.dumps(info['object_writer'],sort_keys=True)+'\\n')
    if records[0].get('frontier_contract')=='output-directive-input':
        argument=frame.read_var('value')
        info['output_directive']={'kind':int(frame.read_var('kind')),'value':argument.string(),
            'caller_value_alias':int(argument)==int(frame.older().read_var('library')),
            'formals':[{'name':item.name,'type':str(item.type)}for item in frame.block()if item.is_argument]}
        gdb.write('C2-OUTPUT-DIRECTIVE '+json.dumps(info['output_directive'],sort_keys=True)+'\\n')
    gdb.write('C2-GLOBALS-COMPLETE '+json.dumps(info['globals_complete'],sort_keys=True)+'\\n')
""".replace('MODEL_SOURCE',repr(complete_model))
    program=program.replace('open(',complete_program+'open(',1)
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
    command=['/usr/bin/gdb','-nx','-batch','-ex','set pagination off','-ex','set confirm off','-ex','set displaced-stepping off',
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
    receipt=dict(directive_callsite_proof=directive_callsite_proof,case_id=config['case']['id'],case_inputs={name:sha((work/name).read_bytes())for name in input_names},artifacts=artifacts(work,case_value,output),target_environment_sha256=sha((output/'target-environment.bin').read_bytes()),data_model_sha256=sha((output/'data-model.json').read_bytes())if config.get('data_model')else None,resource_sha256=resource_sha256,kind='native-linux-gdb',command=command,candidates=config['trial'],base_configuration_sha256=sha(base_snapshot),configuration_sha256=sha(snapshot),
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
