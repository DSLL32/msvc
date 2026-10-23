"""Instruction-boundary observers for arguments/results, never function vertices."""

def plan(accepted):
    result=[]
    if '_mainCRTStartup' in accepted:
        result += [(0x65320,'observe:initerm_c'),(0x6535b,'observe:initerm_cpp')]
    for suffix,address in [('12',0x424e70),('8',0x431d80),('4',0x431df0)]:
        if '_allocate_buffer'+suffix in accepted:
            result += [(address+5-0x400000,'observe:new_call_'+suffix),(address+10-0x400000,'observe:new_return_'+suffix)]
    for suffix,address in [('12',0x424ea0),('8',0x431db0),('4',0x431e20),('timing',0x444ea0)]:
        name='_register_'+('timing_cleanup' if suffix=='timing' else 'buffer'+suffix+'_cleanup')
        if name in accepted:
            result += [(address+5-0x400000,'observe:atexit_call_'+suffix),(address+10-0x400000,'observe:atexit_return_'+suffix)]
    if '_main' in accepted:
        result += [(0xded2,'observe:console_call'),(0xded8,'observe:console_return')]
    if '_initialize_file_cache@16' in accepted:
        result += [(0xb568,'observe:module_call'),(0xb56e,'observe:module_return'),
                   (0xb578,'observe:proc_call'),(0xb57e,'observe:proc_return')]
    if '_initialize_memory' in accepted:
        result += [(0x4f571,'observe:system_before'),(0x4f577,'observe:system_after'),
                   (0x4f586,'observe:memory_before'),(0x4f58c,'observe:memory_after')]
    if '_parse_command_line@16' in accepted:result.append((0xde93,'observe:main_frame'))
    return result
