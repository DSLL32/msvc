"""Full canonical qsort callback/return/array/error/x87 evidence for this case."""
import struct
import re


def validate(config,original,native,expected,actual,current,current_native,permutation):
    old=original['sort_calls'];new=native['sort_calls']
    assert old and len(old)==len(new)<=4096,'complete sort callback inventory differs'
    base=original['module_bases']['msvcrt.dll'];abi=original['sort_crt_abi']
    assert abi['base']==base and abi['bytes']==bytes.fromhex('e888b1ffff83c008c3e891b1ffff83c00cc3')+(base+0x3d004).to_bytes(4,'little'),'sort canonical CRT error-access ABI differs'
    def error_state(row):
        assert len(row['raw_errors'])==32 and len(row['fp'])==112 and len(row['nonvolatile'])==4,'incomplete original sort error/FP/register state'
        values=struct.unpack('<8I',row['raw_errors'])
        assert values[0]==row['teb'] and values[1]==values[4]==row['tid'] and values[3]==row['ptd'] and row['ptd'] and row['tls_index']<64 and row['tls_slot']==base+0x3d004,'sort actual original thread/TLS/PTD identity differs'
        return [values[6],values[7],values[2]]
    def native_state(row):
        names={'st'+str(i) for i in range(8)}|{'fctrl','fstat','ftag','fiseg','fioff','foseg','fooff','fop'}
        assert set(row['fp'])==names and len(row['nonvolatile'])==4,'incomplete native sort x87/register schema'
        assert all(re.fullmatch('0x[0-9a-f]{'+('20' if name.startswith('st') else '8')+'}',value) for name,value in row['fp'].items()),'native raw x87 register width/encoding differs'
        assert all(row[name] for name in ['errno_address','doserrno_address','last_error_address']),'native sort error storage lacks actual identity'
        return [row[name] for name in ['errno','doserrno','last_error']]
    before=original['sort_environments']['before'];native_before=native['sort_environments']['before'];errors=error_state(before)
    native_state(native_before)
    assert errors==[native_before[name] for name in ['errno','doserrno','last_error']],'sort initial original/native error values differ'
    for phase in ('before','after'):
        left=original['sort_environments'][phase];right=native['sort_environments'][phase]
        native_state(right)
        assert error_state(left)==errors and left['raw_errors']==before['raw_errors'] and left['teb']==before['teb'] and left['ptd']==before['ptd'] and left['tls_index']==before['tls_index'],'sort wrapper changed original error identity/state'
        assert left['fp']==before['fp'],'sort wrapper changed original x87 state'
        assert right['fp']==native_before['fp'] and all(right[name]==native_before[name] for name in ['errno','doserrno','last_error','errno_address','doserrno_address','last_error_address']),'sort wrapper changed native error/x87 state'
    assert original['sort_environments']['after']['nonvolatile']==before['nonvolatile'] and native['sort_environments']['after']['nonvolatile']==native_before['nonvolatile'],'sort wrapper normal-return registers differ'
    wrapper=original['sort_wrapper'];assert wrapper['before']['calls']==0 and wrapper['after']['calls']==len(old) and wrapper['before']['esp']==wrapper['after']['esp'],'sort original wrapper call/stack/count differs'
    assert native['directory_sort_complete']['exit_esp']==native['directory_sort_input']['entry_esp']+4,'sort native wrapper cdecl return differs'
    root=original['directory_sort_input']['address'];other=native['directory_sort_input']['address']
    for index,(left,right) in enumerate(zip(old,new)):
        assert left['sequence']==right['sequence']==index and left['return_pc']-base in (0x2551e,0x25534,0x2560c),'sort actual callback sequence/provider site differs'
        assert left['entry_esp']==left['exit_esp'] and left['return_pc']==left['return_pc_after'] and right['exit_esp']==right['entry_esp']+4,'sort actual callback cdecl stack/return differs'
        assert set(right['formal_inventory'])=={'first','second'} and all(value=='const void *' for value in right['formal_inventory'].values()),'sort actual callback formal inventory differs'
        assert len(left['records'])==len(bytes.fromhex(right['records']))==24,'sort actual callback initialized input incomplete'
        for field,offset in [('first',0),('second',12)]:
            a=left[field]-root;b=right[field]-other
            assert a==b and 0<=a<696 and a%12==0,'sort actual callback argument row/owner differs'
            row=left['records'][offset:offset+12];actual_row=bytes.fromhex(right['records'])[offset:offset+12]
            matches=[i for i,value in enumerate(expected) if value==row]
            assert len(matches)==1 and actual_row==actual[matches[0]],'sort actual callback typed row input differs'
        assert left['result']==right['result'] and left['result'] in (-1,0,1),'sort actual comparator signed return differs'
        for phase in ('before','after'):
            a=left[phase];b=right[phase]
            native_state(b)
            assert error_state(a)==errors and a['raw_errors']==before['raw_errors'] and a['teb']==before['teb'] and a['ptd']==before['ptd'] and a['tls_index']==before['tls_index'],'sort callback changed original error identity/state'
            assert a['fp']==before['fp'],'sort callback changed original x87 state'
            assert b['fp']==native_before['fp'] and all(b[name]==native_before[name] for name in ['errno','doserrno','last_error','errno_address','doserrno_address','last_error_address']),'sort callback changed native error/x87 state'
        assert left['before']['nonvolatile']==left['after']['nonvolatile'] and right['before']['nonvolatile']==right['after']['nonvolatile'],'sort callback normal-return registers differ'
    complete=original['directory_sort_complete'];native_complete=native['directory_sort_complete']
    assert complete['address']==root and native_complete['address']==other and complete['count']==native_complete['count']==58 and complete['capacity']==native_complete['capacity']==912,'sort completed array owner/count/capacity differs'
    assert complete['bytes']==current and bytes.fromhex(native_complete['bytes'])==current_native,'sort completed initialized array/current identity differs'
    expected_order=sorted(range(58),key=lambda i:(0 if int.from_bytes(expected[i][:2],'little')==0x120 else 1,int.from_bytes(expected[i][2:4],'little'),int.from_bytes(expected[i][:2],'little')))
    assert permutation==expected_order,'complete sort final logical row order differs'
    control=struct.unpack('<I',before['fp'][:4])[0]&0xffff;native_control=int(native_before['fp']['fctrl'],16)&0xffff
    assert control==native_control,'native startup x87 control differs from canonical CRT baseline'
    original_status,original_tags=[struct.unpack('<I',before['fp'][offset:offset+4])[0]&0xffff for offset in (4,8)]
    assert original_status==(int(native_before['fp']['fstat'],16)&0xffff) and original_tags==(int(native_before['fp']['ftag'],16)&0xffff),'native startup x87 status/tags differ from canonical CRT baseline'
    return dict(callbacks=len(old),complete_callback_input_result_stack_sequence=True,complete_initialized_postsort_bytes=840,active_sorted_rows=58,ordinary_callback_nonvolatile_registers=['ebx','esi','edi','ebp'],error_values=dict(errno=errors[0],doserrno=errors[1],last_error=errors[2]),actual_original_thread=dict(teb=before['teb'],tid=before['tid'],tls_index=before['tls_index'],ptd=before['ptd']),original_x87_control=control,native_x87_control=native_control,original_x87_status=original_status,original_x87_tags=original_tags,native_x87_status=int(native_before['fp']['fstat'],16)&0xffff,native_x87_tags=int(native_before['fp']['ftag'],16)&0xffff,original_x87_raw_bytes=112,native_x87_raw_registers=sorted(native_before['fp']),x87_preserved_with_each_side_own_raw_baseline=True)
