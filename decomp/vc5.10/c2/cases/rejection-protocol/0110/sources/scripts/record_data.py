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


def verify_globals(binary,config):
    name='_prepare_global_records'
    if name not in config['accepted']+config['trial']:return None
    from data_evidence import reader
    record=next(r for r in config['driver']if r['name']==name)
    path=binary/'cases/analysis/prepare-global-records-tables.json';value=json.loads(path.read_text())
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['source_sha256']==sha((binary/'src/code-records.c').read_bytes())
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(binary/'build/c2.map').read_text(),re.M)}
    assert record['shared']==[[0x41f9e0,0x41f9f4],[0x41f9f4,0x41fa04]]
    rows=[r for r in config['text_data']if r.get('owner')==name]
    assert [[r['start'],r['end']]for r in rows]==record['shared']
    assert record['rebuilt_text_data_offsets']==value['rebuilt_offsets']
    result={}
    for side,read,start,intervals in [('original',reader(TARGET),record['start'],record['shared']),('rebuilt',reader(binary/'build/c2.exe'),maps[name],[[maps[name]+a,maps[name]+z]for a,z in value['rebuilt_offsets']])]:
        assert [z-a for a,z in intervals]==[20,16]
        targets=list(struct.unpack('<5I',read(intervals[0][0],20)));classes=list(read(intervals[1][0],16))
        assert classes==value['classes']==[0,4,4,1,4,4,4,2,4,4,4,4,4,4,4,3],'global initializer classification content differs'
        assert [x-start for x in targets]==value[side]['targets_offsets'],'global initializer logical case target differs'
        assert targets[0]==targets[3]and targets[1]==targets[2]and len(set(targets))==3
        assert all(start<=x<intervals[0][0]for x in targets)
        if side=='rebuilt':assert read(intervals[0][0]-1,1)==b'\xc3'
        for address in ([0x497d98,0x497df0]if side=='original'else[maps['_global_initializers'],maps['_global_records_processing']]):assert read(address,4)==bytes(4),'initial global initializer head/processing flag differs'
        result[side]={'targets':targets,'classes':classes,'intervals':intervals,'sha256s':[sha(read(a,z-a))for a,z in intervals]}
    return {'evidence_sha256':sha(path.read_bytes()),'data_bytes':36,'sides':result}


def verify_initializers(binary,config):
    name='@write_codeview_records@4'
    if name not in config['accepted']+config['trial']:return None
    from data_evidence import reader
    path=binary/'cases/analysis/record-initializers-analysis.json';value=json.loads(path.read_text())
    assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['primary']==[0x41f567,0x41f58c]and value['bytes']==37
    original=reader(TARGET);assert sha(original(0x41f567,37))==value['sha256']
    assert original(0x4540b8,15).hex()==value['field']['writer_bytes']and original(0x4540bc,3)==b'\x89\x68\x32'
    assert value['guard']['original']==0x497cac and value['guard']['native']=='c2_output_name'and value['guard']['type']=='char*'
    assert value['field']['offset']==50 and value['field']['width']==4 and value['field']['type']=='C2InitRecord*'and value['field']['enclosing_size']==110
    assert value['child']['entry']==0x44a6e4 and value['child']['logical']=='@emit_initializer@4'and value['child']['already_indexed']is True
    rows={r['name']:r for r in config['driver']+config['stubs']};record=rows[name]
    assert record['symbol']=='@write_record_initializers@4'and record['native']=='write_record_initializers'
    child=rows['@emit_initializer@4'];assert child['start']==0x44a6e4 and child['native']=='emit_initializer'
    source=(binary/'src/code-records.c').read_text()
    exact='void __fastcall write_record_initializers(C2DebugLink *records)\n{\n    if (c2_output_name) {\n        for (; records; records = records->next)\n            emit_initializer(((C2InputSymbol *)records->symbol)->initializer);\n    }\n}'
    assert source.count(exact)==1,'initializer producer must use actual output-name guard, list/symbol owner and +50 pointer'
    header=(binary/'src/c2.h').read_text()
    assert 'C2InitRecord *initializer;\n    unsigned attributes;'in header
    assert 'offsetof(C2InputSymbol, initializer) == 50'in header and 'offsetof(C2FunctionSymbol, initializer) == 50'in header
    assert 'sizeof(C2InputSymbol) == 110'in header and 'sizeof(C2FunctionSymbol) == 110'in header
    assert 'sizeof(C2InitRecord) == 28'in header
    assert sha((binary/'cases/analysis/record-initializers-disassembly.txt').read_bytes())==value['disassembly_sha256']
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(binary/'build/c2.map').read_text(),re.M)}
    rebuilt=reader(binary/'build/c2.exe');base=maps[record['symbol']]
    assert rebuilt(base+7,7)==b'\x83\x3d'+struct.pack('<I',maps['_c2_output_name'])+b'\0'
    assert rebuilt(base+35,6)==b'\x8b\x42\x04\x8b\x48\x32','actual symbol+50 DWORD must be real incoming ECX initializer argument'
    call=base+41;assert rebuilt(call,5)==b'\xe8'+struct.pack('<i',maps['@emit_initializer@4']-call-5)
    assert rebuilt(base+51,1)==b'\xc3'
    return {'evidence_sha256':sha(path.read_bytes()),'field':value['field'],'child':'existing genuine emit_initializer(C2InitRecord*ECX), no new body credit','canonical_source_alias':record['symbol'],'scope':'Exact existing four-byte +50 initializer member/writer and emitted consumer; only currentNULL list skip observed. No pointee/default/initialized-data credit or new callback identity.'}
