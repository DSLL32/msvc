"""Auxiliary production numeric-workspace guard probe; no original replay."""
import gdb
import json
import os
from pathlib import Path

gdb.execute('set pagination off');gdb.execute('set confirm off');gdb.execute('break main');gdb.execute('run')
assert gdb.newest_frame().name()=='main'
text=int(gdb.parse_and_eval('(void *)malloc(18)'));macro=int(gdb.parse_and_eval('(void *)malloc(sizeof(RcMacro))'))
assert text and macro
gdb.selected_inferior().write_memory(text,'0x0400\0D\0'.encode('utf-16le'));gdb.selected_inferior().write_memory(macro,bytes(16))
gdb.execute('set $macro=(RcMacro *)%d'%macro);gdb.execute('set $macro->text=(WCHAR *)%d'%text)
gdb.execute('set variable rc_expanding_macro=1');gdb.execute('set variable rc_number_buffer_active=1');gdb.execute('set variable rc_macro_frames[1].macro=$macro');gdb.execute('set variable rc_source_cursor=(WCHAR *)%d'%(text+2));gdb.execute('set variable rc_cpp_active=1')
mode=os.environ['RCDLL_NUMBER_PROBE_MODE'];offset={'supported':2056,'exact-end':4076,'underflow':2046,'unaligned':2057,'overrun':4078,'outside':4090}.get(mode,2056)
base=int(gdb.parse_and_eval('&rc_string_buffer'));gdb.execute('set variable rc_number_buffer=(WCHAR *)%d'%(base+offset));seed=bytes([0xa7])*4090;gdb.selected_inferior().write_memory(base,seed)
if mode=='wrong-active':gdb.execute('set variable rc_number_buffer_active=2')
if mode=='wrong-source':gdb.execute('set variable rc_source_cursor=(WCHAR *)%d'%(text+4))
if mode=='wrong-first':first=ord('1')
else:first=ord('0')
try:result=gdb.parse_and_eval('rc_read_number(%d)'%first)
except gdb.error:result=None
supported=mode in ['supported','exact-end']
if supported:
    assert result is not None and int(result)==1
    actual=bytes(gdb.selected_inferior().read_memory(base,4090));assert actual==seed[:offset]+'0x0400\0'.encode('utf-16le')+seed[offset+14:]
    assert int(gdb.parse_and_eval('rc_number_buffer'))==base+offset+12
    assert int(gdb.parse_and_eval('rc_source_cursor'))==text+12
    proof=dict(status='success',result=1,write_begin=offset,write_end=offset+14,work_bytes=4090)
else:
    frame=gdb.newest_frame();pc=int(gdb.parse_and_eval('$pc'));assert frame.name()=='rc_read_number'and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b'\xcc'
    proof=dict(status='production-INT3',function=frame.name(),PC=hex(pc),executed_byte='cc',offset=offset)
Path(os.environ['RCDLL_NUMBER_PROBE_OUTPUT']).write_text(json.dumps(dict(mode=mode,**proof),indent=2)+'\n')
