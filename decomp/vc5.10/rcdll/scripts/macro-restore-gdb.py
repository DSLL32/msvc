"""Opposite-state R-marker probe of the production resume function."""
import gdb
import json
import os
from pathlib import Path

gdb.execute('set pagination off');gdb.execute('set confirm off');gdb.execute('break main');gdb.execute('run')
assert gdb.newest_frame().name()=='main'
macro=int(gdb.parse_and_eval('(void *)malloc(sizeof(RcMacro))'));assert macro
gdb.selected_inferior().write_memory(macro,bytes(16));gdb.execute('set $macro=(RcMacro *)%d'%macro);gdb.execute('set $macro->flags=5')
settings={'rc_expanding_macro':'1','rc_number_buffer_active':'0','rc_macro_output_cursor':'&rc_string_buffer[3]','rc_number_buffer':'&rc_string_buffer[1034]','rc_source_cursor':'&rc_string_buffer[201]','rc_macro_frames[1].source':'&rc_string_buffer[10]','rc_macro_frames[1].output':'&rc_string_buffer[5]','rc_macro_frames[1].number':'&rc_string_buffer[1028]','rc_macro_frames[1].macro':'$macro','rc_string_buffer[200]':'0','rc_string_buffer[201]':'82'}
mode=os.environ.get('RCDLL_RESTORE_MODE','restore')
if mode=='finish-active2':settings['rc_number_buffer_active']='2'
if mode=='finish-unaligned':
    settings['rc_number_buffer_active']='1';settings['rc_number_buffer']='(WCHAR *)((char *)&rc_string_buffer+2057)'
for name,value in settings.items():gdb.execute('set variable '+name+'='+value)
def observed():
    return {name:dict(value=int(gdb.parse_and_eval(name))&0xffffffff,address=int(gdb.parse_and_eval('&'+name)),bytes=gdb.parse_and_eval(name).type.sizeof)for name in settings}
before=observed()
for name,value in settings.items():assert before[name]['value']==int(gdb.parse_and_eval(value))&0xffffffff
frames=int(gdb.parse_and_eval('&rc_macro_frames'));table=bytes(gdb.selected_inferior().read_memory(frames,1280));base=int(gdb.parse_and_eval('&rc_string_buffer'));work=bytes(gdb.selected_inferior().read_memory(base,4090));record=bytes(gdb.selected_inferior().read_memory(macro,16))
if mode!='restore':
    try:gdb.parse_and_eval('rc_finish_macro_frame()')
    except gdb.error:pass
    frame=gdb.newest_frame();pc=int(gdb.parse_and_eval('$pc'));assert frame.name()=='rc_finish_macro_frame'and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b'\xcc'
    assert observed()==before and bytes(gdb.selected_inferior().read_memory(frames,1280))==table and bytes(gdb.selected_inferior().read_memory(base,4090))==work
    Path(os.environ['RCDLL_RESTORE_PROBE']).write_text(json.dumps(dict(mode=mode,status='production-INT3',function=frame.name(),PC=hex(pc),executed_byte='cc',before=before,scope='Native finish guard rejects unproved active0/active2 or unaligned work cursor before app state writes; no exception/overflow behavior claim.'),indent=2)+'\n')
    gdb.execute('quit')
assert int(gdb.parse_and_eval('rc_resume_input()'))==4
after=observed();assert after['rc_expanding_macro']['value']==after['rc_number_buffer_active']['value']==0
assert after['rc_macro_output_cursor']['value']==base and after['rc_number_buffer']['value']==base+2048
assert after['rc_source_cursor']['value']==before['rc_macro_frames[1].source']['value']
assert bytes(gdb.selected_inferior().read_memory(frames,1280))==table and bytes(gdb.selected_inferior().read_memory(base,4090))==work
assert bytes(gdb.selected_inferior().read_memory(macro,16))==record[:13]+b'\4'+record[14:]
Path(os.environ['RCDLL_RESTORE_PROBE']).write_text(json.dumps(dict(before=before,after=after,result=4,macro=hex(macro),flag_before=5,flag_after=4,frame_bytes_unchanged=1280,work_bytes_unchanged=4090,scope='Native-only actual production R path with distinct current/saved/reset pointer values, proving source restoration, constant work-area resets and BYTE flag decrement. No original oracle or extra function/code credit.'),indent=2)+'\n')
