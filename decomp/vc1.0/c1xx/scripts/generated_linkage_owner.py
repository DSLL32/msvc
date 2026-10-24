"""Validate the reached, initialized default-linkage slot for generated storage.

The existing accepted parse_options calls push_linkage(DEFAULT) at 0x3cfaa;
its depth starts -1, becomes0 and initializes slot0. This receipt credits only
that actual depth0 path, not a hypothetical pre-options negative index.
"""


def validate(function, anchors, native):
    assert function=='_create_generated_object@8'
    depth, slot = anchors['linkage_depth'], anchors['current_linkage']
    for word in (depth, slot):
        assert word['bytes']==word['read_bytes']==4 and word['read_succeeded'], 'missing complete linkage owner read'
    assert depth['original']==0x9fc7c and depth['value']==native['observed_linkage_depth']==0, 'reached linkage depth is not initialized slot0'
    assert slot['original']==0x913a4+4*depth['value'], 'wrong actual original linkage slot'
    assert slot['value']==native['observed_linkage']==0, 'default linkage input differs'
    assert 0<native['linkage_base']<=0xffffffff-39, 'native linkage array span wraps'
    assert native['observed_linkage_slot']==native['linkage_base']+4*native['observed_linkage_depth'], 'wrong actual native linkage slot'
