"""Only reached x87 inputs of check_extended_range and store_extended.

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

def original_observation(events,count=2):
    assert count in (1,2), 'only reached one/two operand ABIs are supported'
    context=control_observation(events,count);top,tag=context['top'],context['tag']
    rows=re.findall(r'^x87-input (st[01]) physical (\d+) tag (\d+) requested (\d+) actual (\d+) success (\d+) ([0-9a-f]{20})$',events,re.M)
    assert len(rows)==count and [row[0] for row in rows]==['st'+str(i) for i in range(count)],'missing x87 operand'
    inputs=[]
    for i,(register,physical,kind,requested_bytes,read_bytes,read_succeeded,raw) in enumerate(rows):
        physical,kind,requested_bytes,read_bytes,read_succeeded=map(int,(physical,kind,requested_bytes,read_bytes,read_succeeded))
        assert physical==((top+i)&7) and kind==((tag>>(2*physical))&3) and kind!=3,'empty/mismapped x87 input'
        assert requested_bytes==read_bytes==10 and read_succeeded==1,'short/unreadable x87 operand'
        inputs.append(dict(register=register,physical_index=physical,tag=kind,requested_bytes=10,read_bytes=10,read_succeeded=True,raw=raw))
    return dict(context,inputs=inputs)


def validate_range_inputs(name,description,receipt,events,native,anchors,config):
    """Bind only this reached double-conversion caller's two real operands."""
    import copy,hashlib,struct,json
    assert name=='_check_extended_range' and description.get('x87_inputs'), 'unsupported x87 ABI transport'
    assert not description['formals'] and not description['hidden_inputs'] and not description['unknown_inputs']
    rows=description['register_inputs']
    assert [row['name'] for row in rows]==['st0','st1'], 'missing or extra x87 formal'
    old=original_observation(events);assert old==receipt['x87_inputs'],'x87 receipt differs from raw original context'
    new=native['x87_inputs']
    assert new['requested']==new['actual']==2 and new['context_read']
    for key in ['control','status','tag','top']:assert old[key]==new[key], 'x87 entry environment differs: '+key
    assert [row['register'] for row in new['inputs']]==['st0','st1']
    raw_native=re.findall(r'^st([01])\s+.*\(raw 0x([0-9a-f]{20})\)\s*$',new['raw_gdb'],re.M)
    assert [name for name,value in raw_native]==['0','1']
    assert [bytes.fromhex(value)[::-1].hex() for name,value in raw_native]==[row['raw'] for row in new['inputs']], 'native x87 raw register record differs'
    owner=description['x87_owner']
    assert owner==dict(original_function='_extended_to_double',native_function='extended_to_double',native_local='value',native_field='caller_value',original_register='esi',return_rva=0x64e12,source_address=0x97ba8,limit_address=0xa66de), 'wrong range operand owner binding'
    assert receipt['terminal']==name and receipt['candidate'] is None
    assert re.findall(r'^caller ([0-9a-f]+)$',events,re.M)==['00064e12'],'wrong genuine range helper caller'
    projection=receipt['ownership_entry_projection'];assert projection['observed']==projection['expected']==['_extended_to_double']
    assert projection['events_sha256']==hashlib.sha256(events.encode()).hexdigest()
    assert projection['accepted_prefix_sha256']==hashlib.sha256(json.dumps(config['accepted'],separators=(',',':')).encode()).hexdigest()
    caller=next(row for row in config['driver'] if row['name']=='_extended_to_double');assert caller['name'] in config['accepted']
    assert re.search(r'^entry _extended_to_double rva %08x byte 55$'%(caller['start']-config['target']['base']),events,re.M)
    original_owner=receipt['ownership_register_observations'];assert len(original_owner)==1 and original_owner[0]['register']=='esi' and original_owner[0]['read_succeeded'] and original_owner[0]['value']==0x97ba8
    actual_owner=native['caller_locals']['caller_value'];assert actual_owner==dict(function='extended_to_double',local='value',fields=[],value=native['value_owner']), 'wrong actual native x87 value owner'
    bound=[]
    for i,(row,prefix,address) in enumerate(zip(rows,['limit','value'],[0xa66de,0x97ba8])):
        expected=dict(kind='x87-memory-words',original_address=address,native_fields=[prefix+'_low',prefix+'_high',prefix+'_exponent'])
        assert row['binding']==expected, 'wrong x87 operand memory binding'
        parts=[]
        for field,offset,width in zip(expected['native_fields'],[0,4,8],[4,4,2]):
            read=anchors[field]
            assert read['original']==address+offset and read['bytes']==read['read_bytes']==width and read['read_succeeded'],'wrong/short x87 memory read'
            assert read['value']==native[field], 'actual x87 memory values differ'
            parts.append(read['value'])
        encoded=struct.pack('<IIH',*parts).hex()
        left,right=old['inputs'][i],new['inputs'][i]
        assert right['register']=='st'+str(i) and right['physical_index']==((new['top']+i)&7)
        assert right['tag']==((new['tag']>>(2*right['physical_index']))&3)!=3
        assert right['requested_bytes']==right['read_bytes']==10 and right['read_succeeded']
        assert left['raw']==right['raw']==encoded, 'x87 operand slot/memory mapping differs'
        result=copy.deepcopy(row);result.update(original_raw=left['raw'],native_raw=right['raw'],result='match');bound.append(result)
    return dict(registers=bound,environment=dict(original={k:old[k] for k in ['control','status','tag','top']},native={k:new[k] for k in ['control','status','tag','top']}))


def validate_store_inputs(name,description,receipt,events,native,anchors,config):
    """Actual ESI/ST0 inputs of the four-byte store reached from double wrapper."""
    import copy,hashlib,json
    assert name=='_store_extended' and description.get('x87_single') and not description.get('x87_inputs')
    assert not description['formals'] and not description['hidden_inputs'] and not description['unknown_inputs']
    rows=description['register_inputs'];assert [row['name'] for row in rows]==['esi','st0']
    owner=description['x87_owner']
    assert owner==dict(original_function='_double_to_extended',native_function='double_to_extended',native_local='result',native_field='caller_result',original_register='esi',return_rva=0x64e81,destination_address=0x97ba8), 'wrong store input owner binding'
    assert receipt['terminal']==name and receipt['candidate'] is None
    assert re.findall(r'^caller ([0-9a-f]+)$',events,re.M)==['00064e81'], 'wrong actual store caller'
    projection=receipt['ownership_entry_projection'];assert projection['observed']==projection['expected']==['_double_to_extended']
    assert projection['events_sha256']==hashlib.sha256(events.encode()).hexdigest()
    assert projection['accepted_prefix_sha256']==hashlib.sha256(json.dumps(config['accepted'],separators=(',',':')).encode()).hexdigest()
    caller=next(row for row in config['driver'] if row['name']=='_double_to_extended');assert caller['name'] in config['accepted']
    assert re.search(r'^entry _double_to_extended rva %08x byte 55$'%(caller['start']-config['target']['base']),events,re.M)
    registers=receipt['ownership_register_observations'];assert len(registers)==1 and registers[0]['register']=='esi' and registers[0]['read_succeeded']
    assert registers[0]['value']==0x97ba8, 'actual original store destination differs'
    actual_owner=native['caller_locals']['caller_result'];assert actual_owner==dict(function='double_to_extended',local='result',fields=[],value=native['result_owner']), 'wrong actual native store result owner'
    assert native['raw_esi']==native['result_owner'], 'actual native ESI differs from caller result'
    assert rows[0]['binding']==dict(kind='object-offset',original_object=0x97ba8,offset=0,native_anchor_field='result_owner')
    assert rows[1]['binding']==dict(kind='x87-caller-load',original_instruction=0x74e71,bytes=8,native_function='double_to_extended',native_local='value'), 'wrong actual caller FLDQWORD def-use'
    state=anchors['result_format'];assert state['original']==0x97bb2 and state['bytes']==state['read_bytes']==2 and state['read_succeeded'] and state['value']==native['result_format']==8, 'store owner format word differs'
    old=original_observation(events,1);assert old==receipt['x87_inputs'], 'raw original ST0 differs from receipt'
    new=native['x87_inputs'];assert new['requested']==new['actual']==1 and new['context_read']
    for key in ['control','status','tag','top']:assert old[key]==new[key], 'store x87 entry environment differs: '+key
    assert len(new['inputs'])==1
    raw=re.findall(r'^st0\s+.*\(raw 0x([0-9a-f]{20})\)\s*$',new['raw_gdb'],re.M);assert len(raw)==1
    left,right=old['inputs'][0],new['inputs'][0]
    assert right['register']=='st0' and right['physical_index']==new['top'] and right['tag']==((new['tag']>>(2*new['top']))&3)!=3
    assert right['requested_bytes']==right['read_bytes']==10 and right['read_succeeded']
    assert left['raw']==right['raw']==bytes.fromhex(raw[0])[::-1].hex(), 'actual store ST0 values differ'
    pointer=copy.deepcopy(rows[0]);pointer.update(original_raw=registers[0]['value'],native_raw=native['raw_esi'],result='match')
    value=copy.deepcopy(rows[1]);value.update(original_raw=left['raw'],native_raw=right['raw'],result='match')
    return dict(registers=[pointer,value],environment=dict(original={k:old[k] for k in ['control','status','tag','top']},native={k:new[k] for k in ['control','status','tag','top']}))


def validate_finish_inputs(name,description,receipt,events,native,anchors,config):
    """Only the actual multiply caller's ESI/ST0 result-finishing inputs."""
    import copy,hashlib,json
    assert name=='_finish_extended_arithmetic' and description.get('x87_single') and not description.get('x87_inputs')
    assert not description['formals'] and not description['hidden_inputs'] and not description['unknown_inputs']
    rows=description['register_inputs'];assert [row['name'] for row in rows]==['esi','st0']
    owner=description['x87_owner']
    assert owner==dict(original_function='_multiply_extended',native_function='multiply_extended',native_local='result',native_field='caller_result',original_register='esi',original_frame_register='ebp',original_frame_offset=44,return_rva=0x64d7f), 'wrong actual arithmetic finisher owner'
    assert receipt['terminal']==name and receipt['candidate'] is None
    assert re.findall(r'^caller ([0-9a-f]+)$',events,re.M)==['00064d7f'], 'wrong actual finisher caller'
    projection=receipt['ownership_entry_projection'];assert projection['observed']==projection['expected']==['_multiply_extended']
    assert projection['events_sha256']==hashlib.sha256(events.encode()).hexdigest()
    assert projection['accepted_prefix_sha256']==hashlib.sha256(json.dumps(config['accepted'],separators=(',',':')).encode()).hexdigest()
    caller=next(row for row in config['driver'] if row['name']=='_multiply_extended');assert caller['name'] in config['accepted']
    assert re.search(r'^entry _multiply_extended rva %08x byte 55$'%(caller['start']-config['target']['base']),events,re.M)
    registers=receipt['ownership_register_observations'];assert [row['register'] for row in registers]==['esi','ebp'] and all(row['read_succeeded'] for row in registers)
    for row in registers:assert re.findall(r'^ownership-anchor '+row['register']+r' ([0-9a-f]{8})$',events,re.M)==['%08x'%row['value']], 'raw caller owner receipt differs'
    original_result=registers[0]['value'];assert 0<registers[1]['value']<=0xffffffff-56 and original_result==registers[1]['value']+44, 'actual original result owner differs'
    actual_owner=native['caller_locals']['caller_result'];assert actual_owner==dict(function='multiply_extended',local='result',fields=[],value=native['raw_esi']), 'actual native ESI/result formal differ'
    assert rows[0]['binding']==dict(kind='caller-result-pointer',original_frame_register='ebp',original_frame_offset=44,native_owner_field='caller_result')
    assert rows[1]['binding']==dict(kind='x87-caller-arithmetic',original_instruction=0x74d47,operation='fmulp',native_function='multiply_extended'), 'wrong actual FMULP def-use'
    state=anchors['result_format'];assert state['original']==original_result+10 and state['bytes']==state['read_bytes']==2 and state['read_succeeded'] and state['value']==native['result_format']==8, 'actual readable initialized result format differs'
    old=original_observation(events,1);assert old==receipt['x87_inputs'], 'original raw result ST0 differs'
    new=native['x87_inputs'];assert new['requested']==new['actual']==1 and new['context_read'] and len(new['inputs'])==1
    for key in ['control','status','tag','top']:assert old[key]==new[key], 'finisher x87 entry environment differs: '+key
    raw=re.findall(r'^st0\s+.*\(raw 0x([0-9a-f]{20})\)\s*$',new['raw_gdb'],re.M);assert len(raw)==1
    left,right=old['inputs'][0],new['inputs'][0]
    assert right['register']=='st0' and right['physical_index']==new['top'] and right['tag']==((new['tag']>>(2*new['top']))&3)!=3
    assert right['requested_bytes']==right['read_bytes']==10 and right['read_succeeded']
    assert left['raw']==right['raw']==bytes.fromhex(raw[0])[::-1].hex(), 'actual arithmetic result ST0 differs'
    pointer=copy.deepcopy(rows[0]);pointer.update(original_raw=original_result,native_raw=native['raw_esi'],result='match')
    value=copy.deepcopy(rows[1]);value.update(original_raw=left['raw'],native_raw=right['raw'],result='match')
    return dict(registers=[pointer,value],environment=dict(original={k:old[k] for k in ['control','status','tag','top']},native={k:new[k] for k in ['control','status','tag','top']}))
