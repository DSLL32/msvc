"""Finite encountered record/expression dispatch content, independent of CFG."""
import json
import re
import struct
from state import TARGET, sha


def verify(binary,config):
    name='@prepare_function_records@4'
    if name not in config['accepted']+config['trial']:return None
    from data_evidence import reader
    record=next(r for r in config['driver']if r['name']==name)
    path=binary/'cases/analysis/prepare-records-tables.json';value=json.loads(path.read_text())
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['source_sha256']==sha((binary/'src/code-records.c').read_bytes())
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(binary/'build/c2.map').read_text(),re.M)}
    assert record['shared']==[[0x41f7a0,0x41f7b0],[0x41f7b0,0x41f7bf],[0x41f7c0,0x41f7d4]]
    rows=[r for r in config['text_data']if r.get('owner')==name]
    assert [[r['start'],r['end']]for r in rows]==record['shared']
    assert record['rebuilt_record_dispatch_offsets']==value['rebuilt_offsets']
    results={}
    for side,read,start,end,intervals in [('original',reader(TARGET),record['start'],record['end'],record['shared']),('rebuilt',reader(binary/'build/c2.exe'),maps[name],maps[name]+value['rebuilt_code_end_offset'],[[maps[name]+a,maps[name]+z]for a,z in value['rebuilt_offsets']])]:
        assert [z-a for a,z in intervals]==[16,15,20]
        targets=list(struct.unpack('<4I',read(intervals[0][0],16)))
        classes=list(read(intervals[1][0],15));nodes=list(struct.unpack('<5I',read(intervals[2][0],20)))
        assert classes==[0]*10+[3,1,3,3,2],'record classmap content differs'
        assert len(set(targets))==4 and all(start<=x<end for x in targets+nodes),'dispatch target outside actual source body'
        assert [x-start for x in targets]==value[side]['record_target_offsets'],'record logical case target differs'
        assert [x-start for x in nodes]==value[side]['expression_target_offsets'],'expression logical case target differs'
        assert nodes[0]==nodes[1]and nodes[3]==nodes[4]and len(set(nodes))==3
        if side=='rebuilt':assert read(intervals[0][0]-1,1)==b'\xc3','rebuilt dispatch must follow actual RET0'
        results[side]={'record_targets':targets,'classmap':classes,'expression_targets':nodes,'intervals':intervals,'sha256s':[sha(read(a,z-a))for a,z in intervals]}
    return {'scope':'Exact4recordtargets/15classbytes/5expressiontargets, normalized logical case positions and explicit textdata ownership;51B data separate from503B originalcode. Common Rust independently verifies all graph/extent/instruction ownership.','evidence_sha256':sha(path.read_bytes()),'sides':results}
