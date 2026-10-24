"""Native-only opposite-state probe through the production context push."""
import gdb
import json
import os
import struct
from pathlib import Path

gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('break main')
gdb.execute('run')
assert gdb.newest_frame().name()=='main'
macro=int(gdb.parse_and_eval('(void *)malloc(sizeof(RcMacro))'))
assert macro and int(gdb.parse_and_eval('sizeof(RcMacro)'))==16
gdb.selected_inferior().write_memory(macro,bytes(16))
gdb.execute('set $macro=(RcMacro *)%d'%macro)
settings={
    'rc_expanding_macro':'1',
    'rc_number_buffer_active':'3',
    'rc_source_cursor':'&rc_string_buffer[4]',
    'rc_number_buffer':'&rc_string_buffer[1030]',
    'rc_saved_macro_number':'&rc_string_buffer[1025]',
    'rc_saved_macro_output':'&rc_string_buffer[6]',
    'rc_expanded_argument_count':'0x11223344',
}
for name,value in settings.items():gdb.execute('set variable '+name+'='+value)
def observed():
    return {name:dict(value=int(gdb.parse_and_eval(name))&0xffffffff,address=int(gdb.parse_and_eval('&'+name)),bytes=gdb.parse_and_eval(name).type.sizeof)for name in settings}
before=observed();assert len({row['address']for row in before.values()})==len(settings)
for name,expression in settings.items():assert before[name]['value']==int(gdb.parse_and_eval(expression))&0xffffffff
frames=int(gdb.parse_and_eval('&rc_macro_frames'));assert int(gdb.parse_and_eval('sizeof(rc_macro_frames)'))==1280
seed=bytes([0xa7])*1280;gdb.selected_inferior().write_memory(frames,seed)
gdb.execute('call rc_push_macro($macro)',to_string=True)
after=observed();assert after['rc_expanding_macro']['value']==2 and after['rc_number_buffer_active']['value']==4
assert after['rc_number_buffer']['value']==before['rc_saved_macro_number']['value']
for name in ['rc_source_cursor','rc_saved_macro_number','rc_saved_macro_output','rc_expanded_argument_count']:assert after[name]==before[name]
expected=struct.pack('<4IBB',before['rc_source_cursor']['value'],before['rc_saved_macro_output']['value'],before['rc_saved_macro_number']['value'],macro,0x44,0)+bytes([0xa7])*2
actual=bytes(gdb.selected_inferior().read_memory(frames,1280));assert actual==seed[:40]+expected+seed[60:]
Path(os.environ['RCDLL_MACRO_PUSH_PROBE']).write_text(json.dumps(dict(before=before,after=after,macro=hex(macro),frame_index=2,frame=expected.hex(),full_before_sha256=__import__('hashlib').sha256(seed).hexdigest(),full_after_sha256=__import__('hashlib').sha256(actual).hexdigest(),untouched_bytes=1262,reserved_tail='a7a7',scope='Native-only opposite-state actual production push: distinct cursor aliases, depth1→2, active3→4, lowBYTE truncation44 from11223344, expanded cleared and reserved/nonselected storage untouched. No original oracle, output or new function credit.'),indent=2)+'\n')
