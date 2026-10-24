"""Native-only production-function store probe, outside case/reference replay."""
import gdb
import json
import os
from pathlib import Path

gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('break main')
gdb.execute('run')
assert gdb.newest_frame().name()=='main'
macro=int(gdb.parse_and_eval('(void *)malloc(sizeof(RcMacro))'))
assert macro and gdb.parse_and_eval('sizeof(RcMacro)')==16
gdb.selected_inferior().write_memory(macro,bytes(16))
gdb.execute('set $macro=(RcMacro *)%d'%macro)
gdb.execute('set $macro->text=&rc_string_buffer[0]')
settings={
    'rc_number_buffer':'&rc_string_buffer[1030]',
    'rc_macro_output_cursor':'&rc_string_buffer[3]',
    'rc_saved_macro_number':'&rc_string_buffer[1025]',
    'rc_saved_macro_output':'&rc_string_buffer[4]',
    'rc_preparing_line':'0x11223344',
    'rc_expansion_line':'0x22334455',
    'rc_expanded_argument_count':'0x33445566',
    'rc_number_buffer_active':'0x44556677',
    'rc_expression_parentheses':'0x55667788',
    'rc_expanding_macro':'0',
}
for name,value in settings.items():gdb.execute('set variable '+name+'='+value)
def observed():
    return {name:dict(value=int(gdb.parse_and_eval(name))&0xffffffff,address=int(gdb.parse_and_eval('&'+name)),bytes=gdb.parse_and_eval(name).type.sizeof)for name in settings}
before=observed()
assert len({row['address']for row in before.values()})==len(settings)
for name,expression in settings.items():assert before[name]['value']==int(gdb.parse_and_eval(expression))&0xffffffff
frames=int(gdb.parse_and_eval('&rc_macro_frames'))
assert gdb.parse_and_eval('sizeof(rc_macro_frames)')==1280
assert bytes(gdb.selected_inferior().read_memory(frames,1280))==bytes(1280)
try:gdb.execute('call rc_expand_macro($macro)',to_string=True)
except gdb.error:pass
frame=gdb.newest_frame();assert frame.name()=='rc_push_macro'and frame.older().name()=='rc_expand_macro'
pc=int(gdb.parse_and_eval('$pc'));assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b'\xcc'
assert int(frame.read_var('macro'))==macro
parent=frame.older();assert int(parent.read_var('result'))==1 and int(parent.read_var('parameters'))==0
after=observed()
assert after['rc_number_buffer_active']['value']==after['rc_expanded_argument_count']['value']==0
assert after['rc_saved_macro_number']['value']==before['rc_number_buffer']['value']
assert after['rc_saved_macro_output']['value']==before['rc_macro_output_cursor']['value']
assert after['rc_expansion_line']['value']==before['rc_preparing_line']['value']
for name in ['rc_number_buffer','rc_macro_output_cursor','rc_preparing_line','rc_expression_parentheses','rc_expanding_macro']:assert after[name]==before[name]
assert bytes(gdb.selected_inferior().read_memory(frames,1280))==bytes(1280)
Path(os.environ['RCDLL_MACRO_INIT_PROBE']).write_text(json.dumps(dict(before=before,after=after,macro=hex(macro),PC=hex(pc),actual_child='rc_push_macro',executed_byte='cc',frame_bytes=1280,frame_writes=0,scope='Synthetic native-only opposite-state store/alias probe through actual production rc_expand_macro, stopping at actual named child INT3. Ordinary GDB invocation from before host entry; no original output/oracle, dependency result or extra reconstruction credit.'),indent=2)+'\n')
