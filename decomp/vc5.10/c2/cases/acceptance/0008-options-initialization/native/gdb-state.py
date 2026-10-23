import gdb,json
from pathlib import Path
frame=gdb.newest_frame()
info={'function':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'signal':gdb.execute('info program',to_string=True),'backtrace':gdb.execute('bt',to_string=True),'trace':trace,'globals':{}}
pid=gdb.selected_inferior().pid
actual_args=Path('/proc/%d/cmdline'%pid).read_bytes().rstrip(b'\0').split(b'\0')
actual_env=Path('/proc/%d/environ'%pid).read_bytes().split(b'\0')
Path('/root/.ai-tmp/msvc2/vc5.10/c2/firstcase/options-native/native-maps.txt').write_text(Path('/proc/%d/maps'%pid).read_text())
gdb.execute('info registers eip')
gdb.execute('info symbol $pc')
gdb.execute('x/1ib $pc-1')
gdb.execute('info program')
info['arguments']=[v.decode()for v in actual_args[1:]]
flags=[v[14:].decode()for v in actual_env if v.startswith(b'MSC_CMD_FLAGS=')]
info['MSC_CMD_FLAGS']=flags[0]if flags else None
records=[r for r in [{'name': '_c2_math_error', 'native': 'c2_math_error', 'start': 4484380, 'end': 4484383, 'abi': 'cdecl, one pointer parameter', 'kind': 'crt-source'}, {'name': '_c2_print_banner', 'native': 'c2_print_banner', 'start': 4738355, 'end': 4738356, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '@c2_open_intermediate@16', 'native': 'c2_open_intermediate', 'start': 4530997, 'end': 4530998, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX base EDX suffix; stack mode and description, RET8'}, {'name': '_c2_read_header', 'native': 'c2_read_header', 'start': 4533924, 'end': 4533925, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_reset_symbol_table', 'native': 'c2_reset_symbol_table', 'start': 4534290, 'end': 4534291, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_create_type_table', 'native': 'c2_create_type_table', 'start': 4534317, 'end': 4534318, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_constants', 'native': 'c2_initialize_constants', 'start': 4534326, 'end': 4534327, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_opcodes', 'native': 'c2_initialize_opcodes', 'start': 4534439, 'end': 4534440, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_machine', 'native': 'c2_initialize_machine', 'start': 4534522, 'end': 4534523, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_trees', 'native': 'c2_initialize_trees', 'start': 4534572, 'end': 4534573, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_codegen', 'native': 'c2_initialize_codegen', 'start': 4534958, 'end': 4534959, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_registers', 'native': 'c2_initialize_registers', 'start': 4535034, 'end': 4535035, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_initialize_optimizer', 'native': 'c2_initialize_optimizer', 'start': 4535045, 'end': 4535046, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_compile_unit', 'native': 'c2_compile_unit', 'start': 4538855, 'end': 4538856, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_finalize_output', 'native': 'c2_finalize_output', 'start': 4541174, 'end': 4541175, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_flush_debug', 'native': 'c2_flush_debug', 'start': 4766214, 'end': 4766215, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_cleanup', 'native': 'c2_cleanup', 'start': 4541188, 'end': 4541189, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters'}, {'name': '_c2_console_handler@4', 'native': 'c2_console_handler', 'start': 4737491, 'end': 4737492, 'pending_extent': True, 'kind': 'application', 'abi': 'stdcall, DWORD control event; RET4'}, {'name': '@c2_report_internal@8', 'native': 'c2_report_internal', 'start': 4746044, 'end': 4746045, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX source path EDX line'}, {'name': '_c2_fatal', 'native': 'c2_fatal', 'start': 4764519, 'end': 4764520, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, int message and varargs'}, {'name': '_c2_copy_arguments', 'native': 'c2_copy_arguments', 'start': 4532888, 'end': 4532939, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters; returns pointer to terminal argument slot'}, {'name': '_c2_next_argument', 'native': 'c2_next_argument', 'start': 4533175, 'end': 4533209, 'pending_extent': True, 'kind': 'application', 'abi': 'cdecl, no parameters; returns argument string', 'state_policy': 'command-line-state', 'state': [{'original': '0x499708', 'native': '(unsigned long)c2_onexit_end'}, {'original': '0x4996d4', 'native': '(unsigned long)c2_onexit_begin'}, {'original': '0x49975c', 'native': 'c2_fmode'}, {'original': '0x4980d4', 'native': 'c2_commode'}, {'original': '0x499718', 'native': 'c2_adjust_fdiv'}, {'original': '0x499694', 'native': 'c2_default_math_error'}, {'original': '0x4980d0', 'native': 'c2_new_mode'}, {'original': '0x4980cc', 'native': 'c2_wildcards'}, {'original': '0x7803fa88', 'native': 'recon_fmode'}, {'original': '0x7803fa94', 'native': 'recon_commode'}, {'original': '0x7803f928', 'native': 'recon_adjust_fdiv'}, {'original': '0x7803fa90', 'native': 'recon_crt_app_type'}, {'original': '0x7803f198', 'native': 'recon_crt_newmode'}]}, {'name': '@c2_match_option@16', 'native': 'c2_match_option', 'start': 4533209, 'end': 4533210, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX table EDX input; stack next-callback and mode; RET8'}, {'name': '@c2_validate_alignment@4', 'native': 'c2_validate_alignment', 'start': 4533915, 'end': 4533924, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX alignment'}, {'name': '@c2_get_diagnostic_string@4', 'native': 'c2_get_diagnostic_string', 'start': 4765390, 'end': 4765416, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX message number'}, {'name': '@c2_system_fatal@12', 'native': 'c2_system_fatal', 'start': 4765320, 'end': 4765358, 'pending_extent': True, 'kind': 'application', 'abi': 'fastcall, ECX message EDX prefix; stack filename; RET4'}] if r['native']==frame.name()]
if len(records)==1:
    for view in records[0].get('state',[]):
        info['globals'][view['original']]=int(gdb.parse_and_eval(view['native'])) & 0xffffffff
    if records[0].get('state_policy')in ('command-line-state','environment-flags'):
        ptr=gdb.parse_and_eval('c2_argv');values=[];i=0
        while int(ptr[i]):
            values.append(ptr[i].string());i+=1
            assert i<4096
        info['command_state']={'argc':int(gdb.parse_and_eval('c2_argc')),'argv':values}
        if records[0]['state_policy']=='environment-flags':
            info['flags']=None if int(gdb.parse_and_eval('flags'))==0 else gdb.parse_and_eval('flags').string()
    if records[0].get('state_policy')=='exit-callback':
        info['callback']=int(gdb.parse_and_eval('callback'))
        gdb.write('C2-CALLBACK 0x%x\n'%info['callback'])
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
open('/root/.ai-tmp/msvc2/vc5.10/c2/firstcase/options-native/state.json','w').write(json.dumps(info,indent=2)+'\n')
