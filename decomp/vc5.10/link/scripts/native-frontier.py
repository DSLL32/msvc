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
capture_directory=__NATIVE_CAPTURE_DIRECTORY__
state={'entries':[],'initerm_calls':[],'operator_new_calls':[],'atexit_calls':[],'console_calls':[],'module_probes':[],'system_calls':[],'mbsnbicmp_calls':[],'virtual_alloc_calls':[],'parser_allocations':[],'duplicate_calls':[],'timing_end_calls':[],'delete_calls':[],'new_ownership':[],'mbspbrk_calls':[],'canonicalized_arguments':[],'library_search_calls':[],'searchenv_calls':[],'free_calls':[],'link_string_calls':[],'virtual_memory_calls':[],'section_flag_calls':[],'section_enum_calls':[],'section_next_calls':[],'section_end_calls':[],'section_lookup_calls':[],'section_create_calls':[],'section_resolve_calls':[],'group_enum_calls':[],'group_find_calls':[],'group_next_calls':[],'group_end_calls':[]}
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
class CollisionReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')))
        state['collision_call']=self.record;return False
class CollisionEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        record=dict(output=int(args[1]),inputs=int(args[2]),entry_stack=int(gdb.parse_and_eval('$esp')))
        CollisionReturned(gdb.newest_frame(),record);return False
CollisionEntry('*check_output_input_collision',internal=True)
class CollisionPathReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')),after=gdb.selected_inferior().read_memory(self.record['output'],260).tobytes().hex(),after_errors=errors())
        self.record['text']=gdb.Value(self.record['output']).cast(gdb.lookup_type('char').pointer()).string()
        state.setdefault('collision_path_calls',[]).append(self.record);return False
class CollisionPathEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if not caller or caller.name()!='check_output_input_collision':return False
        args=gdb.parse_and_eval('(unsigned*)$esp');out=int(args[1]);name=int(args[2]);limit=int(args[3])
        record=dict(local_output=int(caller.read_var('output_path').address),local_input=int(caller.read_var('input_path').address),local_sizes=[caller.read_var(k).type.sizeof for k in ('output_path','input_path')],output=out,name=name,capacity=limit,input=gdb.Value(name).cast(gdb.lookup_type('char').pointer()).string(),entry_stack=int(gdb.parse_and_eval('$esp')),before=gdb.selected_inferior().read_memory(out,260).tobytes().hex(),before_errors=errors())
        CollisionPathReturned(frame,record);return False
CollisionPathEntry('*recon_fullpath',internal=True)
class CollisionCompareReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')),after_errors=errors())
        state.setdefault('collision_compare_calls',[]).append(self.record);return False
class CollisionCompareEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if not caller or caller.name()!='check_output_input_collision':return False
        args=gdb.parse_and_eval('(unsigned*)$esp');left=int(args[1]);right=int(args[2])
        record=dict(left=left,right=right,left_text=gdb.Value(left).cast(gdb.lookup_type('char').pointer()).string(),right_text=gdb.Value(right).cast(gdb.lookup_type('char').pointer()).string(),entry_stack=int(gdb.parse_and_eval('$esp')),before_errors=errors(),codepage=int(gdb.parse_and_eval('recon_mb_codepage')),lc_ctype_handle=int(gdb.parse_and_eval('recon_lc_handle[2]')),mb_cur_max=int(gdb.parse_and_eval('recon_mb_cur_max')))
        CollisionCompareReturned(frame,record);return False
CollisionCompareEntry('*recon_mbsicmp',internal=True)
class LibraryFindEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['library_find_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),raw_formals=[int(args[i])for i in (1,2,3)],hidden_inputs=[]);LibraryFindReturned(gdb.newest_frame());return False
LibraryFindEntry('*find_library',internal=True)
class LibraryEnumReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),after=gdb.selected_inferior().read_memory(self.record['formals'][0],16).tobytes().hex())
        state['library_enum_return']=self.record
        return False
class LibraryEnumEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');formals=[int(args[i])for i in (1,2)]
        state['library_enum_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=formals,hidden_inputs=[],before=gdb.selected_inferior().read_memory(formals[0],16).tobytes().hex())
        LibraryEnumReturned(gdb.newest_frame(),state['library_enum_entry'].copy())
        return False
LibraryEnumEntry('*initialize_library_enumerator',internal=True)
class LibraryNextReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        raw=int(gdb.parse_and_eval('$eax'))
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=raw,result=raw&255,after=gdb.selected_inferior().read_memory(self.record['formals'][0],16).tobytes().hex())
        state['library_next_return']=self.record
        return False
class LibraryNextEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['library_next_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1])],hidden_inputs=[])
        LibraryNextReturned(gdb.newest_frame(),state['library_next_entry'].copy());return False
LibraryNextEntry('*next_library',internal=True)
class LibraryFindReturned(gdb.FinishBreakpoint):
    def __init__(self,frame):super().__init__(frame,internal=True)
    def stop(self):
        state['library_find_return']=dict(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True);return False
class LibraryCreateReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        result=int(self.return_value);raw=gdb.selected_inferior().read_memory(result,68).tobytes();array=struct.unpack_from('<I',raw,4)[0]
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=result,record=raw.hex(),modules=array,module_array=gdb.selected_inferior().read_memory(array,20).tobytes().hex(),list_after=gdb.selected_inferior().read_memory(self.record['formals'][2],16).tobytes().hex())
        state['library_create_return']=self.record;return False
class LibraryCreateEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');formals=[int(args[i])for i in (1,2,3)];state['library_create_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=formals,hidden_inputs=[])
        state['library_list_before']=gdb.selected_inferior().read_memory(formals[2],16).tobytes().hex()
        LibraryCreateReturned(gdb.newest_frame(),state['library_create_entry'].copy());return False
LibraryCreateEntry('*create_library',internal=True)
class LoadInputEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');argument=int(args[1]);name=gdb.Value(argument).cast(gdb.lookup_type('InputArgument').pointer()).dereference()['name'].string()
        assert len(name.encode('latin1'))+1<=32,'unknown longer diagnostic-name consumer:32B modeled prefix only; original capacity unproved'
        state['input_diagnostic_before']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&current_phase')),32).tobytes().hex()
        state['incremental_status_entry']=dict(code=int(gdb.parse_and_eval('incremental_status_code')),reuse=int(gdb.parse_and_eval('incremental_reuse')),code_bytes=4,reuse_bytes=1)
        state['load_input_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),this=int(gdb.parse_and_eval('$ecx')),formals=[argument,int(args[2])],other_hidden_inputs=[]);return False
LoadInputEntry('*_ZN11LinkContext17LoadInputArgumentEP13InputArgumentP13OutputLibrary',internal=True)
class ReadInputEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['read_input_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[i])for i in (1,2,3)],hidden_inputs=[]);return False
ReadInputEntry('*read_input_argument',internal=True)
class ArchiveReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        raw=int(gdb.parse_and_eval('$eax'))&0xffffffff
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=raw,result=raw&255)
        state.setdefault('archive_returns',[]).append(self.record);return False
class ArchiveEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['archive_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1]),int(args[2])],hidden_inputs=[]);ArchiveReturned(gdb.newest_frame(),state['archive_entry'].copy());return False
ArchiveEntry('*is_archive',internal=True)
class ArchiveReadReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record,output):super().__init__(frame,internal=True);self.record=record;self.output=output
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value))
        self.output['after']=gdb.selected_inferior().read_memory(self.record['formals'][1],8).tobytes().hex()
        self.output['record_after']=gdb.selected_inferior().read_memory(self.record['formals'][0],40).tobytes().hex()
        self.record['output']=self.output;state.setdefault('archive_read_returns',[]).append(self.record);return False
class BigEndianReadReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record,output):super().__init__(frame,internal=True);self.record=record;self.output=output
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value))
        self.output['after']=gdb.selected_inferior().read_memory(self.record['formals'][1],4).tobytes().hex()
        self.output['record_after']=gdb.selected_inferior().read_memory(self.record['formals'][0],40).tobytes().hex()
        self.record['output']=self.output;state.setdefault('big_endian_read_returns',[]).append(self.record);return False
class DetectReadReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record,output):super().__init__(frame,internal=True);self.record=record;self.output=output
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value))
        self.output['after']=gdb.selected_inferior().read_memory(self.record['formals'][1],4).tobytes().hex()
        self.output['record_after']=gdb.selected_inferior().read_memory(self.record['formals'][0],40).tobytes().hex()
        self.record['output']=self.output;state.setdefault('detect_read_returns',[]).append(self.record);return False
class CoffReadReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record,output):super().__init__(frame,internal=True);self.record=record;self.output=output
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value))
        self.output['after']=gdb.selected_inferior().read_memory(self.record['formals'][1],20).tobytes().hex()
        self.output['record_after']=gdb.selected_inferior().read_memory(self.record['formals'][0],40).tobytes().hex()
        self.record['output']=self.output;state.setdefault('coff_read_returns',[]).append(self.record);return False
class MappedReadEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');values=[int(args[i])for i in (1,2,3)]
        entry=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=values,hidden_inputs=[])
        caller=gdb.newest_frame().older()
        if caller.name()=='is_archive':
            state.setdefault('mapped_read_entry',entry.copy());local=caller.read_var('signature')
            state['archive_output']=dict(address=values[1],bytes=values[2],owner_local_address=int(local.address),owner_local_bytes=local.type.sizeof,before=gdb.selected_inferior().read_memory(values[1],8).tobytes().hex(),initial_value_contract='uninitialized output array; no incoming bytes consumed')
            ArchiveReadReturned(gdb.newest_frame(),entry.copy(),state['archive_output'].copy())
        elif caller.name()=='read_big_endian_dword':
            state['big_endian_read_entry']=entry.copy();local=caller.read_var('value')
            state['big_endian_output']=dict(address=values[1],bytes=values[2],owner_local_address=int(local.address),owner_local_bytes=local.type.sizeof,before=gdb.selected_inferior().read_memory(values[1],4).tobytes().hex(),initial_value_contract='uninitialized DWORD output; no incoming bytes consumed')
            BigEndianReadReturned(gdb.newest_frame(),entry.copy(),state['big_endian_output'].copy())
        elif caller.name()=='detect_input_machine':
            state['detect_read_entry']=entry.copy();local=caller.read_var('signature')
            state['detect_output']=dict(address=values[1],bytes=values[2],owner_local_address=int(local.address),owner_local_bytes=local.type.sizeof,before=gdb.selected_inferior().read_memory(values[1],4).tobytes().hex(),initial_value_contract='uninitialized DWORD signature output; no incoming bytes consumed')
            DetectReadReturned(gdb.newest_frame(),entry.copy(),state['detect_output'].copy())
        elif caller.name()=='read_coff_header':
            owner=caller.older();local=owner.read_var('header');state['coff_read_entry']=entry.copy()
            state['coff_read_output']=dict(address=values[1],bytes=values[2],owner_local_address=int(local.address),owner_local_bytes=local.type.sizeof,before=gdb.selected_inferior().read_memory(values[1],20).tobytes().hex(),initial_value_contract='uninitialized IMAGE_FILE_HEADER output; incoming bytes not compared')
            CoffReadReturned(gdb.newest_frame(),entry.copy(),state['coff_read_output'].copy())
        return False
MappedReadEntry('*mapped_read',internal=True)
class MacResourceReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        raw=int(gdb.parse_and_eval('$eax'))&0xffffffff
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=raw,result=raw&255)
        state.setdefault('mac_resource_returns',[]).append(self.record);return False
class HeaderReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,key,record):super().__init__(frame,internal=True);self.key=key;self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value)&0xffffffff)
        state.setdefault(self.key,[]).append(self.record);return False
class ObjectReaderEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['object_reader_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[i])for i in (1,2,3)],hidden_inputs=[])
        state['object_read_mode']=dict(address=int(gdb.parse_and_eval('&machine_initialized')),bytes=1,value=int(gdb.parse_and_eval('machine_initialized')));return False
ObjectReaderEntry('*read_object_file',internal=True)
class InputMachineReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=int(gdb.parse_and_eval('$eax'))&0xffffffff,context_after=gdb.selected_inferior().read_memory(self.record['formals'][0],664).tobytes().hex())
        state.setdefault('input_machine_returns',[]).append(self.record);return False
class DetectMachineReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        raw=int(gdb.parse_and_eval('$eax'))&0xffffffff
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=raw,result=raw&0xffff)
        state.setdefault('detect_machine_returns',[]).append(self.record);return False
class InputMachineEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['input_machine_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1]),int(args[2])],hidden_inputs=[]);state['input_machine_context_before']=gdb.selected_inferior().read_memory(int(args[1]),664).tobytes().hex();InputMachineReturned(gdb.newest_frame(),state['input_machine_entry'].copy());return False
InputMachineEntry('*check_input_machine',internal=True)
class DetectMachineEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['detect_machine_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1]),int(args[2])],hidden_inputs=[]);DetectMachineReturned(gdb.newest_frame(),state['detect_machine_entry'].copy());return False
DetectMachineEntry('*detect_input_machine',internal=True)
class InitializeMachineReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=int(gdb.parse_and_eval('$eax'))&0xffffffff,context_after=gdb.selected_inferior().read_memory(self.record['formals'][0],664).tobytes().hex(),machine_initialized=int(gdb.parse_and_eval('machine_initialized')))
        state.setdefault('initialize_machine_returns',[]).append(self.record);return False
class InitializeMachineEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['initialize_machine_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1])],hidden_inputs=[]);state['initialize_machine_context_before']=gdb.selected_inferior().read_memory(int(args[1]),664).tobytes().hex();InitializeMachineReturned(gdb.newest_frame(),state['initialize_machine_entry'].copy());return False
InitializeMachineEntry('*initialize_machine',internal=True)
class ValidateDelayReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=int(gdb.parse_and_eval('$eax'))&0xffffffff,context_after=gdb.selected_inferior().read_memory(self.record['formals'][0],664).tobytes().hex())
        state.setdefault('validate_delay_returns',[]).append(self.record);return False
class ValidateDelayEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['validate_delay_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1])],hidden_inputs=[])
        caller=gdb.newest_frame().older();local=caller.read_var('incremental_supported')
        state['incremental_support_output']=dict(address=int(local.address),bytes=local.type.sizeof,value=int(local))
        state['machine_init_globals']=dict(delay_libraries=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&delay_libraries')),12).tobytes().hex(),small_section_alignment=int(gdb.parse_and_eval('small_section_alignment')),entry_symbol=int(gdb.parse_and_eval('entry_symbol')),entry_point_name=int(gdb.parse_and_eval('entry_point_name')))
        ValidateDelayReturned(gdb.newest_frame(),state['validate_delay_entry'].copy());return False
ValidateDelayEntry('*validate_delay_load_options',internal=True)
class X86BackendReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record,output):super().__init__(frame,internal=True);self.record=record;self.output=output
    def stop(self):
        self.output['after']=gdb.selected_inferior().read_memory(self.record['formals'][1],1).tobytes().hex()
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=int(gdb.parse_and_eval('$eax'))&0xffffffff,context_after=gdb.selected_inferior().read_memory(self.record['formals'][0],664).tobytes().hex(),output=self.output,process_relocations=int(gdb.parse_and_eval('process_relocations')),classify_relocation=int(gdb.parse_and_eval('classify_relocation')))
        state.setdefault('x86_backend_returns',[]).append(self.record);return False
class X86BackendEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['x86_backend_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1]),int(args[2])],hidden_inputs=[])
        caller=gdb.newest_frame().older();local=caller.read_var('incremental_supported')
        state['x86_incremental_support_output']=dict(address=int(args[2]),owner_local_address=int(local.address),bytes=1,owner_local_bytes=local.type.sizeof,before=gdb.selected_inferior().read_memory(int(args[2]),1).tobytes().hex())
        state['relocation_callbacks_before']=dict(process_relocations=int(gdb.parse_and_eval('process_relocations')),classify_relocation=int(gdb.parse_and_eval('classify_relocation')))
        X86BackendReturned(gdb.newest_frame(),state['x86_backend_entry'].copy(),state['x86_incremental_support_output'].copy());return False
X86BackendEntry('*initialize_x86_backend',internal=True)
class CoffHeaderReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        raw=int(gdb.parse_and_eval('$eax'))&0xffffffff
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),raw_result=raw,result=raw&255,header_after=gdb.selected_inferior().read_memory(self.record['formals'][1],20).tobytes().hex())
        state.setdefault('coff_header_returns',[]).append(self.record);return False
class CoffHeaderEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['coff_header_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[i])for i in (1,2,3,4)],hidden_inputs=[])
        caller=gdb.newest_frame().older();local=caller.read_var('header')
        state['coff_header_output']=dict(address=int(args[2]),bytes=20,owner_local_address=int(local.address),owner_local_bytes=local.type.sizeof,before=gdb.selected_inferior().read_memory(int(args[2]),20).tobytes().hex(),initial_value_contract='uninitialized IMAGE_FILE_HEADER output; incoming bytes not compared')
        state['current_machine_state']=dict(machine_initialized=int(gdb.parse_and_eval('machine_initialized')),process_relocations=int(gdb.parse_and_eval('process_relocations')),classify_relocation=int(gdb.parse_and_eval('classify_relocation')),small_section_alignment=int(gdb.parse_and_eval('small_section_alignment')))
        CoffHeaderReturned(gdb.newest_frame(),state['coff_header_entry'].copy());return False
CoffHeaderEntry('*read_coff_header',internal=True)
class InputModuleEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['input_module_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[i])for i in range(1,13)],hidden_inputs=[])
        caller=gdb.newest_frame().older();local=caller.read_var('created')
        state['input_module_created_output']=dict(address=int(args[6]),bytes=1,owner_local_address=int(local.address),owner_local_bytes=local.type.sizeof,before=gdb.selected_inferior().read_memory(int(args[6]),1).tobytes().hex(),initial_value_contract='BYTE created output; own incoming byte retained, writes before consumption qualified separately')
        state['current_machine_state']=dict(machine_initialized=int(gdb.parse_and_eval('machine_initialized')),process_relocations=int(gdb.parse_and_eval('process_relocations')),classify_relocation=int(gdb.parse_and_eval('classify_relocation')),small_section_alignment=int(gdb.parse_and_eval('small_section_alignment')))
        return False
InputModuleEntry('*create_input_module',internal=True)
class FindModuleEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        state['module_find_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),this=int(gdb.parse_and_eval('$ecx'))&0xffffffff,formals=[int(args[1]),int(args[2])],other_hidden_inputs=[])
        state['module_find_state']=dict(header=gdb.selected_inferior().read_memory(state['input_module_entry']['formals'][3],20).tobytes().hex(),created=gdb.selected_inferior().read_memory(state['input_module_entry']['formals'][5],1).tobytes().hex(),mac_target=int(gdb.parse_and_eval('mac_target')),native_ppc_symbol_maps=int(gdb.parse_and_eval('native_ppc_symbol_maps')))
        return False
FindModuleEntry('*_ZN13OutputLibrary10FindModuleEPKcPKv',internal=True)

class MacResourceEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['mac_resource_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1])],hidden_inputs=[]);MacResourceReturned(gdb.newest_frame(),state['mac_resource_entry'].copy());return False
MacResourceEntry('*is_mac_resource_file',internal=True)
class FileLengthReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        record=int(gdb.parse_and_eval('file_slots[%%d]'%%self.record['formals'][0]))
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value)&0xffffffff,record_after=gdb.selected_inferior().read_memory(record,40).tobytes().hex())
        state.setdefault('file_length_returns',[]).append(self.record);return False
class FileLengthEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');record=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1])],hidden_inputs=[])
        state.setdefault('file_length_entry',record.copy());FileLengthReturned(gdb.newest_frame(),record);return False
FileLengthEntry('*get_file_length',internal=True)
class SeekFileReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value)&0xffffffff,record_after=gdb.selected_inferior().read_memory(self.record['record'],40).tobytes().hex(),error_after=gdb.selected_inferior().read_memory(self.record['formals'][3],4).tobytes().hex())
        state.setdefault('seek_file_calls',[]).append(self.record);return False
class SeekFileEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');values=[int(args[i])for i in (1,2,3,4)]
        entry=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=values,hidden_inputs=[])
        state.setdefault('seek_file_entry',entry.copy());caller=gdb.newest_frame().older()
        if caller.name()=='get_file_length':local_name='current_error'if values[2]==1 else'end_error'
        elif caller.name()in ('is_mac_resource_file','detect_input_machine','read_object_file'):local_name='error'
        else:raise RuntimeError('unobserved seek caller owner '+caller.name())
        local=caller.read_var(local_name);before=gdb.selected_inferior().read_memory(values[3],4).tobytes().hex()
        output=dict(address=values[3],bytes=4,owner_local_address=int(local.address),owner_local_bytes=local.type.sizeof,before=before,initial_value_contract='initialized DWORD error scratch; no incoming-value assumption at pending seek')
        state.setdefault('file_length_output',output.copy())
        output['owner_kind']=caller.name()+'.'+local_name
        record=int(gdb.parse_and_eval('file_slots[%%d]'%%values[0]))
        call=dict(entry,caller=caller.name(),record=record,record_before=gdb.selected_inferior().read_memory(record,40).tobytes().hex(),output=output)
        SeekFileReturned(gdb.newest_frame(),call);return False
SeekFileEntry('*seek_file',internal=True)
class SeekMappedReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value)&0xffffffff,record_after=gdb.selected_inferior().read_memory(self.record['formals'][0],40).tobytes().hex(),error_after=gdb.selected_inferior().read_memory(self.record['formals'][3],4).tobytes().hex())
        state.setdefault('mapped_seek_calls',[]).append(self.record);return False
class SeekMappedEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');values=[int(args[i])for i in (1,2,3,4)];entry=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=values,hidden_inputs=[])
        state.setdefault('seek_mapped_entry',entry.copy())
        record=dict(entry,record_before=gdb.selected_inferior().read_memory(values[0],40).tobytes().hex(),error_before=gdb.selected_inferior().read_memory(values[3],4).tobytes().hex())
        SeekMappedReturned(gdb.newest_frame(),record);return False
SeekMappedEntry('*seek_mapped_file',internal=True)
class BigEndianEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['big_endian_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1])],hidden_inputs=[]);HeaderReturned(gdb.newest_frame(),'big_endian_returns',state['big_endian_entry'].copy());return False
BigEndianEntry('*read_big_endian_dword',internal=True)
class ByteSwapEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['byte_swap_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[1])],hidden_inputs=[]);HeaderReturned(gdb.newest_frame(),'byte_swap_returns',state['byte_swap_entry'].copy());return False
ByteSwapEntry('*byte_swap_dword',internal=True)
class MappingAPIReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        result=int(self.return_value);self.record.update(result=result,return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True,after_errors=errors())
        if self.record['kind']=='view'and result:
            view=dict(address=result,bytes=self.record['arguments'][4],mapping_index=self.record['mapping_index'])
            state.setdefault('created_views',[]).append(view)
            state.setdefault('created_view',dict(address=result,bytes=view['bytes']))
        if self.record['kind']=='close'and result and 'created_view'in state:
            view=state['created_views'][-1];data=gdb.selected_inferior().read_memory(view['address'],view['bytes']).tobytes()
            filename='created-mapping-view.bin'if view['mapping_index']==0 else 'mapping-%%02d-view.bin'%%view['mapping_index']
            open(capture_directory+'/'+filename,'wb').write(data)
        return False
class MappingAPIEntry(gdb.Breakpoint):
    def __init__(self,symbol,kind,count):super().__init__('*'+symbol,internal=True);self.kind=kind;self.count=count
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller and caller.name()=='initialize_file_mapping':
            args=gdb.parse_and_eval('(unsigned*)$esp');record=dict(kind=self.kind,mapping_index=len(state.get('mapping_entries',[]))-1,entry_stack=int(gdb.parse_and_eval('$esp')),arguments=[int(args[i])for i in range(1,self.count+1)],before_errors=errors())
            state.setdefault('mapping_api_calls',[]).append(record);MappingAPIReturned(frame,record)
        return False
for symbol,kind,count in [('recon_CreateFileMappingA','create',6),('recon_MapViewOfFileEx','view',6),('recon_CloseHandle','close',1)]:MappingAPIEntry(symbol,kind,count)
class MappedCreateReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True,after_errors=errors());state['mapped_create_call']=self.record;state.setdefault('mapped_create_calls',[]).append(self.record);return False
class MappedCreateEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller and caller.name()=='open_mapped_file':
            args=gdb.parse_and_eval('(unsigned*)$esp');values=[int(args[i])for i in range(1,8)]
            MappedCreateReturned(frame,dict(entry_stack=int(gdb.parse_and_eval('$esp')),arguments=values,text=gdb.Value(values[0]).cast(gdb.lookup_type('char').pointer()).string(),before_errors=errors()))
        return False
MappedCreateEntry('*recon_CreateFileA',internal=True)
class MappingReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):super().__init__(frame,internal=True);self.entry=entry
    def stop(self):
        state['mapping_return']=dict(entry_stack=self.entry['stack_pointer'],arguments=self.entry['raw_formals'],return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value),raw_eax=int(gdb.parse_and_eval('$eax')),error=int(gdb.parse_and_eval('*(unsigned*)'+str(self.entry['raw_formals'][2]))),normal_return=True);state.setdefault('mapping_returns',[]).append(state['mapping_return'].copy());return False
class MappedOpenReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):super().__init__(frame,internal=True);self.entry=entry
    def stop(self):
        caller=gdb.newest_frame();assert caller.name()=='open_argument_file'
        local=caller.read_var('created');size=caller.read_var('size');args=state['file_record_entry']['formals']
        formals=dict(name=args[0],flags=args[1],mode=args[2],created=args[3],initial_created=int(local),size_slot=int(size.address),initial_size=int(size),initial_mapped_result=int(caller.read_var('mapped')),file_access_statistics=int(gdb.parse_and_eval('file_access_statistics')),created_bytes=local.type.sizeof,local_created=int(local.address))
        state['mapped_open_return']=dict(entry_stack=self.entry['stack_pointer'],arguments=self.entry['raw_formals'],return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value),error=int(size),created=int(local),normal_return=True,caller_formals=formals);state.setdefault('mapped_open_returns',[]).append(state['mapped_open_return'].copy());return False
class OutputOpenReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,entry):super().__init__(frame,internal=True);self.entry=entry
    def stop(self):
        returned=dict(entry_stack=self.entry['stack_pointer'],arguments=self.entry['formals'],return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value),normal_return=True)
        state['input_open_return'if self.entry['formals'][1]==0x8000 else 'output_open_return']=returned
        state.setdefault('file_open_returns',[]).append(returned);return False
class MappingEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['mapping_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),raw_formals=[int(args[i])for i in (1,2,3)],hidden_inputs=[]);state.setdefault('mapping_entries',[]).append(state['mapping_entry'].copy());MappingReturned(gdb.newest_frame(),state['mapping_entry']);return False
MappingEntry('*initialize_file_mapping',internal=True)
class FileOpenEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        state['open_file_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[i])for i in (1,2,3)],hidden_inputs=[])
        state.setdefault('file_open_entries',[]).append(state['open_file_entry'].copy())
        OutputOpenReturned(gdb.newest_frame(),state['open_file_entry']);return False
FileOpenEntry('*open_argument_file',internal=True)
def file_cache_snapshot():
    table=int(gdb.parse_and_eval('file_slots'));raw=gdb.selected_inferior().read_memory(table,64).tobytes();slots=struct.unpack('<16I',raw)
    return dict(slots=list(slots),table=table,table_bytes=raw.hex(),records=[gdb.selected_inferior().read_memory(p,40).tobytes().hex()for p in slots],cache={k:int(gdb.parse_and_eval(k))for k in ('file_newest','pending_files','cached_files','cached_file_tail','cached_file_count')},global_slots={k:int(gdb.parse_and_eval('&'+k))for k in ('file_newest','pending_files')})
class FileFlowReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,kind,record):super().__init__(frame,internal=True);self.kind=kind;self.record=record
    def stop(self):
        self.record.update(return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True)
        if self.kind!='move':self.record['result']=int(self.return_value)
        self.record['after']=file_cache_snapshot()
        state.setdefault('file_flow_calls',{}).setdefault(self.kind,[]).append(self.record);return False
class FileMoveEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');record=dict(entry_stack=int(gdb.parse_and_eval('$esp')),arguments=[int(args[i])for i in range(1,7)],before=file_cache_snapshot())
        FileFlowReturned(gdb.newest_frame(),'move',record);return False
FileMoveEntry('*move_file_record',internal=True)
class FileAvailableEntry(gdb.Breakpoint):
    def stop(self):
        FileFlowReturned(gdb.newest_frame(),'available',dict(entry_stack=int(gdb.parse_and_eval('$esp')),arguments=[],before=file_cache_snapshot()));return False
FileAvailableEntry('*get_available_file_record',internal=True)
class FileAcquireReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        result=int(self.return_value);raw=gdb.selected_inferior().read_memory(result,40).tobytes();name=struct.unpack('<10I',raw)[0]
        self.record.update(result=result,raw=raw.hex(),name=name,name_bytes=gdb.selected_inferior().read_memory(name,10).tobytes().hex(),after_created=int(gdb.parse_and_eval('*(unsigned char*)'+str(self.record['arguments'][3]))),return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True)
        state['file_acquire_return']=self.record;return False
class FileNameReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        result=int(self.return_value);self.record.update(result=result,name_bytes=gdb.selected_inferior().read_memory(result,10).tobytes().hex(),return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True)
        state['file_name_call']=self.record;return False
class FileNameEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller and caller.name()=='acquire_file_record':
            args=gdb.parse_and_eval('(unsigned*)$esp');name=int(args[1]);FileNameReturned(frame,dict(entry_stack=int(gdb.parse_and_eval('$esp')),arguments=[name],text=gdb.Value(name).cast(gdb.lookup_type('char').pointer()).string()))
        return False
FileNameEntry('*duplicate_argument',internal=True)
class MappedFileEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');state['mapped_open_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),raw_formals=[int(args[i])for i in (1,2,3,4)],hidden_inputs=[])
        MappedOpenReturned(gdb.newest_frame(),state['mapped_open_entry']);return False
MappedFileEntry('*open_mapped_file',internal=True)
class FileRecordEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        state['file_record_entry']=dict(stack_pointer=int(gdb.parse_and_eval('$esp')),formals=[int(args[i])for i in (1,2,3,4)],hidden_inputs=[])
        FileAcquireReturned(gdb.newest_frame(),dict(entry_stack=state['file_record_entry']['stack_pointer'],arguments=state['file_record_entry']['formals'],before_created=int(gdb.parse_and_eval('*(unsigned char*)'+str(int(args[4]))))))
        return False
FileRecordEntry('*acquire_file_record',internal=True)
class FileLookupReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),normal_return=True,return_stack=int(gdb.parse_and_eval('$esp')))
        state.setdefault('file_lookup_calls',[]).append(self.record);return False
class FileLookupEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        record=dict(name=int(args[1]),entry_stack=int(gdb.parse_and_eval('$esp')))
        FileLookupReturned(gdb.newest_frame(),record);return False
FileLookupEntry('*find_cached_file',internal=True)
class PassOneEntry(gdb.Breakpoint):
    def stop(self):
        state['pass_one_entry']=dict(this=int(gdb.parse_and_eval('$ecx')),stack_pointer=int(gdb.parse_and_eval('$esp')),stack_formals=[],other_hidden_inputs=[])
        return False
PassOneEntry('*'+str(int(gdb.lookup_global_symbol('LinkContext::PassOne').value().address)),internal=True)

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
class LibraryArrayReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        result=int(self.return_value)
        self.record.update(result=result,return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True,raw=gdb.selected_inferior().read_memory(result,self.record['bytes']).tobytes().hex())
        state['library_array_allocation']=self.record;return False
class ParserAllocationEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        if frame.older() and frame.older().name()=='parse_switch':
            ParserAllocationReturned(frame,int(gdb.parse_and_eval('((unsigned*)$esp)[1]')))
        elif frame.older() and frame.older().name()=='create_library':
            LibraryArrayReturned(frame,dict(bytes=int(gdb.parse_and_eval('((unsigned*)$esp)[1]')),entry_stack=int(gdb.parse_and_eval('$esp'))))
        return False
ParserAllocationEntry('*allocate_memory',internal=True)
class SectionLookupReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record,key):super().__init__(frame,internal=True);self.record=record;self.key=key
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')))
        state[self.key].append(self.record);return False
class SectionLookupEntry(gdb.Breakpoint):
    def __init__(self,symbol,key):super().__init__(symbol,internal=True);self.key=key
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');values=[int(args[i])for i in range(1,6)]
        SectionLookupReturned(gdb.newest_frame(),dict(arguments=values,text=gdb.Value(values[1]).cast(gdb.lookup_type('char').pointer()).string(),entry_stack=int(gdb.parse_and_eval('$esp'))),self.key);return False
SectionLookupEntry('*find_matching_output_section','section_lookup_calls')
SectionLookupEntry('*create_output_section','section_create_calls')
class SectionResolveReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')))
        state['section_resolve_calls'].append(self.record);return False
class SectionResolveEntry(gdb.Breakpoint):
    def stop(self):
        pointer=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        SectionResolveReturned(gdb.newest_frame(),dict(section=pointer,entry_stack=int(gdb.parse_and_eval('$esp'))));return False
SectionResolveEntry('*resolve_output_section',internal=True)
class SectionEndReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record,key='section_end_calls'):super().__init__(frame,internal=True);self.record=record;self.key=key
    def stop(self):
        self.record.update(after=gdb.selected_inferior().read_memory(self.record['enumerator'],16).tobytes().hex(),return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True)
        state[self.key].append(self.record);return False
class SectionEndEntry(gdb.Breakpoint):
    def stop(self):
        pointer=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        SectionEndReturned(gdb.newest_frame(),dict(enumerator=pointer,entry_stack=int(gdb.parse_and_eval('$esp')),before=gdb.selected_inferior().read_memory(pointer,16).tobytes().hex()),'group_end_calls'if gdb.newest_frame().older().name()=='find_contribution_group'else 'section_end_calls');return False
SectionEndEntry('*end_section_enumerator',internal=True)
class SectionNextReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(after=gdb.selected_inferior().read_memory(self.record['enumerator'],16).tobytes().hex(),return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value))
        state['section_next_calls'].append(self.record);return False
class SectionNextEntry(gdb.Breakpoint):
    def stop(self):
        pointer=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        SectionNextReturned(gdb.newest_frame(),dict(enumerator=pointer,entry_stack=int(gdb.parse_and_eval('$esp')),before=gdb.selected_inferior().read_memory(pointer,16).tobytes().hex()));return False
SectionNextEntry('*next_output_section',internal=True)
class SectionEnumReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(after=gdb.selected_inferior().read_memory(self.record['enumerator'],16).tobytes().hex(),return_stack=int(gdb.parse_and_eval('$esp')))
        state['section_enum_calls'].append(self.record);return False
class SectionEnumEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');pointer=int(args[1])
        SectionEnumReturned(gdb.newest_frame(),dict(enumerator=pointer,sections=int(args[2]),entry_stack=int(gdb.parse_and_eval('$esp')),before=gdb.selected_inferior().read_memory(pointer,16).tobytes().hex()));return False
SectionEnumEntry('*initialize_section_enumerator',internal=True)
class GroupEnumReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(after=gdb.selected_inferior().read_memory(self.record['enumerator'],16).tobytes().hex(),return_stack=int(gdb.parse_and_eval('$esp')))
        state['group_enum_calls'].append(self.record);return False
class GroupEnumEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');pointer=int(args[1])
        GroupEnumReturned(gdb.newest_frame(),dict(enumerator=pointer,section=int(args[2]),owner_local=int(gdb.newest_frame().older().read_var('enumerator').address),entry_stack=int(gdb.parse_and_eval('$esp')),before=gdb.selected_inferior().read_memory(pointer,16).tobytes().hex()));return False
GroupEnumEntry('*initialize_group_enumerator',internal=True)
class GroupFindReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(result=int(self.return_value),return_stack=int(gdb.parse_and_eval('$esp')))
        state['group_find_calls'].append(self.record);return False
class GroupFindEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');name=int(args[2])
        GroupFindReturned(gdb.newest_frame(),dict(section=int(args[1]),name=name,text=gdb.Value(name).cast(gdb.lookup_type('char').pointer()).string(),entry_stack=int(gdb.parse_and_eval('$esp'))));return False
GroupFindEntry('*find_contribution_group',internal=True)
class GroupNextReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(after=gdb.selected_inferior().read_memory(self.record['enumerator'],16).tobytes().hex(),return_stack=int(gdb.parse_and_eval('$esp')),result=int(self.return_value)&255)
        state['group_next_calls'].append(self.record);return False
class GroupNextEntry(gdb.Breakpoint):
    def stop(self):
        pointer=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        GroupNextReturned(gdb.newest_frame(),dict(enumerator=pointer,entry_stack=int(gdb.parse_and_eval('$esp')),before=gdb.selected_inferior().read_memory(pointer,16).tobytes().hex()));return False
GroupNextEntry('*next_contribution_group',internal=True)
class SectionsInitializeReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(return_stack=int(gdb.parse_and_eval('$esp')),normal_return=True)
        state['sections_initializer_call']=self.record;return False
class SectionsInitializeEntry(gdb.Breakpoint):
    def stop(self):
        SectionsInitializeReturned(gdb.newest_frame(),dict(this=int(gdb.parse_and_eval('$ecx')),entry_stack=int(gdb.parse_and_eval('$esp'))));return False
SectionsInitializeEntry('*'+str(int(gdb.lookup_global_symbol('LinkContext::InitializeSections').value().address)),internal=True)
class SectionFlagsReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
    def stop(self):
        self.record.update(after=int(gdb.Value(self.record['flags']).cast(gdb.lookup_type('unsigned int').pointer()).dereference()),return_stack=int(gdb.parse_and_eval('$esp')))
        state['section_flag_calls'].append(self.record);return False
class SectionFlagsEntry(gdb.Breakpoint):
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp');pointer=int(args[1]);name=int(args[2]);header=int(args[3])
        caller=gdb.newest_frame().older();owner_kind=caller.name();slot=caller.read_var('normalized'if owner_kind=='find_matching_output_section'else 'flags').address
        SectionFlagsReturned(gdb.newest_frame(),dict(owner_kind=owner_kind,owner_slot=int(slot),flags=pointer,name=name,header=header,before=int(gdb.Value(pointer).cast(gdb.lookup_type('unsigned int').pointer()).dereference()),text=gdb.Value(name).cast(gdb.lookup_type('char').pointer()).string(),initialize_bss=int(gdb.parse_and_eval('initialize_bss')),subsystem=int(gdb.Value(header).cast(gdb.lookup_type('IMAGE_OPTIONAL_HEADER').pointer()).dereference()['Subsystem']),entry_stack=int(gdb.parse_and_eval('$esp'))));return False
SectionFlagsEntry('*adjust_output_section_flags',internal=True)
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
    if state['function']in ('create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):state['frontier_inputs']=dict(formals=state['library_create_entry']['formals'],hidden_register_inputs=[])
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
    if configuration or state['function']in ('validate_debug_options','LinkContext::FullLink','initialize_output_modules','duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
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
        if state['function']in ('validate_debug_options','LinkContext::FullLink','initialize_output_modules','duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
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
        if state['function']in ('duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
            if state['function']=='duplicate_link_string':
                initializer=frame.older();module=initializer.read_var('module');library=initializer.read_var('library');result=initializer.read_var('result');text=frame.read_var('text')
                state['argument_detail']=dict(address=int(text),text=text.string())
            else:
                module=gdb.parse_and_eval('output_module');library=module.dereference()['library'];result=gdb.parse_and_eval('&output_module')
                state['frontier_inputs']=dict(formals=[int(frame.read_var(k))for k in ('address','bytes','flags','protection')]if state['function']=='allocate_virtual_memory'else [],hidden_register_inputs=[])
                if state['function']=='allocate_virtual_memory':state['module_section_pools']={k:int(gdb.parse_and_eval(k))for k in ('module_section_map','module_section_headers','module_section_capacity','module_section_pools_ready')}
                if state['function']in ('initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                    if state['function']=='LinkContext::InitializeSections':
                        state['comdat_arena']=int(gdb.parse_and_eval('comdat_arena'))
                        state['section_initializer_formals']=dict(this=int(frame.read_var('this')),stack_formals=[],other_hidden_inputs=[])
                    state['module_section_pools']={k:int(gdb.parse_and_eval(k))for k in ('module_section_map','module_section_headers','module_section_capacity','module_section_pools_ready')}
                    state['module_section_bytes']=[gdb.selected_inferior().read_memory(state['module_section_pools'][k],size).tobytes().hex()for k,size in [('module_section_map',4000),('module_section_headers',20000)]]
                if state['function']in ('create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section'):
                    keys=('module','name','flags','sections','header')
                    lookup=frame.older()if state['function']in ('adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section')else frame
                    state['section_creation_formals']={k:int(lookup.read_var(k))for k in keys}
                    state['section_creation_formals']['text']=lookup.read_var('name').string()
                    state['frontier_inputs']['formals']=[state['section_creation_formals'][k]for k in keys]
                    if state['function']=='adjust_output_section_flags':
                        pointer=frame.read_var('flags');local=lookup.read_var('normalized')
                        state['section_flag_formals']=dict(flags=int(pointer),before=int(pointer.dereference()),name=int(frame.read_var('name')),header=int(frame.read_var('header')),owner_local=int(local.address),raw=gdb.selected_inferior().read_memory(int(pointer),4).tobytes().hex())
                        state['frontier_inputs']['formals']=[state['section_flag_formals'][k]for k in ('flags','name','header')]
                    if state['function']in ('initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section'):
                        local=lookup.read_var('enumerator');pointer=local.address if state['function']=='resolve_output_section'else frame.read_var('enumerator');normalized=lookup.read_var('normalized')
                        state['section_enumerator_formals']=dict(enumerator=int(pointer),sections=int(lookup.read_var('sections')),owner_local=int(local.address),normalized_pointer=int(normalized.address),normalized=int(normalized),before=gdb.selected_inferior().read_memory(int(pointer),16).tobytes().hex(),initial_value_contract='uninitialized output record; no incoming bytes consumed'if state['function']=='initialize_section_enumerator'else 'fully initialized enumerator record')
                        state['frontier_inputs']['formals']=[int(pointer),int(frame.read_var('sections'))]if state['function']=='initialize_section_enumerator'else [int(pointer)]
                        if state['function']=='resolve_output_section':
                            state['resolution_formals']=dict(section=int(frame.read_var('section')),caller_current=int(local['current']),caller_current_slot=int(local['current'].address))
                            state['frontier_inputs']['formals']=[int(frame.read_var('section'))]
                        if state['function']in ('next_output_section','end_section_enumerator','resolve_output_section'):
                            state['section_callback_addresses']={k:int(gdb.parse_and_eval('&'+k))for k in ('next_output_section','end_section_enumerator')}
                    initializer=frame.older()
                    while initializer and initializer.name()!='LinkContext::InitializeSections':initializer=initializer.older()
                    assert initializer
                    state['section_initializer_this']=int(initializer.read_var('this'))
                    state['section_handles']=[int(gdb.parse_and_eval(k))for k in ['bss_section', 'small_data_section', 'read_only_section', 'data_section', 'procedure_section', 'import_section', 'import_thunk_section', 'import_descriptor_section', 'delay_import_section', 'export_section', 'export_group', 'exception_section', 'architecture_section', 'ppc_loader_section', 'debug_section', 'debug_symbol_group', 'debug_type_group', 'debug_precompiled_group', 'debug_fixup_group'] ]
                    state['comdat_arena']=int(gdb.parse_and_eval('comdat_arena'))
                if state['function']in ('create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                    if state['function']in ('LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                        owner=frame if state['function']=='LinkContext::PassOne'else frame.older()
                        while owner and owner.name()!='LinkContext::PassOne':owner=owner.older()
                        assert owner and owner.name()=='LinkContext::PassOne'
                        state['pass_one_formals']=dict(this=int(owner.read_var('this')),stack_formals=[],other_hidden_inputs=[])
                        if state['function']=='LinkContext::PassOne':
                            state['pass_one_formals']['raw_ecx']=int(gdb.parse_and_eval('$ecx'))
                            state['frontier_inputs']=dict(formals=[],hidden_register_inputs=[dict(register='ecx',value=int(owner.read_var('this')))])
                        else:
                            if state['function']=='check_output_input_collision':
                                output=frame.read_var('output');inputs=frame.read_var('inputs')
                                state['collision_formals']=dict(output=int(output),inputs=int(inputs),text=output.string())
                                state['frontier_inputs']=dict(formals=[int(output),int(inputs)],hidden_register_inputs=[])
                            else:
                                if state['function']in ('create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                                    n=state['open_file_entry']['formals'];state['open_file_formals']=dict(name=n[0],flags=n[1],mode=n[2],text=gdb.Value(n[0]).cast(gdb.lookup_type('char').pointer()).string())
                                    state['file_record_formals']=state['mapped_open_return']['caller_formals'].copy();raw=state['mapped_open_entry']['raw_formals'];error=state['mapped_open_return']['error']
                                    state['mapped_open_formals']=dict(record=raw[0],flags=raw[1],created=raw[2]&255,raw_created_word=raw[2],raw_upper_bits=raw[2]>>8,size=raw[3],initial_size=error)
                                    physical=state['mapping_entry']['raw_formals'];state['mapping_formals']=dict(record=physical[0],resize=physical[1]&255,raw_resize_word=physical[1],error=physical[2],initial_error=state['mapping_return']['error'])
                                    state['file_cache_lists']={k:int(gdb.parse_and_eval(k))for k in ('cached_files','cached_file_tail','pending_files','cached_file_count')};state['file_cache_slots']={k:int(gdb.parse_and_eval('&'+k))for k in ('file_newest','pending_files')}
                                    state['file_record_name_bytes']=state['file_acquire_return']['name_bytes']
                                    raw=state['library_create_entry']['formals'];state['library_create_formals']=dict(name=raw[0],path=raw[1],libraries=raw[2]);state['library_list_input']=gdb.selected_inferior().read_memory(raw[2],16).tobytes().hex()
                                    state['frontier_inputs']=dict(formals=raw,hidden_register_inputs=[]);state['output_file']=int(gdb.parse_and_eval('output_file'));state['file_input_lifetime']='captured at mapped-opener return before caller locals expire'
                                    if state['function']in ('find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                                        physical=state['library_find_entry']['raw_formals'];state['find_library_formals']=dict(name=physical[0],first=physical[1],before=physical[2]&255,raw_before_word=physical[2]);state['frontier_inputs']=dict(formals=[physical[0],physical[1],physical[2]&255],hidden_register_inputs=[])
                                    if state['function']in ('initialize_library_enumerator','next_library'):
                                        frame=gdb.newest_frame()
                                        while frame and frame.name()!='find_library':frame=frame.older()
                                        assert frame
                                        local=frame.read_var('enumerator');address=int(local.address);size=int(local.type.sizeof)
                                        assert size==16
                                        formals=state['library_enum_entry']['formals']
                                        state['library_enum_input']=dict(address=address,bytes=size,raw_before=state['library_enum_entry']['before'],formals=formals,hidden_inputs=[])
                                        state['frontier_inputs']=dict(formals=formals,hidden_register_inputs=[])
                                        if state['function']=='next_library':
                                            state['library_enum_after']=gdb.selected_inferior().read_memory(address,16).tobytes().hex()
                                            state['library_enum_callbacks']={k:int(gdb.parse_and_eval('&'+k))for k in ('next_library','end_section_enumerator')}
                                            state['frontier_inputs']=dict(formals=state['library_next_entry']['formals'],hidden_register_inputs=[])
                                    if state['function']in ('LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                                        entry=state['library_enum_entry'];address=entry['formals'][0]
                                        state['library_enum_input']=dict(address=address,bytes=16,raw_before=entry['before'],formals=entry['formals'],hidden_inputs=[])
                                        state['library_enum_after']=state['library_enum_return']['after']
                                        state['library_enum_callbacks']={k:int(gdb.parse_and_eval('&'+k))for k in ('next_library','end_section_enumerator')}
                                        physical=state['load_input_entry'];state['frontier_inputs']=dict(formals=physical['formals'],hidden_register_inputs=[dict(register='ecx',value=physical['this'])])
                                        input_library_address=physical['formals'][1];raw=gdb.selected_inferior().read_memory(input_library_address,68).tobytes();array=struct.unpack_from('<I',raw,4)[0]
                                        state['command_line_library']=dict(address=input_library_address,record=raw.hex(),modules=array,module_array=gdb.selected_inferior().read_memory(array,20).tobytes().hex(),global_slot=int(gdb.parse_and_eval('&current_input_library')),current=int(gdb.parse_and_eval('current_input_library')))
                                        if state['function']in ('read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                                            state['frontier_inputs']=dict(formals=state['read_input_entry']['formals'],hidden_register_inputs=[])
                                            state['input_diagnostic_name']=dict(address=int(gdb.parse_and_eval('&current_phase')),modeled_prefix_bytes=32,raw=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&current_phase')),32).tobytes().hex())
                                            state['input_file_index']=int(gdb.parse_and_eval('current_input_file'))
                                            state['incremental_status']=dict(code=int(gdb.parse_and_eval('incremental_status_code')),reuse=int(gdb.parse_and_eval('incremental_reuse')),code_bytes=4,reuse_bytes=1)
                                            if state['function']=='is_archive':state['frontier_inputs']=dict(formals=state['archive_entry']['formals'],hidden_register_inputs=[])
                                            if state['function']in ('mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                                                state['frontier_inputs']=dict(formals=state['mapped_read_entry']['formals'],hidden_register_inputs=[])
                                                state['archive_member_offset']=int(gdb.parse_and_eval('archive_member_offset'))
                                                state['archive_statistics_enabled']=int(gdb.parse_and_eval('file_access_statistics'))
                                                if state['function']=='is_mac_resource_file':state['frontier_inputs']=dict(formals=state['mac_resource_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='get_file_length':state['frontier_inputs']=dict(formals=state['file_length_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='seek_file':state['frontier_inputs']=dict(formals=state['seek_file_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='seek_mapped_file':state['frontier_inputs']=dict(formals=state['seek_mapped_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='read_big_endian_dword':state['frontier_inputs']=dict(formals=state['big_endian_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='byte_swap_dword':state['frontier_inputs']=dict(formals=state['byte_swap_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='read_object_file':state['frontier_inputs']=dict(formals=state['object_reader_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='check_input_machine':state['frontier_inputs']=dict(formals=state['input_machine_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='detect_input_machine':state['frontier_inputs']=dict(formals=state['detect_machine_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='initialize_machine':state['frontier_inputs']=dict(formals=state['initialize_machine_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='validate_delay_load_options':state['frontier_inputs']=dict(formals=state['validate_delay_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='initialize_x86_backend':state['frontier_inputs']=dict(formals=state['x86_backend_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='read_coff_header':state['frontier_inputs']=dict(formals=state['coff_header_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='create_input_module':state['frontier_inputs']=dict(formals=state['input_module_entry']['formals'],hidden_register_inputs=[])
                                                if state['function']=='OutputLibrary::FindModule':state['frontier_inputs']=dict(formals=state['module_find_entry']['formals'],hidden_register_inputs=[dict(register='ecx',value=state['module_find_entry']['this'])])



                                else:
                                    file_frame=frame
                                    if state['function']in ('find_cached_file','get_available_file_record','move_file_record'):
                                        while file_frame and file_frame.name()!='acquire_file_record':file_frame=file_frame.older()
                                        assert file_frame

                                    if state['function']in ('open_mapped_file','initialize_file_mapping'):
                                        file_frame=frame.older()
                                        while file_frame.name()!='open_argument_file':file_frame=file_frame.older()
                                    name=file_frame.read_var('name');flags=file_frame.read_var('flags');mode=file_frame.read_var('mode')
                                    state['open_file_formals']=dict(name=int(name),flags=int(flags),mode=int(mode),text=name.string())
                                    state['frontier_inputs']=dict(formals=[int(name),int(flags),int(mode)],hidden_register_inputs=[])
                                    if state['function']in ('acquire_file_record','find_cached_file','get_available_file_record','move_file_record'):
                                        created=file_frame.read_var('created');caller=file_frame.older();local=caller.read_var('created');size=caller.read_var('size')
                                        state['file_record_formals']=dict(name=int(name),flags=int(flags),mode=int(mode),created=int(created),initial_created=int(created.dereference()),size_slot=int(size.address),initial_size=int(size),initial_mapped_result=int(caller.read_var('mapped')),file_access_statistics=int(gdb.parse_and_eval('file_access_statistics')),created_bytes=local.type.sizeof,local_created=int(local.address))
                                        if state['function']in ('get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping'):
                                            state['frontier_inputs']=dict(formals=[],hidden_register_inputs=[])
                                            state['file_cache_lists']={k:int(gdb.parse_and_eval(k))for k in ('cached_files','cached_file_tail','pending_files','cached_file_count')}
                                            if state['function']=='move_file_record':
                                                keys=('record','source_first','source_last','destination_first','destination_last','append')
                                                values={k:frame.read_var(k)for k in keys}
                                                state['file_move_formals']={k:int(v)for k,v in values.items()}
                                                state['file_move_formals'].update(source_value=int(values['source_first'].dereference()),destination_value=int(values['destination_first'].dereference()),slot_bytes=values['source_first'].type.target().sizeof)
                                                state['file_cache_slots']={k:int(gdb.parse_and_eval('&'+k))for k in ('file_newest','pending_files')}
                                                state['frontier_inputs']=dict(formals=[int(values[k])for k in keys],hidden_register_inputs=[])
                                        elif state['function']=='find_cached_file':state['frontier_inputs']=dict(formals=[int(frame.read_var('name'))],hidden_register_inputs=[])
                                        else:state['frontier_inputs']['formals'].append(int(created))

                            if state['function']in ('open_mapped_file','initialize_file_mapping'):
                                caller=file_frame;local=caller.read_var('created');size=caller.read_var('size');args=state['file_record_entry']['formals'];raw=state['mapped_open_entry']['raw_formals']
                                state['file_record_formals']=dict(name=args[0],flags=args[1],mode=args[2],created=args[3],initial_created=int(local),size_slot=int(size.address),initial_size=int(size),initial_mapped_result=int(caller.read_var('mapped')),file_access_statistics=int(gdb.parse_and_eval('file_access_statistics')),created_bytes=local.type.sizeof,local_created=int(local.address))
                                state['mapped_open_formals']=dict(record=raw[0],flags=raw[1],created=raw[2]&255,raw_created_word=raw[2],raw_upper_bits=raw[2]>>8,size=raw[3],initial_size=int(gdb.parse_and_eval('*(unsigned*)'+str(raw[3]))))
                                state['frontier_inputs']=dict(formals=[raw[0],raw[1],raw[2]&255,raw[3]],hidden_register_inputs=[])
                                state['file_cache_lists']={k:int(gdb.parse_and_eval(k))for k in ('cached_files','cached_file_tail','pending_files','cached_file_count')}
                                state['file_cache_slots']={k:int(gdb.parse_and_eval('&'+k))for k in ('file_newest','pending_files')}
                                state['file_record_name_bytes']=state['file_acquire_return']['name_bytes']
                                if state['function']=='initialize_file_mapping':
                                    physical=state['mapping_entry']['raw_formals'];state['mapping_formals']=dict(record=physical[0],resize=physical[1]&255,raw_resize_word=physical[1],error=physical[2],initial_error=int(gdb.parse_and_eval('*(unsigned*)'+str(physical[2]))))
                                    state['frontier_inputs']=dict(formals=[physical[0],physical[1]&255,physical[2]],hidden_register_inputs=[])

                            state['pass_one_flags']={k:int(gdb.parse_and_eval(k))for k in ('verbose_link','output_file_open','current_image_file','arch_data_ignored')}
                        phase=gdb.parse_and_eval('default_phase');state['default_phase']=dict(address=int(phase),text=phase.string())
                    else:
                        finder=frame.older()if state['function']in ('initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library')else frame
                        name=finder.read_var('name');section=finder.read_var('section')
                        state['section_group_formals']=dict(name=int(name),section=int(section),text=name.string())
                        state['section_callback_addresses']={k:int(gdb.parse_and_eval('&'+k))for k in ('next_output_section','end_section_enumerator')}
                        state['frontier_inputs']=dict(formals=[int(name),int(section)]if state['function']=='create_contribution_group'else[int(section),int(name)],hidden_register_inputs=[])
                        if state['function']in ('initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                            pointer=frame.read_var('enumerator');owner=finder.read_var('enumerator')
                            state['group_enumerator_formals']=dict(enumerator=int(pointer),section=int(section),owner_local=int(owner.address),before=gdb.selected_inferior().read_memory(int(pointer),16).tobytes().hex(),initial_value_contract='uninitialized output record; no incoming bytes consumed'if state['function']=='initialize_group_enumerator'else 'fully initialized group enumerator record')
                            state['group_callback_addresses']={k:int(gdb.parse_and_eval('&'+k))for k in ('next_contribution_group','end_section_enumerator')}
                            state['frontier_inputs']=dict(formals=[int(pointer),int(section)]if state['function']=='initialize_group_enumerator'else[int(pointer)],hidden_register_inputs=[])
                    if state['function']in ('LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                        state['section_initializer_this']=state['sections_initializer_call']['this']
                        state['section_callback_addresses']={k:int(gdb.parse_and_eval('&'+k))for k in ('next_output_section','end_section_enumerator')}
                        state['group_callback_addresses']={k:int(gdb.parse_and_eval('&'+k))for k in ('next_contribution_group','end_section_enumerator')}
                    else:
                        initializer=frame.older()
                        while initializer and initializer.name()!='LinkContext::InitializeSections':initializer=initializer.older()
                        assert initializer
                        state['section_initializer_this']=int(initializer.read_var('this'))
                    state['section_handles']=[int(gdb.parse_and_eval(k))for k in ['bss_section', 'small_data_section', 'read_only_section', 'data_section', 'procedure_section', 'import_section', 'import_thunk_section', 'import_descriptor_section', 'delay_import_section', 'export_section', 'export_group', 'exception_section', 'architecture_section', 'ppc_loader_section', 'debug_section', 'debug_symbol_group', 'debug_type_group', 'debug_precompiled_group', 'debug_fixup_group'] ]
                    state['comdat_arena']=int(gdb.parse_and_eval('comdat_arena'))
                    state['section_list']=[int(receiver.dereference()['sections'][k])for k in ('first','last')]
                    state['section_records']=[];node=receiver.dereference()['sections']['first'];seen=set()
                    while int(node):
                        assert int(node)not in seen and len(seen)<32;seen.add(int(node))
                        value=node.dereference();name=value['name'];text=name.string()
                        state['section_records'].append(dict(address=int(node),name=int(name),next=int(value['next']),text=text,raw=gdb.selected_inferior().read_memory(int(node),72).tobytes().hex(),name_bytes=gdb.selected_inferior().read_memory(int(name),len(text)+1).tobytes().hex()))
                        node=value['next']
                    arena=gdb.parse_and_eval('output_section_arena');raw=gdb.selected_inferior().read_memory(int(arena),16).tobytes();header=struct.unpack('<4I',raw)
                    state['output_section_arena']=dict(address=int(arena),raw=raw.hex(),data=gdb.selected_inferior().read_memory(header[3],header[1]+header[2]).tobytes().hex())
                    state['output_section_count']=int(gdb.parse_and_eval('output_section_count'))
                    if state['function']in ('LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule'):
                        arena=gdb.parse_and_eval('contribution_group_arena');raw=gdb.selected_inferior().read_memory(int(arena),16).tobytes();header=struct.unpack('<4I',raw)
                        state['contribution_group_arena']=dict(address=int(arena),raw=raw.hex(),data=gdb.selected_inferior().read_memory(header[3],header[1]+header[2]).tobytes().hex())
                        state['group_records']=[]
                        for i in range(header[1]//44):
                            address=header[3]+i*44;value=gdb.Value(address).cast(gdb.lookup_type('ContributionGroup').pointer()).dereference();text=value['name'].string()
                            state['group_records'].append(dict(address=address,name=int(value['name']),text=text,name_bytes=gdb.selected_inferior().read_memory(int(value['name']),len(text)+1).tobytes().hex(),raw=gdb.selected_inferior().read_memory(address,44).tobytes().hex()))

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
    state['file_statistics']=[int(gdb.parse_and_eval('mapped_files[%%d]'%%i)) for i in range(1,16)]
    state['mapped_file_slots']=[int(gdb.parse_and_eval('mapped_files[%%d]'%%i))for i in range(16)]
    state['mapped_file_slots_address']=int(gdb.parse_and_eval('&mapped_files[0]'))
    state['mapped_file_slots_bytes']=gdb.selected_inferior().read_memory(state['mapped_file_slots_address'],64).tobytes().hex()
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
    script=script.replace('__NATIVE_CAPTURE_DIRECTORY__',repr(str(capture)))
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
