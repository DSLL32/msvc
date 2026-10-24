"""Finite source-family generation roles, independent of later allocations."""


def source_family_indices(original,native):
    old=original['source_family_allocations'];new=native['source_family_allocations']
    assert len(old)==len(new)==3,'source family generation inventory differs'
    checked=native['checked_allocations']
    indices=[index for index,row in enumerate(checked) if row.get('owner') in ('build_source_file_index','append_source_name')]
    assert len(indices)==3 and indices==list(range(indices[0],indices[0]+3)),'source family checked events must be exactly three consecutive owners'
    for role,index in enumerate(indices):
        a=original['checked_allocations'][index];b=checked[index]
        expected_owner='append_source_name' if role==1 else 'build_source_file_index'
        assert b['owner']==expected_owner and b['role']==('module','name','output')[role] and b==new[role],'source family actual checked owner/role differs'
        assert a['address']==old[role]['address'] and a['size']==old[role]['size']==b['size'] and b['address']==new[role]['address'],'source family checked original generation differs'
        assert old[role]['after_frees']==b['after_frees']==12,'source family checked cleanup generation differs'
    return set(indices)
