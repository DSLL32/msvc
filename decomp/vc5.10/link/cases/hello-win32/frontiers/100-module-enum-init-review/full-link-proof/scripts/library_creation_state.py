"""Whole command-line library and the true first input-loader boundary."""
import functools
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from mapping_completion_state import library_callbacks


@functools.lru_cache(maxsize=8)
def current_slot(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[line.split()for line in subprocess.check_output(['nm','-S','-n','--defined-only',path],text=True).splitlines()]
    row,=[r for r in rows if len(r)==4 and r[3]=='current_input_library']
    assert int(row[1],16)==4 and row[2]=='B'
    return int(row[0],16)


def record(modules,flags):
    data=bytearray(68)
    struct.pack_into('<I',data,4,modules)
    struct.pack_into('<I',data,8,5)
    data[16]=1
    struct.pack_into('<I',data,48,flags)
    return bytes(data)


def current_record(original,state,trap):
    left=trap['command_line_library'];right=state['command_line_library']
    assert left['address']and right['address']and left['modules']and right['modules']
    expected=record(left['modules'],0x14);nexpected=record(right['modules'],0x14)
    assert(original/'command-line-library.bin').read_bytes()==expected
    assert bytes.fromhex(right['record'])==nexpected
    return expected,nexpected


def compare(original,state,trap,events,native_directory):
    current_record(original,state,trap)
    context=trap['validation_context'];ncontext=state['context_this'];left=trap['command_line_library'];right=state['command_line_library']
    pointer=left['address'];npointer=right['address'];array=left['modules'];narray=right['modules']
    create,=[e for e in events if e['event']=='library_create_entry'];find,=[e for e in events if e['event']=='library_find_entry']
    init,=[e for e in events if e['event']=='library_enum_entry'];initret,=[e for e in events if e['event']=='library_enum_return']
    step,=[e for e in events if e['event']=='library_next_entry'];stepret,=[e for e in events if e['event']=='library_next_return']
    findret,=[e for e in events if e['event']=='library_find_return'];created,=[e for e in events if e['event']=='library_create_return']
    assert create['formals']==[0,0,context+0x1bc]and create['hidden_inputs']==[]
    nc=state['library_create_entry'];assert nc['formals']==[0,0,ncontext+0x1bc]and nc['hidden_inputs']==[]
    assert trap['library_create_formals']==dict(name=0,path=0,libraries=context+0x1bc)
    assert state['library_create_formals']==dict(name=0,path=0,libraries=ncontext+0x1bc)
    before=struct.pack('<4I',0,context+0x1bc,0,0);nbefore=struct.pack('<4I',0,ncontext+0x1bc,0,0)
    after=struct.pack('<4I',pointer,pointer+60,0,0);nafter=struct.pack('<4I',npointer,npointer+60,0,0)
    assert(original/'library-list-before.bin').read_bytes()==before
    assert bytes.fromhex(state['library_list_before'])==nbefore
    assert(original/'library-list-input.bin').read_bytes()==(original/'library-list-after.bin').read_bytes()==after
    assert bytes.fromhex(state['library_list_input'])==nafter
    assert find==dict(event='library_find_entry',stack_pointer=create['stack_pointer']-28,formals=[0,0,0],hidden_inputs=[])
    assert trap['find_library_formals']==state['find_library_formals']==dict(name=0,first=0,before=0,raw_before_word=0)
    nf=state['library_find_entry'];assert nf['raw_formals']==[0,0,0]and nf['hidden_inputs']==[]
    output=init['formals'][0];noutput=state['library_enum_input']['address']
    assert init==dict(event='library_enum_entry',stack_pointer=find['stack_pointer']-820,formals=[output,0],hidden_inputs=[])
    assert output==init['stack_pointer']+32
    assert initret==dict(event='library_enum_return',site=0x40f674,stack=init['stack_pointer']+12,entry_stack=init['stack_pointer'])
    assert step==dict(event='library_next_entry',stack_pointer=init['stack_pointer']+4,formals=[output],hidden_inputs=[])
    assert stepret==dict(event='library_next_return',site=0x40f67e,stack=step['stack_pointer']+8,entry_stack=step['stack_pointer'],raw_result=stepret['raw_result'],result=0)
    assert stepret['raw_result']&255==0 # Only AL is the BYTE result; raw upper bits retained.
    expected=struct.pack('<4I',0x40fb20,0x40fd90,0,0)
    assert(original/'library-enum-return.bin').read_bytes()==(original/'library-next-after.bin').read_bytes()==expected
    assert trap['library_enum_input']==dict(address=output,bytes=16,formals=[output,0],hidden_inputs=[])
    entry=state['library_enum_entry'];ni=state['library_enum_input']
    assert entry['formals']==[noutput,0]and entry['hidden_inputs']==[]
    assert ni==dict(address=noutput,bytes=16,raw_before=entry['before'],formals=[noutput,0],hidden_inputs=[])
    assert len(bytes.fromhex(entry['before']))==len((original/'library-enum-before.bin').read_bytes())==16
    assert entry['stack_pointer']<noutput<nf['stack_pointer']and noutput+16<=nf['stack_pointer']
    directory=Path(native_directory);receipt=json.loads((directory/'receipt.json').read_text());elf=str(directory/'compiled/native-LINK.EXE')
    symbols=library_callbacks(elf,receipt['native_sha256']);assert state['library_enum_callbacks']==symbols
    nexp=struct.pack('<4I',symbols['next_library'],symbols['end_section_enumerator'],0,0).hex()
    assert state['library_enum_return']==dict(entry,normal_return=True,return_stack=entry['stack_pointer']+12,after=nexp)
    assert state['library_enum_after']==nexp
    ns=state['library_next_entry'];assert ns['formals']==[noutput]and ns['hidden_inputs']==[]
    nr=state['library_next_return'];assert nr==dict(ns,normal_return=True,return_stack=ns['stack_pointer']+8,raw_result=nr['raw_result'],result=0,after=nexp)
    assert nr['raw_result']&255==0
    assert findret==dict(event='library_find_return',site=0x40ede7,stack=find['stack_pointer']+16,entry_stack=find['stack_pointer'],result=0)
    assert state['library_find_return']==dict(result=0,return_stack=nf['stack_pointer']+16,normal_return=True)
    assert created==dict(event='library_create_return',site=0x44b906,stack=create['stack_pointer']+16,entry_stack=create['stack_pointer'],result=pointer)
    assert(original/'library-created-record.bin').read_bytes()==record(array,0)
    ncreated=state['library_create_return'];assert ncreated==dict(nc,normal_return=True,return_stack=nc['stack_pointer']+16,result=npointer,record=record(narray,0).hex(),modules=narray,module_array=ncreated['module_array'],list_after=nafter.hex())
    allocation,=[e for e in events if e['event']=='library_array_call'];allocated,=[e for e in events if e['event']=='library_array_return']
    assert allocation['bytes']==20 and allocated==dict(event='library_array_return',stack=allocation['stack']+4,result=array)
    assert(original/'library-array-created.bin').read_bytes()==(original/'library-created-array.bin').read_bytes()==(original/'command-line-library-array.bin').read_bytes()
    assert len((original/'command-line-library-array.bin').read_bytes())==20
    na=state['library_array_allocation'];assert na['bytes']==20 and na['result']==narray and na['normal_return']is True and na['return_stack']==na['entry_stack']+8
    assert len(bytes.fromhex(na['raw']))==20
    assert na['raw']==ncreated['module_array']==right['module_array'] # Unused malloc bytes, no cross-platform equality.
    assert left['global_slot']==0x47f0a8 and left['current']==pointer
    assert right['global_slot']==current_slot(elf,receipt['native_sha256'])and right['current']==npointer
    lists={e['name']:e for e in events if e['event']=='argument_list'}
    argument=lists['link_files']['nodes'][0]['address'];nargument=state['argument_lists']['link_files']['header'][0]
    assert trap['load_input_formals']==dict(this=context,formals=[argument,pointer],other_hidden_inputs=[])
    load=state['load_input_entry'];assert load['stack_pointer']==nc['stack_pointer']<state['pass_one_entry']['stack_pointer']
    assert load['this']==ncontext and load['formals']==[nargument,npointer]and load['other_hidden_inputs']==[]
    if state['function']in ('read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule','initialize_module_enumerator','next_module','initialize_module_enumerator','next_module'):
        observed,=[e for e in events if e['event']=='load_input_entry']
        call,=[e for e in events if e['event']=='load_input_call']
        assert observed==dict(event='load_input_entry',stack_pointer=create['stack_pointer']+4,this=context,formals=[argument,pointer],other_hidden_inputs=[])
        assert call==dict(event='load_input_call',site=0x44b94f,stack=observed['stack_pointer']+4,this=context,formals=[argument,pointer],other_hidden_inputs=[])
        assert trap['read_input_formals']==dict(formals=[argument,pointer,context],hidden_inputs=[])
        assert state['read_input_entry']==dict(stack_pointer=load['stack_pointer']-48,formals=[nargument,npointer,ncontext],hidden_inputs=[])
        if state['function']in ('is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine','detect_input_machine','initialize_machine','validate_delay_load_options','initialize_x86_backend','read_coff_header','create_input_module','OutputLibrary::FindModule','initialize_module_enumerator','next_module','initialize_module_enumerator','next_module'):
            reader,=[e for e in events if e['event']=='read_input_entry']
            assert reader==dict(event='read_input_entry',stack_pointer=observed['stack_pointer']-32,formals=[argument,pointer,context],hidden_inputs=[])
            if state['function']=='is_archive':
                assert trap['esp']==reader['stack_pointer']-24 and trap['stack'][:3]==[0x4498e5,lists['link_files']['nodes'][0]['words'][0],14]
            assert state['archive_entry']==dict(stack_pointer=state['read_input_entry']['stack_pointer']-48,formals=[state['canonicalized_arguments'][0]['result'],14],hidden_inputs=[])
            if state['function']=='is_archive':
                assert trap['frontier_inputs']==dict(formals=trap['stack'][1:3],hidden_register_inputs=[])
                assert state['frontier_inputs']==dict(formals=state['archive_entry']['formals'],hidden_register_inputs=[])
                assert trap['argument_pointer']==trap['stack'][1]and trap['argument_text']=='hello.obj'
        else:
            assert trap['esp']==observed['stack_pointer']-32 and trap['stack'][:4]==[0x449a9e,argument,pointer,context]
            assert trap['frontier_inputs']==dict(formals=[argument,pointer,context],hidden_register_inputs=[])
            assert state['frontier_inputs']==dict(formals=[nargument,npointer,ncontext],hidden_register_inputs=[])
    else:
        assert trap['stack'][:3]==[0x44b954,argument,pointer]and trap['ecx']==context
        assert trap['esp']==create['stack_pointer']+4
        assert trap['frontier_inputs']==dict(formals=[argument,pointer],hidden_register_inputs=[dict(register='ecx',value=context)])
        assert state['frontier_inputs']==dict(formals=[nargument,npointer],hidden_register_inputs=[dict(register='ecx',value=ncontext)])
    assert trap['output_file']==state['output_file']==15
    return dict(original_base=array,native_base=narray,bytes=20)
