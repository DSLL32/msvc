"""Finite ten-row directive dispatch/data proof; no graph discovery."""
import re
import struct
from data_evidence import reader, cstring
from state import TARGET, sha


def verify(binary, config, graph):
    name='_output_callback_453e02'
    if name not in config['accepted']+config['trial']:return None
    record=next(r for r in config['driver']if r['name']==name)
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(binary/'build/c2.map').read_text(),re.M)}
    original,rebuilt=reader(TARGET),reader(binary/'build/c2.exe')
    assert struct.unpack('<10I',original(0x453e3c,40))==(0x474d5d,0x474d81,0x474d6f,0x453e16,0x474d81,0x474d67,0x474d52,0x474d81,0x474d40,0x453e28)
    assert record['shared']==[[0x453e3c,0x453e64]]
    base=maps[record['symbol']]
    assert record['rebuilt_dispatch_table_offsets']==[[194,234]]
    labels=[47,175,62,84,175,106,119,175,131,153]
    assert record['rebuilt_dispatch_labels']==labels
    assert struct.unpack('<10I',rebuilt(base+194,40))==tuple(base+n for n in labels),'directive jump-table content differs'
    def call(address, target):
        data=rebuilt(address,5)
        assert data[0]==0xe8 and address+5+struct.unpack_from('<i',data,1)[0]==maps[target],'directive named child differs'
    def literal(address, text):
        assert rebuilt(address,1)==b'\xb9'
        pointer=struct.unpack('<I',rebuilt(address+1,4))[0]
        assert cstring(rebuilt,pointer)==text,'directive literal differs'
    banner='Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00'
    literal(base+47,banner);call(base+52,'@emit_incremental_string@4')
    for offset,prefix,quoted,source_register in [(62,'-?comment:',1,'ecx'),(84,'-defaultlib:',0,'edx'),(131,'-nodefaultlib:',0,'ecx'),(153,'',0,'edx')]:
        assert rebuilt(base+offset,4)==bytes([0x6a,quoted,0x6a,0])
        load=b'\x8b\x4d\xfc\x8b\x51\x08'if source_register=='ecx'else b'\x8b\x55\xfc\x8b\x52\x08'
        assert rebuilt(base+offset+4,6)==load,'directive value must come from actual input payload+8'
        literal(base+offset+10,prefix);call(base+offset+15,'@write_coff_directive@16')
    assert rebuilt(base+106,6)==b'\x8b\x45\xfc\x8b\x48\x08'
    call(base+112,'@emit_incremental_string@4')
    assert rebuilt(base+119,10)==b'\xc7\x05'+struct.pack('<II',maps['_coff_directive_option'],1)
    assert original(0x497dac,4)==rebuilt(maps['_coff_directive_option'],4)==bytes(4)
    assert rebuilt(base+175,5)==b'\xba'+struct.pack('<I',1514)
    literal(base+180,'E:\\utc\\src\\\\P2\\coff.c');call(base+185,'@c2_report_internal@8')
    assert name in graph['functions'] and graph['functions'][name]['edges']==['@c2_report_internal@8','@emit_incremental_string@4','@write_coff_directive@16']
    return {'kind':'finite original/rebuilt switch-content, actual payload loads, named child, scalar and point-of-use literal certificate',
        'original_table':[0x453e3c,0x453e64],'original_table_sha256':sha(original(0x453e3c,40)),
        'rebuilt_table':[base+194,base+234],'rebuilt_table_sha256':sha(rebuilt(base+194,40)),
        'rebuilt_labels':labels,'source_sha256':sha((binary/'src/output-callbacks.c').read_bytes()),
        'rebuilt_sha256':sha((binary/'build/c2.exe').read_bytes()),'checker_sha256':sha(__import__('pathlib').Path(__file__).read_bytes()),
        'limits':'Only encountered ten dispatch rows and their actual code/data/ABI operands; pending child extents/body credit unchanged. Common Rust owns all CFG/RET/extent discovery.'}
