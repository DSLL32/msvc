"""Actual Linux callback return/API4/frontier proof; no target-state reseeding."""
import json
from pathlib import Path
from clock_state import compare_fp
from mre_state import state


def indirect_call(layout,address,owner):
    target=address-layout.bias
    calls=[(pc,b)for pc,b in layout.instructions.items()if pc+len(b)==target and layout.owners[pc]==owner]
    assert len(calls)==1
    pc,code=calls[0]
    assert len(code)>=2 and code[0]==0xff and (code[1]>>3)&7==2,'not actual indirect callback CALL'
    return pc+layout.bias


def verify_tail(provider,directory,layout,pid,observations,previous,jobs=False):
    directory=Path(directory);packet=provider.packet;original=provider.original
    if not jobs:assert packet['signature_frontier']['symbol']=='jobs_from_deferred'
    from mre_native import decode
    response=json.loads((directory/'mre-native.json').read_bytes())
    previous=decode(bytes.fromhex(response['requests'][6]['payload_hex']))
    lower=packet['lowercase_call'];records=[];buffers=[]
    def common(record,phase):
        assert record['phase']==phase and record['pid']==pid
        layout.bind(record);derived,memory=state(record)
        expected={**{k:previous[k]for k in derived},'driver_words':list(previous['driver_words']),
            'scratch':(original/('observer/'+phase+'-shared-scratch.bin')).read_bytes().hex()}
        expected['driver_words'][5]=0;expected['driver_words'][6]=derived['driver']+20
        assert derived==expected,'changed actual post-MRE typed app/CRT state at '+phase
        assert memory.word(record['symbols']['mre_call_count'])==(4 if phase=='mre-frontier'else 3)
        assert memory.word(record['symbols']['use_standard_include'])==1
        assert memory.text(derived['driver_words'][9])==packet['target_objects'][phase]['include_input']['text']
        context=(original/('observer/'+phase+'-context.bin')).read_bytes()
        compare_fp(context[28:140].hex(),record['x87'])
        assert record['x87']==observations[-1]['x87']and record['descriptor3']==observations[-1]['descriptor3']
        streams=json.loads((directory/('streams-'+phase+'.json')).read_bytes())
        before=json.loads((directory/'streams-mre-3-after.json').read_bytes())
        if phase=='mre-frontier':before['mre_calls']=4
        assert streams==before,'changed native FILE/standard-handle aliases, positions or error state'
        assert memory.read(record['data_addresses']['recon_iob'],96)==(directory/('streams-'+phase+'-iob.bin')).read_bytes()
        return derived,memory
    for phase in('lower-before','lower-after'):
        r=json.loads((directory/('native-'+phase+'-gdb.json')).read_bytes());derived,m=common(r,phase)
        a=r['arguments'];g=r['general'];assert len(a['values'])==1
        assert m.words(a['stack'],2)==[a['return'],a['values'][0]]
        layout.call(a['return'],'recon_mbslwr','option_signature')
        assert a['state']==derived['driver']==m.word(a['state_slot'])
        assert a['state_slot']==a['signature_ebp']+8
        assert m.word(a['signature_ebp'])==observations[-1]['general']['ebp']
        layout.call(m.word(a['signature_ebp']+4),'option_signature','prepare_jobs')
        text=m.text(a['values'][0]).encode('ascii')+b'\0';buffers.append(text)
        assert text.hex()==lower['before_hex'if phase=='lower-before'else'after_hex']
        provider.pointer.bind(lower['ABI']['input'],a['values'][0])
        if phase=='lower-before':
            assert g['eip']==layout.bias+layout.symbol('recon_mbslwr')and g['esp']==a['stack']
        else:
            first=records[0];assert a==first['arguments']
            assert g['eip']==a['return']and g['esp']==a['stack']+4 and g['eax']==a['values'][0]
            assert all(first['general'][name]==g[name]for name in('ebx','esi','edi','ebp'))
            assert first['x87']==r['x87']
        records.append(r)
    callback_records=[];callback_call=None
    wanted=packet['comparator_call']
    # Source retains strcmp; the exact native target object must call the
    # published profile shim directly, with no local return normalization.
    calls=[]
    import struct
    for address,code in layout.instructions.items():
        if layout.owners[address]=='compare_option_strings'and len(code)==5 and code[:1]==b'\xe8':
            target=address+5+struct.unpack_from('<i',code,1)[0]
            if target==layout.symbol('recon_strcmp'):calls.append(address)
    assert len(calls)==1,'native comparator lacks exact shared strcmp call'
    for phase in('compare-before','compare-after'):
        r=json.loads((directory/('native-'+phase+'-gdb.json')).read_bytes());derived,m=common(r,phase)
        a=r['arguments'];g=r['general'];sp=a['stack']
        assert m.words(sp,3)==[a['return'],a['left'],a['right']]
        assert a['left_slot']==sp+4 and a['right_slot']==sp+8
        callback_call=indirect_call(layout,a['return'],'recon_qsort')
        assert m.word(a['qsort_ebp'])==a['signature_ebp']
        layout.call(m.word(a['qsort_ebp']+4),'recon_qsort','option_signature')
        assert m.word(a['compare_slot'])==a['comparator']==layout.bias+layout.symbol('compare_option_strings')
        assert m.word(a['signature_ebp'])==observations[5]['general']['ebp']
        layout.call(m.word(a['signature_ebp']+4),'option_signature','prepare_jobs')
        assert a['slot']==a['signature_ebp']+8 and m.word(a['slot'])==a['state']==derived['driver']
        assert m.word(a['items_slot'])==a['items']and m.word(a['count_slot'])==a['count']
        assert 2<=a['count']<=64
        for name,index in(('left',0),('right',1)):
            actual=a[name]
            assert a['items']<=actual<a['items']+a['count']*4 and (actual-a['items'])%4==0
            value=m.text(m.word(actual)).encode('ascii')+b'\0'
            assert value.hex()==wanted['values_hex'][index]
            provider.pointer.bind(wanted['cdecl_formals'][index],actual)
            provider.pointer.bind(wanted['pointed_string_addresses'][index],m.word(actual))
        assert m.read(a['pending'],20)==bytes(20)
        if phase=='compare-before':
            assert g['eip']==layout.bias+layout.symbol('compare_option_strings')and g['esp']==sp
            assert g['ebp']==a['qsort_ebp']
        else:
            first=callback_records[0];assert a==first['arguments']
            assert g['eip']==a['return']and g['esp']==sp+4 and g['eax']==wanted['result']
            assert all(first['general'][name]==g[name]for name in('ebx','esi','edi','ebp'))
            assert first['x87']==r['x87']
            first_memory=state(first)[1]
            assert m.words(a['items'],a['count'])==first_memory.words(a['items'],a['count'])
        callback_records.append(r)
    if jobs:
        from jobs_native_state import verify_tail as verify_jobs
        return verify_jobs(provider,directory,layout,pid,observations,callback_call)
    r=json.loads((directory/'native-mre-frontier-gdb.json').read_bytes());derived,m=common(r,'mre-frontier');a=r['arguments'];g=r['general']
    pc=g['eip']-layout.bias-1
    assert layout.instructions[pc]==b'\xcc'and layout.owners[pc]=='jobs_from_deferred','not authentic deferred-jobs INT3'
    ebp=g['ebp'];assert a['slot']==ebp+8 and m.word(a['slot'])==a['inputs']==0
    assert m.word(ebp)==observations[-1]['general']['ebp']
    layout.call(m.word(ebp+4),'jobs_from_deferred','prepare_jobs')
    assert m.read(a['pending'],20)==bytes(20)
    assert (directory/'trace.txt').read_bytes()==(original/'trace.txt').read_bytes()
    assert (directory/'stdout.bin').read_bytes()==b''and(directory/'stderr.bin').read_bytes()==(original/'stderr.bin').read_bytes()
    assert (directory/'case.idb').read_bytes()==provider.idb
    return {'case':'mre-object','scope':'comparator candidate, four genuine MRE responses and deferred-jobs trap; later lifetime denied','calls':[1,1,1,1],'lowercase_calls':1,'observed_comparator_returns':1,'frontier':'jobs_from_deferred','native_pid':pid,'actual_native_ABI_state_and_file_match':True,'callback_call':callback_call,'formal_slot_qualification':'Original low byte reused with final selector0; native signature formal remains real Driver pointer, independently authenticated.'}
