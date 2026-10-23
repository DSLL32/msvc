"""Pure actual-input/state checks for externally called COMDAT diagnostics."""
import struct
from pathlib import Path
NAMES={0x17600:'dump_comdat_graph',0x3a880:'debug_printf',0x17540:'print_comdat_node',0x17520:'comdat_usage_label',0x175e0:'print_comdat_edge'}
def compare_entry(native,original,payload):
    name=NAMES[original['eip']];assert native['name']==name
    assert native['event']=='entry' and original['event'] in ['entry','frontier']
    for event in [native,original]:
        assert event['input_base']==0x12000000 and bytes.fromhex(event['region'])==payload and len(payload)==4096
        assert event['debug_flags']=='00'*100 and event['debug_log']==0
        assert event['mb_cur_max']==1 and len(bytes.fromhex(event['ctype']))==514
    for key in ['errors','ctype','mb_cur_max','pending','format','string_value']:assert native[key]==original[key],key
    for i in [1,3,4,5,6,7]:assert native['stdout_file'][i]==original['stdout_file'][i]
    expected_count=3 if name=='debug_printf' and b'%' in bytes.fromhex(original['format']) else 2
    assert len(native['arguments'])==len(original['arguments'])==expected_count,'actual formal/consumed-word inventory'
    if name=='debug_printf':
        assert original['arguments'][1] and native['arguments'][1] and original['format']
        if expected_count==3:
            if original['format']==b'|con=0x%p'.hex():assert native['arguments'][2]==original['arguments'][2] in [0x12000100,0x12000140]
            elif original['format']==b'%s'.hex():assert native['string_value']==original['string_value'] in [b'R'.hex(),b'N'.hex()]
            else:assert native['arguments'][2]==original['arguments'][2],'caller-owned string pointer identity'
    else:assert native['arguments'][1:]==original['arguments'][1:],'actual caller-owned graph formal'
    return dict(name=name,status='pass',formal_count=1,consumed_variadic_words=expected_count-2,owned_input_bytes=4096,prestate='Complete defined FILE/pending/257ctype/MB_CUR_MAX/error/debug state match')

def compare_history(native,original,payload,pe,complete=False):
    """Paired prefix or complete invocation, with actual call/return inventories."""
    left=[x for x in native['events'] if x['event']!='frontier']
    right=[x for x in original if x['event'] in ['entry','return','frontier']]
    assert left and right and len(left)==len(right),'mandatory actual packet inventory'
    stacks=[[],[]];checks=[]
    for a,b in zip(left,right):
        assert a['event']==('entry' if b['event']=='frontier' else b['event'])
        assert a['name']==('dump_comdat_graph' if b['name']=='print_comdat_graph' else b['name'])
        for x in [a,b]:
            assert bytes.fromhex(x['region'])==payload and x['input_base']==0x12000000
            assert x['debug_flags']=='00'*100 and x['debug_log']==0
        for key in ['errors','ctype','mb_cur_max','pending']:assert a[key]==b[key],(a['event'],a['name'],key)
        for i in [1,3,4,5,6,7]:assert a['stdout_file'][i]==b['stdout_file'][i]
        if a['event']=='entry':
            checks.append(compare_entry(a,b,payload))
            if b['event']!='frontier':
                if stacks[1]:
                    if complete:assert a['actual_caller']==stacks[0][-1]['name'],'actual named native caller'
                    caller=b['arguments'][0];raw=pe.read(caller-5,5)
                    assert raw[0]==0xe8 and caller+struct.unpack_from('<i',raw,1)[0]==b['eip'],'actual original direct callsite'
                if complete and not stacks[1]:assert a['actual_caller']=='__wrap_main','explicit external caller'
                for stack,x in zip(stacks,[a,b]):stack.append(x)
        else:
            for stack,x in zip(stacks,[a,b]):
                before=stack.pop();assert x['esp']==before['esp']+4 and x['eip']==before['arguments'][0]
                assert all(x[r]==before[r] for r in ['ebx','esi','edi','ebp']),'nonvolatile register'
            if a['name']=='comdat_usage_label':
                assert b['eax'] in [0x7a048,0x7a04c]
                assert bytes.fromhex(a['return_text'])==pe.string(b['eax']).encode()
            else:assert a['eax']==b['eax'],'defined result'
    if complete:
        assert not any(stacks) and native['terminal']==[dict(event='exit',code=0)]
        assert original[-1]==dict(event='summary',failed=0,stopped=1,exit=0,stdout_bytes=287,stderr_bytes=0)
    else:assert left[-1]['event']=='entry' and right[-1]['event']=='frontier'
    return checks
