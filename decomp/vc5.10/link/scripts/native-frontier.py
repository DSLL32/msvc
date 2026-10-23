#!/usr/bin/env python3
"""Native i386 GDB capture. This module has no Wine/PE dependency runner."""
import argparse
from contextlib import nullcontext
import shlex
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]


def sha(data): return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',choices=['help','hello-win32','hello-dll'],default='help')
    parser.add_argument('--config', type=Path, default=BINARY / 'functions.json')
    parser.add_argument('--environment-reference',type=Path,help='explicit cached original hardware sample; never invokes an original')
    args = parser.parse_args()
    subprocess.run(['make','-q','native'],cwd=BINARY,check=True)
    scratch = Path.home() / '.ai-tmp/msvc2/vc5.10/link'
    capture = Path(tempfile.mkdtemp(prefix='native-', dir=scratch))
    config = args.config.read_bytes()
    (capture / 'functions.json').write_bytes(config)
    (capture / 'runtime.json').write_bytes((BINARY / 'runtime.json').read_bytes())
    from capture_sources import freeze
    source_content,content_paths=freeze(capture)
    configuration = json.loads(config)
    from case_inputs import load,workspace,native_environment,windows_work
    manifest=load(args.case,configuration)
    arguments=manifest['arguments']
    if args.case!='help':(capture/'case.json').write_text(json.dumps(manifest,indent=2)+'\n')
    targets = []
    for record in configuration['driver']:
        if record['name'] in configuration['accepted']:
            targets.append((record['name'], record.get('native_symbol','link_main' if record['name']=='_main' else record['name'].lstrip('_'))))
    script = '''
import gdb,json,struct
state={'entries':[],'initerm_calls':[],'operator_new_calls':[],'atexit_calls':[],'console_calls':[],'module_probes':[],'system_calls':[],'mbsnbicmp_calls':[],'virtual_alloc_calls':[],'parser_allocations':[],'duplicate_calls':[],'timing_end_calls':[],'delete_calls':[],'new_ownership':[],'mbspbrk_calls':[],'canonicalized_arguments':[],'library_search_calls':[],'searchenv_calls':[],'free_calls':[],'link_string_calls':[],'virtual_memory_calls':[]}
def cpp_ownership():
    result=[];node=gdb.parse_and_eval('recon_cpp_allocations');seen=set()
    while int(node):
        address=int(node);assert address not in seen and len(seen)<32
        seen.add(address);value=node.dereference()
        result.append(dict(record=address,**{key:int(value[key])for key in ('base','requested','rounded','generation','live')}))
        node=value['next']
    return result
class NewReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,size):
        super().__init__(frame,internal=True);self.size=size
    def stop(self):
        state['operator_new_calls'].append({'bytes':self.size,'result':int(self.return_value)})
        owned=[r for r in cpp_ownership()if r['base']==int(self.return_value)and r['live']]
        assert len(owned)==1 and owned[0]['requested']==self.size
        state['new_ownership'].append(owned[0])
        return False
class NewEntry(gdb.Breakpoint):
    def stop(self):
        size=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        NewReturned(gdb.newest_frame(),size)
        return False
NewEntry('*recon_cpp_new',internal=True)
class MbsReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,left,right,count):
        super().__init__(frame,internal=True);self.left=left;self.right=right;self.count=count
    def stop(self):
        state['mbsnbicmp_calls'].append(dict(left=self.left,right=self.right,bytes=self.count,result=int(self.return_value)))
        return False
class MbsEntry(gdb.Breakpoint):
    def stop(self):
        arguments=gdb.parse_and_eval('(unsigned*)$esp')
        text=gdb.lookup_type('char').pointer()
        MbsReturned(gdb.newest_frame(),gdb.Value(int(arguments[1])).cast(text).string(),gdb.Value(int(arguments[2])).cast(text).string(),int(arguments[3]))
        return False
MbsEntry('*recon_mbsnbicmp',internal=True)
class MbspbrkReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,text,accept):super().__init__(frame,internal=True);self.text=text;self.accept=accept
    def stop(self):
        state['mbspbrk_calls'].append(dict(text=self.text,accept=self.accept,result=int(self.return_value)))
        return False
class MbspbrkEntry(gdb.Breakpoint):
    def stop(self):
        arguments=gdb.parse_and_eval('(unsigned*)$esp');text=gdb.lookup_type('char').pointer()
        MbspbrkReturned(gdb.newest_frame(),gdb.Value(int(arguments[1])).cast(text).string(),gdb.Value(int(arguments[2])).cast(text).string())
        return False
MbspbrkEntry('*recon_mbspbrk',internal=True)
class CanonicalizeReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,text,address):super().__init__(frame,internal=True);self.text=text;self.address=address
    def stop(self):
        result=int(self.return_value);text=self.return_value.string();data=gdb.selected_inferior().read_memory(result,len(text)+1).tobytes()
        state['canonicalized_arguments'].append(dict(input=self.address,result=result,text=text,bytes=data.hex()))
        return False
class CanonicalizeEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();text=frame.read_var('text');CanonicalizeReturned(frame,text.string(),int(text));return False
CanonicalizeEntry('canonicalize_argument',internal=True)

def errors():
    return dict(errno=int(gdb.parse_and_eval('(int)errno')),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval('last_error')))
def environment_value(name):
    pointer=gdb.parse_and_eval('recon_crt_environment'if int(gdb.parse_and_eval('recon_crt_arguments_bound'))else '(char**)environ')
    for index in range(4096):
        if not int(pointer[index]):return None
        text=pointer[index].string();key,separator,value=text.partition('=')
        if separator and key.upper()==name.upper():return value
    raise AssertionError('unterminated environment')
class LibrarySearchReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')),after=gdb.selected_inferior().read_memory(self.record['found'],260).tobytes().hex())
        state['library_search_calls'].append(self.record);return False
class LibrarySearchEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');name=int(args[1]);found=int(args[2])
        record=dict(name=name,found=found,text=gdb.Value(name).cast(gdb.lookup_type('char').pointer()).string(),entry_stack=int(gdb.parse_and_eval('$esp')),before=gdb.selected_inferior().read_memory(found,260).tobytes().hex())
        LibrarySearchReturned(gdb.newest_frame(),record);return False
LibrarySearchEntry('*search_library_paths',internal=True)
class SearchenvReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(after=gdb.selected_inferior().read_memory(self.record['output'],260).tobytes().hex(),after_errors=errors())
        state['searchenv_calls'].append(self.record);return False
class SearchenvEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');text=gdb.lookup_type('char').pointer();output=int(args[3])
        variable=gdb.Value(int(args[2])).cast(text).string()
        record=dict(filename_pointer=int(args[1]),variable_pointer=int(args[2]),output=output,filename=gdb.Value(int(args[1])).cast(text).string(),variable=variable,value=environment_value(variable),before=gdb.selected_inferior().read_memory(output,260).tobytes().hex(),before_errors=errors(),codepage=int(gdb.parse_and_eval('recon_mb_codepage')),lc_ctype_handle=int(gdb.parse_and_eval('recon_lc_handle[2]')),mb_cur_max=int(gdb.parse_and_eval('recon_mb_cur_max')))
        SearchenvReturned(gdb.newest_frame(),record);return False
SearchenvEntry('*recon_searchenv',internal=True)
class FreeReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,after_errors=errors(),return_stack=int(gdb.parse_and_eval('$esp')))
        state['free_calls'].append(self.record);return False
class FreeEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        if not frame.older()or frame.older().name()!='release_memory':return False
        address=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'));text=gdb.Value(address).cast(gdb.lookup_type('char').pointer()).string()
        FreeReturned(frame,dict(argument=address,text=text,before=gdb.selected_inferior().read_memory(address,len(text)+1).tobytes().hex(),before_errors=errors(),entry_stack=int(gdb.parse_and_eval('$esp'))))
        return False
FreeEntry('*recon_free',internal=True)
class FullLinkEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');output=int(args[1])
        state['full_link_entry']=dict(this=int(gdb.parse_and_eval('$ecx')),pack_requested=output,stack_pointer=int(gdb.parse_and_eval('$esp')),initial=gdb.selected_inferior().read_memory(output,1).tobytes().hex())
        return False
FullLinkEntry('*'+str(int(gdb.lookup_global_symbol('LinkContext::FullLink').value().address)),internal=True)

class AtExitReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,callback):
        super().__init__(frame,internal=True);self.callback=callback
    def stop(self):
        state['atexit_calls'].append({'callback':self.callback,'result':int(gdb.parse_and_eval('(int)$eax'))})
        return False
class AtExitEntry(gdb.Breakpoint):
    def stop(self):
        callback=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        name=gdb.execute('info symbol '+str(callback),to_string=True).strip().split(' ')[0]
        if name in ('free_buffer12','free_buffer8','free_buffer4','close_timing'):
            AtExitReturned(gdb.newest_frame(),name)
        return False
AtExitEntry('*atexit',internal=True)
class ConsoleReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,callback,add):
        super().__init__(frame,internal=True);self.callback=callback;self.add=add
    def stop(self):
        state['console_calls'].append(dict(callback=self.callback,add=self.add,result=int(self.return_value)))
        return False
class ConsoleEntry(gdb.Breakpoint):
    def stop(self):
        callback=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        add=int(gdb.parse_and_eval('((unsigned*)$esp)[2]'))
        name=gdb.execute('info symbol '+str(callback),to_string=True).strip().split(' ')[0]
        ConsoleReturned(gdb.newest_frame(),name,add)
        return False
ConsoleEntry('*recon_SetConsoleCtrlHandler',internal=True)
class ModuleReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,kind,requested,module):
        super().__init__(frame,internal=True);self.kind=kind;self.requested=requested;self.module=module
    def stop(self):
        state['module_probes'].append(dict(kind=self.kind,requested=self.requested,module=self.module,result=int(self.return_value)))
        return False
class ModuleEntry(gdb.Breakpoint):
    def __init__(self,symbol,kind):super().__init__(symbol,internal=True);self.kind=kind
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        module=int(args[1]) if self.kind=='proc' else 0
        address=int(args[2 if self.kind=='proc' else 1])
        requested=gdb.Value(address).cast(gdb.lookup_type('char').pointer()).string()
        ModuleReturned(gdb.newest_frame(),self.kind,requested,module)
        return False
ModuleEntry('*recon_GetModuleHandleA','module')
ModuleEntry('*recon_GetProcAddress','proc')
class SystemReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,kind,address,size,before):
        super().__init__(frame,internal=True);self.kind=kind;self.address=address;self.size=size;self.before=before
    def stop(self):
        after=gdb.selected_inferior().read_memory(self.address,self.size).tobytes().hex()
        state['system_calls'].append(dict(kind=self.kind,address=self.address,before=self.before,after=after))
        return False
class SystemEntry(gdb.Breakpoint):
    def __init__(self,symbol,kind,size):super().__init__(symbol,internal=True);self.kind=kind;self.size=size
    def stop(self):
        address=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        before=gdb.selected_inferior().read_memory(address,self.size).tobytes().hex()
        SystemReturned(gdb.newest_frame(),self.kind,address,self.size,before)
        return False
SystemEntry('*recon_GetSystemInfo','system',36)
SystemEntry('*recon_GlobalMemoryStatus','memory',32)
class VirtualReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,arguments):super().__init__(frame,internal=True);self.arguments=arguments
    def stop(self):
        address=int(self.return_value)
        state['virtual_alloc_calls'].append(dict(arguments=self.arguments,result=address,initial=gdb.selected_inferior().read_memory(address,self.arguments[1]).tobytes().hex() if address else None))
        return False
class VirtualEntry(gdb.Breakpoint):
    def stop(self):
        arguments=[int(gdb.parse_and_eval('((unsigned*)$esp)[%%d]'%%i)) for i in range(1,5)]
        VirtualReturned(gdb.newest_frame(),arguments)
        return False
VirtualEntry('*recon_VirtualAlloc',internal=True)
class ParserAllocationReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,size):super().__init__(frame,internal=True);self.size=size
    def stop(self):
        state['parser_allocations'].append(dict(bytes=self.size,result=int(self.return_value)))
        return False
class ParserAllocationEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        if frame.older() and frame.older().name()=='parse_switch':
            ParserAllocationReturned(frame,int(gdb.parse_and_eval('((unsigned*)$esp)[1]')))
        return False
ParserAllocationEntry('*allocate_memory',internal=True)
class VirtualMemoryReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')))
        state['virtual_memory_calls'].append(self.record);return False
class VirtualMemoryEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        VirtualMemoryReturned(gdb.newest_frame(),dict(arguments=[int(args[i])for i in range(1,5)],entry_stack=int(gdb.parse_and_eval('$esp'))));return False
VirtualMemoryEntry('*allocate_virtual_memory',internal=True)
class LinkStringReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')))
        state['link_string_calls'].append(self.record);return False
class LinkStringEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');address=int(args[1]);text=gdb.Value(address).cast(gdb.lookup_type('char').pointer()).string()
        LinkStringReturned(gdb.newest_frame(),dict(input=address,text=text,entry_stack=int(gdb.parse_and_eval('$esp'))));return False
LinkStringEntry('*duplicate_link_string',internal=True)
class DuplicateReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,text,address):super().__init__(frame,internal=True);self.text=text;self.address=address
    def stop(self):
        state['duplicate_calls'].append(dict(text=self.text,input=self.address,result=int(self.return_value)))
        return False
class DuplicateEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();text=frame.read_var('text')
        DuplicateReturned(frame,text.string(),int(text))
        return False
DuplicateEntry('duplicate_argument',internal=True)
class TimingEndReturned(gdb.FinishBreakpoint):
    def stop(self):
        state['timing_end_calls'].append(int(self.return_value))
        return False
class TimingEndEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        if frame.older() and frame.older().name()=='close_timing':TimingEndReturned(frame,internal=True)
        return False
TimingEndEntry('*recon_GetTickCount',internal=True)
class DeleteReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,address,before):super().__init__(frame,internal=True);self.address=address;self.before=before
    def stop(self):
        records=[r for r in cpp_ownership()if r['generation']==self.before['generation']]
        assert len(records)==1 and records[0]==dict(self.before,live=0)
        state['delete_calls'].append(dict(argument=self.address,normal_return=True,before=self.before,after=records[0]))
        state['cleanup_ownership']=cpp_ownership()
        state['cleanup_descriptors']={name:gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&'+name)),12).tobytes().hex()for name in ('buffer12','buffer8','buffer4')}
        return False
class DeleteEntry(gdb.Breakpoint):
    def stop(self):
        address=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        records=[r for r in cpp_ownership()if r['base']==address and r['live']]
        assert len(records)==1 and records[0]in state['new_ownership']
        DeleteReturned(gdb.newest_frame(),address,records[0])
        return False
DeleteEntry('*recon_cpp_delete',internal=True)




def callback_names(begin,count):
    result=[]
    for index in range(count):
        pointer=int((begin+index).dereference())
        result.append(gdb.execute('info symbol '+str(pointer),to_string=True).strip().split(' ')[0] if pointer else None)
    return result
class InitermEntry(gdb.Breakpoint):
    def stop(self):
        address=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        end=int(gdb.parse_and_eval('((unsigned*)$esp)[2]'))
        begin=gdb.Value(address).cast(gdb.lookup_type('ReconInitializer').pointer())
        state['initerm_calls'].append(callback_names(begin,(end-address)//4))
        return False
InitermEntry('*recon_initterm',internal=True)
class FirstEntry(gdb.Breakpoint):
    def __init__(self,logical,symbol):
        super().__init__(symbol,internal=True)
        self.logical=logical
    def stop(self):
        state['entries'].append(self.logical)
        self.enabled=False
        return False
for logical,symbol in %r:
    FirstEntry(logical,symbol)
def stopped(event):
    state['signal']=getattr(event,'stop_signal',None)
    state['signal_number']=int(gdb.parse_and_eval('$_siginfo.si_signo'))
    frame=gdb.newest_frame()
    state['function']=frame.name()
    state['pc']=int(gdb.parse_and_eval('$pc'))
    state['previous_byte']=int.from_bytes(gdb.selected_inferior().read_memory(state['pc']-1,1).tobytes(),'little')
    state['x87_control_word']=int(gdb.parse_and_eval('$fctrl'))
    state['crt_fmode']=int(gdb.parse_and_eval('recon_fmode'))
    state['crt_commode']=int(gdb.parse_and_eval('recon_commode'))
    state['crt_adjust_fdiv']=int(gdb.parse_and_eval('recon_adjust_fdiv'))
    stdout_address=int(gdb.parse_and_eval('&recon_iob[1]'))
    state['cpp_ownership']=cpp_ownership()
    state['crt_stdout_address']=stdout_address
    state['crt_stdout_bytes']=gdb.selected_inferior().read_memory(stdout_address,32).tobytes().hex()
    stdout_words=struct.unpack('<8I',bytes.fromhex(state['crt_stdout_bytes']))
    if stdout_words[2] and stdout_words[0]>=stdout_words[2] and stdout_words[0]-stdout_words[2]<=stdout_words[6]:
        state['stdout_pending']=gdb.selected_inferior().read_memory(stdout_words[2],stdout_words[0]-stdout_words[2]).tobytes().hex()if stdout_words[0]!=stdout_words[2]else ''
    state['logo_text']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&logo_text')),12).tobytes().hex()

    state['initializers']=[gdb.execute('info symbol '+str(int(gdb.parse_and_eval('application_initializers[%%d]'%%i))),to_string=True).strip().split(' ')[0] if i else None for i in range(5)]
    state['diagnostic_output']=int(gdb.parse_and_eval('diagnostic_output'))
    if state['function']=='initialize_debug':
        state['frontier_arguments']=dict(options=int(frame.read_var('options')),environment=frame.read_var('environment').string())
    if state['function']=='get_debug_flag':state['frontier_arguments']=dict(index=int(frame.read_var('index')))
    driver=frame
    while driver and driver.name()!='run_linker':driver=driver.older()
    if driver:
        count=int(driver.read_var('argc'));argv=driver.read_var('argv')
        state['frontier_arguments']=dict(argc=count,argv=[argv[i].string() for i in range(count)],argv_address=int(argv))
        state['selected_tool_name']=gdb.parse_and_eval('selected_tool_name').string()
        state['selected_tool_name_address']=int(gdb.parse_and_eval('selected_tool_name'))
        state['first_tool_name_address']=int(gdb.parse_and_eval('tool_modes[0].name'))
        state['selected_tool_kind']=int(gdb.parse_and_eval('selected_tool_kind'))
    configuration=frame
    while configuration and configuration.name()!='configure_link_options':configuration=configuration.older()
    full_frame=frame
    while full_frame and full_frame.name()!='LinkContext::FullLink':full_frame=full_frame.older()
    if configuration or state['function']in ('validate_debug_options','LinkContext::FullLink','initialize_output_modules','duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats'):
        receiver=(full_frame.read_var('this')if full_frame else (configuration if configuration else frame).read_var('context'))
        state['context_this']=int(receiver);state['context_vxd']=int(receiver.dereference()['is_vxd'])
        state['caller_context']=int(driver.read_var('context'))
        state['link_context']=gdb.selected_inferior().read_memory(state['context_this'],664).tobytes().hex()
        state['context_extension']=receiver.dereference()['options']['default_extension'].string()
        state['configuration_arguments']=dict(context=int(receiver),first=int(configuration.read_var('module'))if configuration else 0,second=int(configuration.read_var('file'))if configuration else 0)
        state['context_auxiliary_address']=int(receiver.dereference()['auxiliary'])
        state['context_auxiliary']=gdb.selected_inferior().read_memory(state['context_auxiliary_address'],44).tobytes().hex()
        state['context_heap']=[]
        for member in ('symbols','strings'):
            table=receiver.dereference()[member];hash_state=table.dereference()['hash']['state']
            record=dict(table=int(table),table_raw=gdb.selected_inferior().read_memory(int(table),40).tobytes().hex(),hash=int(hash_state),hash_raw=gdb.selected_inferior().read_memory(int(hash_state),56).tobytes().hex(),arenas=[])
            for field in ('directory_arena','bucket_arena'):
                arena=int(hash_state.dereference()[field]);raw=gdb.selected_inferior().read_memory(arena,16).tobytes();words=struct.unpack('<4I',raw)
                record['arenas'].append(dict(address=arena,raw=raw.hex(),data=gdb.selected_inferior().read_memory(words[3],words[1]+words[2]).tobytes().hex()))
            state['context_heap'].append(record)
        if state['function']in ('validate_debug_options','LinkContext::FullLink','initialize_output_modules','duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats'):
            state['validation_arguments']=dict(context=int(receiver))
            state['output_name_pointer']=int(gdb.parse_and_eval('output_file_name'))
            state['output_name']=gdb.parse_and_eval('output_file_name').string()
            state['option_globals']={name:int(gdb.parse_and_eval(name))for name in ('incremental_requested','incremental_switch_set','debug_formats','pdb_enabled','debug_option_present')}
        if full_frame:
            output=full_frame.read_var('pack_requested')
            state['full_link_formals']=dict(this=int(receiver),pack_requested=int(output),caller_output=int(driver.read_var('pack_requested').address),initial=gdb.selected_inferior().read_memory(int(output),1).tobytes().hex())
        if state['function']=='initialize_output_modules':
            module=frame.read_var('result')
            state['output_module_formals']=dict(module=int(module),destination_global=int(gdb.parse_and_eval('&output_module')),initial=int(module.dereference()))
        if state['function']in ('duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats'):
            if state['function']=='duplicate_link_string':
                initializer=frame.older();module=initializer.read_var('module');library=initializer.read_var('library');result=initializer.read_var('result');text=frame.read_var('text')
                state['argument_detail']=dict(address=int(text),text=text.string())
            else:
                module=gdb.parse_and_eval('output_module');library=module.dereference()['library'];result=gdb.parse_and_eval('&output_module')
                state['frontier_inputs']=dict(formals=[int(frame.read_var(k))for k in ('address','bytes','flags','protection')]if state['function']=='allocate_virtual_memory'else [],hidden_register_inputs=[])
                if state['function']=='allocate_virtual_memory':state['module_section_pools']={k:int(gdb.parse_and_eval(k))for k in ('module_section_map','module_section_headers','module_section_capacity','module_section_pools_ready')}
                if state['function']=='initialize_comdats':
                    state['module_section_pools']={k:int(gdb.parse_and_eval(k))for k in ('module_section_map','module_section_headers','module_section_capacity','module_section_pools_ready')}
                    state['module_section_bytes']=[gdb.selected_inferior().read_memory(state['module_section_pools'][k],size).tobytes().hex()for k,size in [('module_section_map',4000),('module_section_headers',20000)]]
                state['output_names']=[dict(address=int(v),bytes=gdb.selected_inferior().read_memory(int(v),len(v.string())+1).tobytes().hex())for v in (module.dereference()['name'],library.dereference()['name'])]
            state['module_constructor']=dict(result_slot=int(result),current=int(result.dereference()),global_slot=int(gdb.parse_and_eval('&output_module')),module=int(module),library=int(library),module_raw=gdb.selected_inferior().read_memory(int(module),156).tobytes().hex(),library_raw=gdb.selected_inferior().read_memory(int(library),68).tobytes().hex())
            state['output_module_arenas']=[]
            for name in ['output_module_arena','output_library_arena']:
                arena=gdb.parse_and_eval(name);address=int(arena);raw=gdb.selected_inferior().read_memory(address,16).tobytes();header=struct.unpack('<4I',raw)
                state['output_module_arenas'].append(dict(slot=int(gdb.parse_and_eval('&'+name)),address=address,raw=raw.hex(),data=gdb.selected_inferior().read_memory(header[3],header[1]+header[2]).tobytes().hex()))
    elif state['function'] in ('LinkContext::Initialize','create_string_table','initialize_hash_table','hash_allocate','reserve_memory','create_memory_arena','allocate_memory'):
        owner=frame
        while owner and owner.name()!='LinkContext::Initialize':owner=owner.older()
        assert owner
        state['context_this']=int(owner.read_var('this'));state['context_vxd']=int(owner.read_var('vxd'))
        state['caller_context']=int(driver.read_var('context'))
        state['link_context']=gdb.selected_inferior().read_memory(state['context_this'],664).tobytes().hex()
        if state['function']!='LinkContext::Initialize':state['context_extension']=owner.read_var('this').dereference()['options']['default_extension'].string()
    if state['function']=='create_string_table':
        state['table_arguments']=dict(table=int(frame.read_var('table')),buckets=int(frame.read_var('buckets')),capacity=int(frame.read_var('capacity')))
    if state['function'] in ('initialize_hash_table','hash_allocate','reserve_memory','create_memory_arena','allocate_memory'):
        if state['function']=='initialize_hash_table':state['hash_arguments']=dict(hash=int(frame.read_var('hash')),buckets=int(frame.read_var('buckets')),capacity=int(frame.read_var('capacity')),key_name=int(frame.read_var('key_name')),flags=int(frame.read_var('flags')))
        state['string_table_address']=int(owner.read_var('this').dereference()['symbols'])
        state['string_table']=gdb.selected_inferior().read_memory(state['string_table_address'],40).tobytes().hex()
        state['string_key_name_address']=int(gdb.lookup_global_symbol('string_key_name').value().address)
    if state['function'] in ('hash_allocate','reserve_memory','create_memory_arena','allocate_memory'):
        allocator=frame
        while allocator and allocator.name()!='hash_allocate':allocator=allocator.older()
        assert allocator
        state['hash_state_address']=int(allocator.read_var('state'))
        state['hash_storage_arguments']=dict(state=state['hash_state_address'],arena=int(allocator.read_var('arena')),count=int(allocator.read_var('count')),size=int(allocator.read_var('size')))
        if state['function'] in ('reserve_memory','create_memory_arena','allocate_memory'):
            reserver=frame
            while reserver and reserver.name()!='reserve_memory':reserver=reserver.older()
            assert reserver
            state['arena_arguments']=dict(arena=int(reserver.read_var('arena')),bytes=int(reserver.read_var('bytes')),owner=int(reserver.read_var('arena').dereference()))
        if state['function'] in ('create_memory_arena','allocate_memory'):
            creator=frame
            while creator and creator.name()!='create_memory_arena':creator=creator.older()
            assert creator
            state['arena_creation_bytes']=int(creator.read_var('bytes'))
        if state['function']=='allocate_memory':state['allocation_bytes']=int(frame.read_var('bytes'))
        state['hash_state']=gdb.selected_inferior().read_memory(state['hash_state_address'],56).tobytes().hex()
    state['string_key_name_address']=int(gdb.lookup_global_symbol('string_key_name').value().address)
    state['member_pointers']={}
    for member in ('image_header_size','image_write_section','image_write_headers'):
        value=gdb.parse_and_eval(member);address=int(gdb.parse_and_eval('&'+member))
        state['member_pointers'][member]=dict(bytes=value.type.sizeof,raw=gdb.selected_inferior().read_memory(address,value.type.sizeof).tobytes().hex())
    state['member_methods']={};state['typed_member_values']={}
    for method in ('PeHeaderSize','VxdHeaderSize','PeWriteSection','VxdWriteSection','PeWriteHeaders','VxdWriteHeaders'):
        name='LinkContext::'+method
        symbol=gdb.lookup_global_symbol(name)
        assert symbol
        state['member_methods'][name]=int(symbol.value().address)
        state['typed_member_values'][name]=gdb.parse_and_eval('&'+name).bytes.hex()
    state['default_dos_image_address']=int(gdb.parse_and_eval('&default_dos_image'))
    state['default_dos_image']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&default_dos_image')),128).tobytes().hex()
    state['option_arguments_address']=int(gdb.parse_and_eval('&option_arguments'))
    state['option_arguments']=[int(gdb.parse_and_eval('option_arguments.'+name)) for name in ('first','last','count')]
    if state['option_arguments'][0]:state['option_node']=gdb.selected_inferior().read_memory(state['option_arguments'][0],24).tobytes().hex()
    if state['function']=='has_switch':state['argument_detail']=dict(text=frame.read_var('name').string(),address=int(frame.read_var('name')))
    if state['function']=='parse_switch':state['argument_detail']=dict(file=int(frame.read_var('file')),text=frame.read_var('text').string(),address=int(frame.read_var('text')))
    if configuration and state['function'] in ('duplicate_argument','release_memory','linker_usage','print_logo','close_timing','free_buffer4','free_buffer8','free_buffer12'):
        if state['function']=='duplicate_argument':state['argument_detail']=dict(text=frame.read_var('text').string(),address=int(frame.read_var('text')))
        info=configuration.read_var('info')
        state['parsed_switch_address']=int(info)
        state['parsed_switch']=gdb.selected_inferior().read_memory(int(info),10).tobytes().hex()
        if state['function']=='release_memory':
            size=9+len(info.dereference()['name'].string())+8*int(info.dereference()['value_count'])
            state['parsed_switch']=gdb.selected_inferior().read_memory(int(info),size).tobytes().hex()
            address=int(frame.read_var('memory'));text=gdb.Value(address).cast(gdb.lookup_type('char').pointer()).string()
            state['argument_detail']=dict(address=address,text=text)
            state['release_bytes']=gdb.selected_inferior().read_memory(address,len(text)+1).tobytes().hex()
            state['duplicate_result']=int(configuration.read_var('repro_option'))
        if state['function'] in ('linker_usage','print_logo','close_timing','free_buffer4','free_buffer8','free_buffer12'):
            copy=configuration.read_var('repro_option')
            state['duplicate_result']=int(copy)
            state['duplicated_option']=gdb.selected_inferior().read_memory(int(copy),2).tobytes().hex()


    if state['function']=='allocate_link_memory':state['allocation_arguments']=dict(count=int(frame.read_var('count')),size=int(frame.read_var('size')))
    state['logo_pending']=int(gdb.parse_and_eval('logo_pending'))
    state['incremental_enabled']=int(gdb.parse_and_eval('incremental_enabled'))
    state['argument_tail']=int(gdb.parse_and_eval('argument_tail'))
    if state['function']=='search_library_paths':
        owner=frame.older();assert owner.name()=='canonicalize_argument'
        state['argument_detail']=dict(text=frame.read_var('name').string(),address=int(frame.read_var('name')),found=int(frame.read_var('found')))
        state['canonicalize_components']={name:owner.read_var(name).string()for name in ('drive','directory','filename','suffix')}
        state['canonicalize_found_address']=int(owner.read_var('found').address)
    if state['function']=='canonicalize_argument':state['argument_detail']=dict(text=frame.read_var('text').string(),address=int(frame.read_var('text')),extension=frame.read_var('extension').string(),extension_address=int(frame.read_var('extension')))
    if state['function'] in ('parse_argument','is_libpath','append_argument','append_argument_record'):
        state['argument_detail']=dict(text=frame.read_var('text').string(),address=int(frame.read_var('text')))
        if state['function']=='parse_argument':state['argument_detail']['source']=int(frame.read_var('source'))
        if state['function'] in ('append_argument','append_argument_record'):state['argument_detail']['list']=int(frame.read_var('list'))
        if state['function']=='append_argument_record':state['argument_detail']['original_address']=int(frame.read_var('original_name'))
    state['input_prepass']=int(gdb.parse_and_eval('input_prepass'))
    state['input_files']=[int(gdb.parse_and_eval('input_files.'+name)) for name in ('first','last','count')]
    state['argument_lists']={}
    for name in ('input_files','option_arguments','link_files'):
        value=gdb.parse_and_eval(name);nodes=[];node=value['first'];seen=set()
        while int(node):
            address=int(node);assert address not in seen and len(seen)<64;seen.add(address)
            words=struct.unpack('<6I',gdb.selected_inferior().read_memory(address,24).tobytes())
            nodes.append(dict(address=address,words=list(words),text=node.dereference()['name'].string(),original=node.dereference()['original_name'].string()))
            node=node.dereference()['next']
        state['argument_lists'][name]=dict(header=[int(value[key])for key in ('first','last','count')],nodes=nodes)

    state['repro_directory']=int(gdb.parse_and_eval('repro_directory'))
    state['repro_response']=int(gdb.parse_and_eval('repro_response'))
    if state['function'] in ('collect_file_arguments','parse_command_line'):
        state['environment_arguments']=[frame.read_var('before').string(),frame.read_var('after').string()]
    if state['function']=='zero_allocate':state['frontier_arguments']=dict(bytes=int(frame.read_var('bytes')))
    if int(gdb.parse_and_eval('program_name')):
        state['program_name_address']=int(gdb.parse_and_eval('program_name'))
        state['program_name']=gdb.parse_and_eval('program_name').string()
        owner=frame
        while owner and owner.name()!='link_main':owner=owner.older()
        assert owner
        state['basename_address']=int(owner.read_var('basename').address)
    state['allocation_page_size']=int(gdb.parse_and_eval('allocation_page_size'))
    state['maximum_physical_memory']=int(gdb.parse_and_eval('maximum_physical_memory'))
    state['file_globals']={name:int(gdb.parse_and_eval(name)) for name in ('file_slots','file_newest','file_record_count','file_window_limit','file_mapping_enabled')}
    if state['file_globals']['file_slots']:
        raw_slots=gdb.selected_inferior().read_memory(state['file_globals']['file_slots'],64).tobytes()
        state['file_slots_bytes']=raw_slots.hex()
        slots=[int.from_bytes(raw_slots[i:i+4],'little') for i in range(0,64,4)]
        state['file_records']=[gdb.selected_inferior().read_memory(pointer,40).tobytes().hex() if pointer else None for pointer in slots]
    state['file_statistics']=[int(gdb.parse_and_eval('file_statistics[%%d]'%%i)) for i in range(15)]
    state['debug_flags']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&debug_flags')),256).tobytes().hex()
    state['app_globals']={}
    for symbol,size in [('buffer12',12),('buffer8',12),('buffer4',12),('timing',20)]:
        address=int(gdb.parse_and_eval('&'+symbol))
        state['app_globals'][symbol]=gdb.selected_inferior().read_memory(address,size).tobytes().hex()
    frame=gdb.newest_frame()
    while frame and frame.name()!='recon_initterm':frame=frame.older()
    if frame:
        frame.select()
        current=int(gdb.parse_and_eval('begin'))
        end=int(gdb.parse_and_eval('end'))
        start=int(gdb.parse_and_eval('&application_initializers[0]'))
        state['initerm_current_index']=(current-start)//4
        state['initerm_end_index']=(end-start)//4
        gdb.newest_frame().select()
    frame=gdb.newest_frame()
    while frame and frame.name()!='target_startup':frame=frame.older()
    if frame:
        frame.select()
        count=int(gdb.parse_and_eval('argc'))
        state['crt_argv_address']=int(gdb.parse_and_eval('argv'))
        state['crt_first_argument_address']=int(gdb.parse_and_eval('argv[1]'))
        state['crt_argv']=[gdb.parse_and_eval('argv[%%d]'%%i).string() for i in range(count)]
        state['crt_argv_addresses']=[int(gdb.parse_and_eval('argv[%%d]'%%i))for i in range(count)]
        gdb.newest_frame().select()
    state['stack']=[int(gdb.parse_and_eval('((unsigned*)$esp)[%%d]'%%i)) for i in range(32)]
    state['snapshot_complete']=True
def exited(event):
    state['exit_status']=getattr(event,'exit_code',None)
gdb.events.stop.connect(stopped)
gdb.events.exited.connect(exited)
gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute(%r)
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
''' % (targets, 'run '+shlex.join(arguments)+' > '+shlex.quote(str(capture/'stdout.bin'))+' 2> '+shlex.quote(str(capture/'stderr.bin')), str(capture / 'state.json'))
    (capture / 'capture.gdb.py').write_text(script)
    executable = BINARY / 'build/native/LINK.EXE'
    command = ['gdb', '-q', '-nx', '-batch', '-ex', 'source ' + str(capture / 'capture.gdb.py'),
               '--args', str(executable)] + arguments
    start = time.perf_counter()
    environment = os.environ.copy()
    for name in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_DB','LINK_REPRO','LINK_REPRO_NAME']:environment.pop(name,None)
    environment['RECON_PROGRAM_PATH'] = r'R:\toolchains\5.10\bin\LINK.EXE'
    if '_initialize_memory' in configuration['accepted']:
        assert args.environment_reference, 'explicit cached environment reference is required'
        from environment_profile import prepare
        profile,reference=prepare(args.environment_reference,capture)
        environment['RECON_SYSTEM_PROFILE']=str(profile)
        environment['RECON_SYSTEM_REFERENCE']=reference
    environment.update(native_environment(args.case))
    with (nullcontext(capture)if args.case=='help'else workspace(args.case,manifest))as work:
        result = subprocess.run(command,cwd=work,env=environment,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=20)
        if args.case!='help':
            from case_inputs import save_artifacts
            save_artifacts(work,capture,manifest)
    elapsed = time.perf_counter() - start
    assert all(sha(path.read_bytes())==source_content[name] for path,name in content_paths), 'compiled sources changed during native capture'
    subprocess.run(['make','-q','native'],cwd=BINARY,check=True)
    (capture / 'gdb.stdout').write_bytes(result.stdout); (capture / 'gdb.stderr').write_bytes(result.stderr)
    receipt = dict(kind='native-i386-gdb-frontier',case=args.case,case_manifest=manifest,working_directory=(str(capture)if args.case=='help'else windows_work(args.case)),configuration_sha256=sha(config),
                   native_sha256=sha(executable.read_bytes()),
                   rebuilt_sha256=sha((BINARY / 'build/link.exe').read_bytes()),
                   source_files={str(p.relative_to(BINARY)):sha(p.read_bytes())
                                 for p in sorted(p for p in (BINARY/'src').iterdir() if p.suffix in ('.c','.h','.cpp'))},
                   source_content=source_content,
                   command=command, status=result.returncode, elapsed_seconds=elapsed,
                   files={str(p.relative_to(capture)):dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes()))
                          for p in capture.rglob('*') if p.is_file() and 'source' not in p.relative_to(capture).parts})
    (capture / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(capture)
    state=json.loads((capture/'state.json').read_text())if (capture/'state.json').exists()else {}
    if result.returncode or not state or (state.get('signal_number')and not state.get('snapshot_complete')):
        raise SystemExit('native debugger capture failed')


if __name__ == '__main__': main()
