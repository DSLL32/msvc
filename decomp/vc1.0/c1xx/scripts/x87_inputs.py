"""Only the two actual x87 operands of check_extended_range.

Original FSAVE logical slots are bound to independently read FLD operands;
TOP and full physical tags establish that the two slots are populated.
"""
import re

def control_observation(events,expected=0):
    matches=re.findall(r'^x87-context requested (\d+) actual (\d+) success (\d+) control ([0-9a-f]{4}) status ([0-9a-f]{4}) tag ([0-9a-f]{4}) top (\d+)$',events,re.M)
    assert len(matches)==1,'missing x87 context receipt'
    requested,actual,success,control,status,tag,top=matches[0]
    requested,actual,success,top=map(int,(requested,actual,success,top));control,status,tag=map(lambda value:int(value,16),(control,status,tag))
    assert requested==actual==expected and success==1 and top==((status>>11)&7),'unreadable x87 context'
    return dict(requested=requested,actual=actual,context_read=True,control=control,status=status,tag=tag,top=top)

def original_observation(events):
    context=control_observation(events,2);top,tag=context['top'],context['tag']
    rows=re.findall(r'^x87-input (st[01]) physical (\d+) tag (\d+) requested (\d+) actual (\d+) success (\d+) ([0-9a-f]{20})$',events,re.M)
    assert len(rows)==2 and [row[0] for row in rows]==['st0','st1'],'missing x87 operand'
    inputs=[]
    for i,(register,physical,kind,requested_bytes,read_bytes,read_succeeded,raw) in enumerate(rows):
        physical,kind,requested_bytes,read_bytes,read_succeeded=map(int,(physical,kind,requested_bytes,read_bytes,read_succeeded))
        assert physical==((top+i)&7) and kind==((tag>>(2*physical))&3) and kind!=3,'empty/mismapped x87 input'
        assert requested_bytes==read_bytes==10 and read_succeeded==1,'short/unreadable x87 operand'
        inputs.append(dict(register=register,physical_index=physical,tag=kind,requested_bytes=10,read_bytes=10,read_succeeded=True,raw=raw))
    return dict(context,inputs=inputs)
