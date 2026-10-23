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
    return result
