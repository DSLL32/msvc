"""Actual first qsort frontier over rebuilt CodeView directory rows.

The initialized70-row input history remains separate from current58 active rows
and their unchanged12-row tail. Six allocation spare rows remain unwritten.
"""
import struct
import importlib.util
from pathlib import Path


def validate(config,data,original,native,objects):
    rows={row['name']:row for row in data}
    def integer(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    root=integer('directory_entries');other=int.from_bytes(bytes.fromhex(native['globals']['directory_entries']),'little')
    old=original['directory_before_writer'];new=native['directory_before_writer'];left=original['directory_sort_input'];right=native['directory_sort_input']
    assert old['address']==left['address']==root and new['address']==right['address']==other,'output directory actual owner aliases differ'
    assert old['count']==new['count']==70 and old['capacity']==new['capacity']==left['capacity']==right['capacity']==912,'output directory initialized input/capacity differs'
    assert len(old['bytes'])==len(bytes.fromhex(new['bytes']))==840,'output directory historical initialized input incomplete'
    assert left['count']==right['count']==integer('directory_count')==58 and left['width']==right['width']==12,'output directory qsort count/width differs'
    assert left['callback']==0x4063f0 and right['callback']==native['function_addresses']['compare_packed_directory_entries'],'output directory actual typed comparator identity differs'
    complete_sort=bool(config.get('observe_complete_directory_sort'))
    if complete_sort:
        expected=(0x4068a0,0x406c17) if config.get('observe_output_buffer_init') else (0x4100e0,0x406b84)
        assert (original['address'],original['stack'][0])==expected and native['frontier_caller']=='flush_packed_file','complete sort unknown next consumer'
    else:assert original['stack'][0]==original['module_bases']['msvcrt.dll']+0x2551e and native['frontier_caller']=='recon_qsort','output directory actual canonical qsort callback site differs'
    expected=[];actual=[];roles=[];seen=set();info=integer('module_list')
    while info:
        assert info not in seen and len(seen)<128,'output directory live module input cycle'
        seen.add(info);before=original['memory'][info];after=bytes.fromhex(native['memory'][str(objects[info])]);assert len(before)==len(after)==48
        module=int.from_bytes(before[:2],'little')
        for kind,pointer,size in [(0x120,12,16),(0x125,20,24),(0x127,28,32)]:
            count=int.from_bytes(before[size:size+4],'little')
            if kind!=0x120 and not count:continue
            assert before[:2]==after[:2] and before[size:size+4]==after[size:size+4],'output directory actual module/size input differs'
            expected.append(struct.pack('<HH',kind,module)+before[pointer:pointer+4]+before[size:size+4])
            actual.append(struct.pack('<HH',kind,module)+after[pointer:pointer+4]+after[size:size+4])
            roles.append((kind,module))
        info=int.from_bytes(before[44:48],'little')
    assert len(seen)==35 and len(expected)==58 and len(set(roles))==58,'output directory rebuilt row inventory differs'
    before_sort=left['bytes'];after_sort=bytes.fromhex(right['bytes'])
    assert len(before_sort)==len(after_sort)==840,'output directory complete initialized sort input missing'
    assert before_sort[:696]==b''.join(expected) and after_sort[:696]==b''.join(actual),'output directory rebuilt rows differ from actual live module inputs'
    assert before_sort[696:]==old['bytes'][696:] and after_sort[696:]==bytes.fromhex(new['bytes'])[696:],'output directory writer changed retained initialized tail'
    current=original['memory'][root];current_native=bytes.fromhex(native['memory'][str(other)])
    assert len(current)==len(current_native)==840,'output directory current initialized capacity incomplete'
    def permutation(payload,expected):
        assert len(payload)==696
        result=[]
        for offset in range(0,696,12):
            entry=payload[offset:offset+12];matches=[index for index,row in enumerate(expected) if entry==row]
            assert len(matches)==1,'output directory current row lacks actual live input owner'
            result.append(matches[0])
        assert len(set(result))==58,'output directory current rows duplicate/omit an input'
        return result
    permutation_old=permutation(current[:696],expected);permutation_new=permutation(current_native[:696],actual)
    assert permutation_old==permutation_new,'canonical/native qsort first-callback directory permutation differs'
    assert current[696:]==old['bytes'][696:] and current_native[696:]==bytes.fromhex(new['bytes'])[696:],'qsort changed inactive initialized input tail'
    complete=None
    if complete_sort:
        spec=importlib.util.spec_from_file_location('complete_sort_state',Path(__file__).with_name('complete-sort-state.py'))
        complete_state=importlib.util.module_from_spec(spec);spec.loader.exec_module(complete_state)
        complete=complete_state.validate(config,original,native,expected,actual,current,current_native,permutation_old)
    else:
        expected_permutation=list(range(58));expected_permutation[0],expected_permutation[29]=29,0
        assert permutation_old==expected_permutation,'unmodeled canonical qsort first-callback prefix'
        assert native['parameters']=={'first':dict(index=1,width=12),'second':dict(index=0,width=12)},'output directory first callback argument roles differ'
    return dict(complete_sort=complete,active_rows=58,element_bytes=12,active_initialized_bytes=696,historical_initialized_rows=70,complete_initialized_capacity_bytes=840,retained_initialized_tail_bytes=144,own_unwritten_spare_capacity_bytes=72,module_rows=35,symbol_rows=22,source_rows=1,canonical_callback_rva=0x2551e,actual_pre_first_callback_swapped_rows=[0,29],first_callback_argument_rows=[1,0],future_callbacks_unobserved=not complete_sort)
