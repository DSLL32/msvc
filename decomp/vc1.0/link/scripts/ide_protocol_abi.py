"""Pure typed-state checks for the direct IDE interface and external callbacks."""
import struct
FUNCTIONS={2:'initialize_ide_api',10:'read_ide_flags'}
COUNTS={'initialize_ide_api':1,'emit_ide_message':2,'read_ide_flags':1}
def function(phase):return FUNCTIONS.get(phase,'emit_ide_message')
def normalize_probe(packet,callbacks):
    data=bytes.fromhex(packet['probe']);assert len(data)==1968
    fields=struct.unpack_from('<6I',data)
    assert fields==(*callbacks[:2],0,callbacks[2],packet['probe_address']+56,1024),'typed callback table/owner'
    # Only the four proven pointer fields are represented by the checked
    # logical callback/buffer ownership above. All remaining bytes compare raw.
    copy=bytearray(data)
    for off in [0,4,12,16]:copy[off:off+4]=b'\0'*4
    assert data[24:56]==data[1080:1112]==b'\xa5'*32,'probe guards'
    return bytes(copy)
def compare_entry(native,original,original_module,exports):
    phase=original['phase'];name=function(phase);assert native['phase']==phase and native['name']==name
    assert native['event']==original['event']=='target_entry'
    assert len(native['arguments'])==len(original['arguments'])==COUNTS[name]+1
    callback_addresses=[original_module+exports[n]['rva'] for n in ['IdeProbeChannel','IdeProbeWrite','IdeProbeAssertion']]
    assert normalize_probe(native,native['callback_addresses'])==normalize_probe(original,callback_addresses)
    assert bytes.fromhex(native['message_inputs'])==bytes.fromhex(original['message_inputs'])
    assert native['mb_cur_max']==original['mb_cur_max']==1 and native['ctype']==original['ctype']
    assert len(native['errors'])==len(original['errors'])==3 and native['errors']==original['errors']
    assert native['pending']==original['pending']
    for i in [1,3,4,5,6,7]:assert native['stdout_file'][i]==original['stdout_file'][i]
    assert len(native['out_slot'])==len(original['out_slot'])==136
    if phase<=10:assert native['out_slot']==original['out_slot']=='a5'*68,'owned4-byte output storage+guards before write'
    if phase==2:
        assert native['arguments'][1]==native['probe_address'] and original['arguments'][1]==original['probe_address'],'actual API formal'
        # Old global API/channel are overwritten before any read: not inputs.
    elif phase<=9:
        assert native['arguments'][1]==original['arguments'][1]==phase-3,'actual tag formal'
        if phase in [4,5,6,7]:
            data=bytes.fromhex(native['message_inputs']);expected=[b'entry',b'source.obj',b'library.lib',b'symbol_name'][phase-4]
            offset=(phase-4)*128+32;assert data[offset:offset+len(expected)+1]==expected+b'\0'
            assert native['message_values'][phase-3]==native['arguments'][2]==native['message_base']+offset,'native string owner formal'
            assert original['message_values'][phase-3]==original['arguments'][2]==original['message_base']+offset,'original string owner formal'
        else:assert native['arguments'][2]==original['arguments'][2]=={3:0,8:0xfedcba98,9:0x12345678}[phase]
        assert native['globals']==[native['probe_address'],7] and original['globals']==[original['probe_address'],7]
    else:
        assert native['arguments'][1]==native['out_slot_address'] and original['arguments'][1]==original['out_slot_address'],'actual char** formal'
    return dict(name=name,phase=phase,formal_count=COUNTS[name],callback_owner='exact typed external callback symbols and owned buffer',probe='all nonpointer bytes match, four pointer fields have mandatory logical ownership',errors=native['errors'],locale='all257ctype entries/mb_cur_max match',stdout='complete defined FILE fields/pending bytes match')

def compare_packets(native,original,original_module,exports):
    assert native and original,'mandatory actual packet inventories'
    callback_addresses=[original_module+exports[n]['rva'] for n in ['IdeProbeChannel','IdeProbeWrite','IdeProbeAssertion']]
    assert native['event']==original['event'] and native['phase']==original['phase']
    assert normalize_probe(native,native['callback_addresses'])==normalize_probe(original,callback_addresses)
    for field in ['message_inputs','pending','errors','mb_cur_max','ctype']:
        assert native[field]==original[field],('actual state',field,native['event'],native['phase'])
    assert len(native['errors'])==3
    for i in [1,3,4,5,6,7]:assert native['stdout_file'][i]==original['stdout_file'][i]
    if original['phase']<10 or original['event']!='target_return':assert native['out_slot']==original['out_slot']=='a5'*68
    else:
        actual_out=bytes.fromhex(native['out_slot']);original_out=bytes.fromhex(original['out_slot'])
        assert actual_out[:32]==actual_out[36:]==original_out[:32]==original_out[36:]==b'\xa5'*32
        assert native['environment']['owned'] is True and native['environment']['pointer']==native['expected_environment']==struct.unpack_from('<I',actual_out,32)[0]
        assert native['environment']['value']==b'alpha.obj,beta.lib\0'.hex()
        assert original['environment']['owned_CRT_environment'] is True and original['environment']['pointer']==struct.unpack_from('<I',original_out,32)[0]
    if original['phase']>=2 and native['globals'] is not None:
        assert native['globals'][1]==original['globals'][1]
        assert (native['globals'][0]==native['probe_address'])==(original['globals'][0]==original['probe_address'])
    if native['event']=='target_entry':return compare_entry(native,original,original_module,exports)
    if native['event']=='channel_entry':
        assert native['actual_target_caller']=='initialize_ide_api'
        assert original['arguments'][0]==0x3570d and original['arguments'][1:]==native['arguments'][1:]==[2]
    elif native['event']=='write_entry':
        assert native['actual_target_caller']=='emit_ide_message' and original['arguments'][0]==0x356f3
        assert native['arguments'][1]==original['arguments'][1]==7
        assert native['arguments'][2]==native['probe_address']+56 and original['arguments'][2]==original['probe_address']+56
        assert native['arguments'][3]==original['arguments'][3]
        length=native['arguments'][3];assert 0<length<1024
        assert bytes.fromhex(native['probe'])[56:56+length]==bytes.fromhex(original['probe'])[56:56+length]
    elif native['event'] in ['target_return','channel_return','write_return']:
        assert native['eax']==original['eax'],'defined return'
        assert native['paired_entry_phase']==native['active_target_phase']==native['phase'],'fresh return-entry association'
    else:raise AssertionError('unknown packet kind')
    return dict(event=native['event'],phase=native['phase'],status='pass')

def compare_history(native,original,exports):
    """Validate a complete direct test, including its actual entry/return pairs."""
    kinds={'target_entry','target_return','channel_entry','channel_return','write_entry','write_return'}
    left=native['events'];right=[x.copy() for x in original if x['event'] in kinds]
    expected=[]
    for phase in range(2,11):
        expected.append(('target_entry',phase))
        if phase<10:
            prefix='channel' if phase==2 else 'write'
            expected.extend([(prefix+'_entry',phase),(prefix+'_return',phase)])
        expected.append(('target_return',phase))
    assert [(x['event'],x['phase']) for x in left]==[(x['event'],x['phase']) for x in right]==expected,'mandatory complete entry/callback/return inventory'
    assert native['terminal']==[dict(event='exit',code=0)]
    env=[x for x in original if x['event']=='environment_result']
    assert len(env)==1 and env[0]['owned_CRT_environment'] is True
    right[-1]['environment']=env[0]
    module=[x['eax'] for x in original if x['event']=='external_return' and x['phase']==0]
    assert len(module)==1 and module[0]>0
    checks=[compare_packets(a,b,module[0],exports) for a,b in zip(left,right)]
    for history in [left,right]:
        stack=[]
        for event in history:
            if event['event'].endswith('_entry'):stack.append(event)
            else:
                assert stack,'return without actual entry'
                before=stack.pop()
                assert event['phase']==before['phase'] and event['event'].replace('_return','_entry')==before['event']
                assert event['eip']==before['arguments'][0] and event['esp']==before['esp']+4,'cdecl return stack'
                assert all(event[r]==before[r] for r in ['ebx','esi','edi','ebp']),'nonvolatile registers'
        assert not stack
    return checks
