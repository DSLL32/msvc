"""Focused ELF symbol/INT3-stop validation; no function graph discovery."""
import struct
import subprocess
from common import HERE


def functions():
    run=subprocess.run(['nm','-S','--defined-only',str(HERE/'build/native/rc')],stdout=subprocess.PIPE,check=True,text=True)
    result={}
    for line in run.stdout.splitlines():
        fields=line.split()
        if len(fields)==4 and fields[2] in ['T','t','W','w']:
            result[fields[3]]={'value':int(fields[0],16),'size':int(fields[1],16)}
    return result


def file_byte(address):
    data=(HERE/'build/native/rc').read_bytes()
    assert data[:6]==b'\x7fELF\x01\x01','Expected i386 little-endian ELF'
    offset=struct.unpack_from('<I',data,28)[0]
    size,count=struct.unpack_from('<HH',data,42)
    for n in range(count):
        kind,raw,va,_,length=struct.unpack_from('<IIIII',data,offset+n*size)
        if kind==1 and va<=address<va+length:return data[raw+address-va]
    raise AssertionError('PC outside file-backed ELF')


def validate(check,frontier,raw):
    records=functions()
    assert check and check['symbol'] in records,'Missing actual ELF stop symbol'
    row=records[check['symbol']]
    pc=check['pc'];start=check['symbol_start'];delta=pc-start
    assert row['size']>0,'Empty ELF function'
    expected=frontier.lstrip('_').split('@')[0]
    if frontier.startswith('import:'):
        import re
        assert re.search(r'Breakpoint [0-9]+,',raw),'Not an actual imported-boundary breakpoint'
        mapping={'import:_controlfp':'recon_controlfp','import:_initterm':'recon_initterm','import:RC':'RC'}
        assert check['kind']=='import-entry' and check['symbol']==mapping[frontier] and delta==0,'Wrong imported-boundary PC'
        assert check['file_value']==row['value'] and check['file_size']==row['size'],'Boundary ELF identity differs'
    else:
        assert check['kind']=='int3' and check['symbol']==expected,'Wrong named INT3 stub'
        assert check['signal']==5 and 'Program received signal SIGTRAP' in raw,'Not an actual SIGTRAP stop'
        assert check['file_value']==row['value'] and check['file_size']==row['size'],'Stub ELF identity differs'
        assert 0<delta<=row['size'] and check['offset']==delta-1,'PC not immediately after named stub INT3'
        assert check['opcode']=='cc' and file_byte(row['value']+delta-1)==0xcc,'PC is not an ELF INT3 byte'
