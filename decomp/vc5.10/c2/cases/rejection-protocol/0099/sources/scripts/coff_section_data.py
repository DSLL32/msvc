"""Finite COFF creator switch rows and contextual flag/name operands, no CFG walk."""
import re
import struct
from pathlib import Path
from data_evidence import reader,cstring
from state import TARGET,sha


def verify(binary,config):
    name='@create_coff_section@4'
    if name not in config['accepted']+config['trial']:return None
    row=next(r for r in config['driver']if r['name']==name)
    symbols={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(binary/'build/c2.map').read_text(),re.M)}
    original,rebuilt=reader(TARGET),reader(binary/'build/c2.exe');base=symbols[row['symbol']]
    original_targets=[0x41ed24,0x41edbc,0x41ee3e,0x41ede0,0x41eea7,0x41eeb4,0x469943,0x41ee0a,0x41ee19,0x41ee57,0x41ee80,0x4699ac]
    classes=bytes([0,1,11,2,3,11,11,11,11,4,5,6,11,11,11,11,11,7,8,9,10])
    assert struct.unpack('<12I',original(0x41eef0,48))==tuple(original_targets)
    assert original(0x41ef20,21)==classes
    assert row['shared']==[[0x41eef0,0x41ef20],[0x41ef20,0x41ef35]]
    assert row['rebuilt_text_data_offsets']==[[1251,1299],[1299,1320]]
    labels=[321,436,507,578,649,670,691,762,781,821,889,975]
    assert row['rebuilt_dispatch_labels']==labels
    assert struct.unpack('<12I',rebuilt(base+1251,48))==tuple(base+n for n in labels),'COFF creator jump targets differ'
    assert rebuilt(base+1299,21)==classes,'COFF creator classmap differs'
    flags=[0x60000020,0xc0000040,0xc0000080,0x40000040,0x42000048,0xa00,0xc0000040,0x40000040,0x60000020,0x42000048,0x42000048]
    literals=[{'.text'},{'.data'},{'.bss'},{'.rdata'},set(),set(),{'.tls$'},{'.xdata$x'},{'.text$x'},{'.debug$S'},{'.debug$P','.debug$T'}]
    for i,(bits,texts)in enumerate(zip(flags,literals)):
        data=rebuilt(base+labels[i],labels[i+1]-labels[i])
        assert data.count(b'\xc7\x45\xfc'+struct.pack('<I',bits))==1,'COFF creator case flags differ'
        actual=set()
        for at in range(len(data)-6):
            if data[at:at+2]!=b'\xc7\x45':continue
            pointer=struct.unpack_from('<I',data,at+3)[0]
            try:actual.add(cstring(rebuilt,pointer))
            except (AssertionError,UnicodeError):pass
        assert actual==texts,'COFF creator case point-of-use names differ'
    assert rebuilt(base+649,9)==bytes.fromhex('8b4de88b51188955f4')
    assert rebuilt(base+670,9)==bytes.fromhex('8b45e88b4818894df4'),'directive name must come from actual section.symbol.name'
    assert rebuilt(base+975,6)==b'\x66\xa1'+struct.pack('<I',symbols['_coff_default_flags'])
    assert original(0x48fb60,4)==rebuilt(symbols['_function_option_23'],4)==struct.pack('<I',1),'encountered function-option bit23 storage differs'
    return {'scope':'Finite12 jump/21class rows,11 exact flagsets/point-of-use name operands and initialtypedscalar; common Rust owns all graph discovery','original_table_sha256':sha(original(0x41eef0,69)),'rebuilt_table_sha256':sha(rebuilt(base+1251,69)),'original_targets':original_targets,'rebuilt_labels':labels,'source_sha256':sha((binary/'src/coff-output.c').read_bytes()),'checker_sha256':sha(Path(__file__).read_bytes())}
