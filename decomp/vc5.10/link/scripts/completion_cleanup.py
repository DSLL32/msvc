"""Typed final callback lifetime proof; debugger state is separate from output replay."""
import hashlib
import json
import struct


def verify(original, native, state=None, events=None):
    assert json.loads((native/'environment/binding.json').read_text())['reference_sha256']==hashlib.sha256((original/'receipt.json').read_bytes()).hexdigest()
    if state is None:state=json.loads((native/'state.json').read_text())
    if events is None:events=[json.loads(line)for line in (original/'events.jsonl').read_text().splitlines()]
    configuration=json.loads((native/'functions.json').read_text())
    applications={record['name']for record in configuration['driver']}
    assert state['entries']==[event['name']for event in events if event['event']=='entry'and event['name']in applications]
    assert state['exit_status']==76 and 'signal_number' not in state
    created=state['new_ownership'];remaining=state['cleanup_ownership']
    assert len(created)==len(remaining)==3
    assert len({r['generation']for r in created})==len({r['generation']for r in remaining})==3
    roles=list(zip(['12','8','4'],[12288,4000,4096],[1024,500,1024],created))
    assert len(state['delete_calls'])==3
    observations=[event for event in events if event['event']=='operator_delete_observation']
    assert len(observations)==6
    for index,(suffix,size,count,record)in enumerate(reversed(roles)):
        allocated=next(event for event in events if event.get('name')=='observe:new_return_'+suffix)
        assert allocated['stack_top']==size and allocated['eax']
        assert record['requested']==record['rounded']==size and record['live']==1
        assert [r for r in remaining if r['generation']==record['generation']]==[dict(record,live=0)]
        assert struct.unpack('<3I',bytes.fromhex(state['cleanup_descriptors']['buffer'+suffix]))==(record['base'],count,count)
        assert state['delete_calls'][index]==dict(argument=record['base'],normal_return=True,before=record,after=dict(record,live=0))
        call,returned=observations[2*index:2*index+2]
        entry={'4':0x431e30,'8':0x431dc0,'12':0x424eb0}[suffix]
        assert (call['name'],returned['name'])==('observe:delete_call_'+suffix,'observe:delete_return_'+suffix)
        assert (call['site'],returned['site'])==(entry+10,entry+15)
        assert call['stack_pointer']==returned['stack_pointer']
        assert call['argument']==returned['argument']==allocated['eax']
        assert call['descriptor']==returned['descriptor']==[allocated['eax'],count,count]
    return dict(allocations=3,normal_deletes=3,remaining_live=0,
        contract='Each constructor generation is deleted once in reverse order, with descriptors retained; original cdecl call/return stack and arguments agree.')
