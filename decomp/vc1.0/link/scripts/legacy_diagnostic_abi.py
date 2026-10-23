"""Pure validation of the external callback's real formals and return contract."""
import hashlib,struct

def validate(actual,original,payload,label,returns):
    assert payload and label in ('strings','menu','dialog')
    owner=next(x for x in original if x['event']=='external_driver')
    expected_entry=next(x for x in original if x['event']=='entry')
    assert expected_entry['arguments']==[owner['sentinel'],owner['input'],owner['size']]
    assert owner['size']==len(payload) and owner['input']
    assert actual and 'entry' in actual
    checks={}
    for key in ['entry']+(['return'] if returns else []):
        a=actual[key];b=next(x for x in original if x['event']==key)
        assert a['mb_cur_max']==b['mb_cur_max']==1
        old=list(struct.unpack('<257H',bytes.fromhex(b['ctype'])))
        new=list(struct.unpack('<257H',bytes.fromhex(a['ctype'])))
        assert old==new,'full C ctype table mismatch'
        assert a['errors']==b['errors'] and len(a['errors'])==3,('error slots',key)
        assert a['buffered']==b['buffered'],('pending stdout bytes',key)
        assert a['guards']=='a5'*64 and bytes.fromhex(a['payload'])==payload,('owned payload/guards',key)
        assert a['input_size']==len(payload) and a['input_pointer']
        assert len(a['stdout_file'])==len(b['stdout_file'])==8
        for index in [1,3,4,5,6,7]:
            assert a['stdout_file'][index]==b['stdout_file'][index],('FILE state',key,index)
        assert (a['stdout_file'][0]-a['stdout_file'][2])==len(bytes.fromhex(a['buffered']))
        if key=='entry':
            assert len(a['arguments'])==3
            assert a['arguments'][1]==a['input_pointer'] and a['arguments'][2]==len(payload),'actual cdecl formals'
        checks[key]=dict(formals='two actual cdecl DWORDs: owned payload pointer and exact byte count',payload_sha256=hashlib.sha256(payload).hexdigest(),size=len(payload),guards='64bytes A5 unchanged',locale='All257 original/native C ctype entries and mb_cur_max1 match',errors=a['errors'],stdout='complete defined FILE fields, logical buffer occupancy and pending bytes match')
    if returns:
        before=actual['entry'];after=actual['return']
        assert after['eip']==before['arguments'][0] and after['esp']==before['esp']+4,'cdecl stack return'
        for name in ['ebx','esi','edi','ebp']:
            assert after['registers'][name]==before['registers'][name],('callee-saved',name)
        assert after['payload']==before['payload'] and after['guards']==before['guards']
    return checks
