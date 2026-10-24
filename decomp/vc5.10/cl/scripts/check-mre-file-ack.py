#!/usr/bin/env python3
"""Focused atomic acknowledgement checks; no Wine or original execution."""
import ctypes,hashlib,json,shutil,struct,subprocess,time,uuid
from pathlib import Path
import mre_file

PROJECT=Path(__file__).resolve().parents[1]
directory=mre_file.CAPTURES/('mre-ack-controls-'+uuid.uuid4().hex)
directory.mkdir();(directory/'observer').mkdir()
phase='mre-1-after';base=directory/'observer'/('mre-files-idb-'+phase+'-')
path=lambda suffix:Path(str(base)+suffix)
passed=[];started=time.perf_counter();content=b'original intermediate file sample\0'*17


def seed():
    for file in(directory/'observer').iterdir():file.unlink()
    (directory/'case.idb').write_bytes(content)
    request=struct.pack('<8I',1,87,48,0x7803e000,116,1024,0,1588)
    path('request.bin').write_bytes(request)
    info=struct.pack('<13I',0x20,1,0,2,0,3,0,45,0,len(content),1,0,678)
    for suffix in('descriptor.bin','path.bin'):path(suffix).write_bytes(info)
    path('lowio.bin').write_bytes(struct.pack('<I',116)+b'\x11'+bytes(31))
    path('cwd.bin').write_bytes(('S:\\vc5.10\\cl\\'+directory.name+'\\\0').encode('utf-16le'))
    path('path-open.bin').write_bytes(struct.pack('<6I',120,0,1,0,0,7))
    path('object-basic.bin').write_bytes(struct.pack('<II',0,0x12019f)+bytes(48))
    receipt=mre_file.copy_stopped_file(directory,phase)
    return request,receipt


try:
    request,receipt=seed();expected=request+phase.encode()+b'\0'
    before=mre_file.metadata((directory/'case.idb').stat())
    ack=mre_file.publish_ack(directory,phase,receipt)
    assert path('ack.bin').read_bytes()==expected and ack['sha256']==hashlib.sha256(expected).hexdigest()
    assert not path('ack.pending').exists() and path('ack.bin').stat().st_nlink==1
    assert mre_file.metadata((directory/'case.idb').stat())==before
    for label,change in[
        ('existing acknowledgement',lambda:path('ack.bin').write_bytes(b'stale')),
        ('existing pending file',lambda:path('ack.pending').write_bytes(b'foreign')),
        ('changed copied contents',lambda:path('contents.bin').write_bytes(b'changed')),
        ('changed receipt',lambda:path('host.json').write_text('{}\n')),
        ('changed request',lambda:path('request.bin').write_bytes(struct.pack('<8I',1,88,48,0x7803e000,116,1024,0,1588)))]:
        request,receipt=seed();change()
        prior=path('ack.bin').read_bytes()if path('ack.bin').exists()else None
        pending=path('ack.pending').read_bytes()if path('ack.pending').exists()else None
        try:mre_file.publish_ack(directory,phase,receipt)
        except(AssertionError,OSError,KeyError):passed.append(label)
        else:raise AssertionError('published '+label)
        assert (path('ack.bin').read_bytes()if path('ack.bin').exists()else None)==prior
        assert (path('ack.pending').read_bytes()if path('ack.pending').exists()else None)==pending
    source=(PROJECT/'scripts/mre-file-observer.inc').read_text()
    a=source.index('static int mre_file_ack_payload(');b=source.index('static void mre_file_ack(',a)
    unit=directory/'payload.c';library=directory/'payload.so'
    unit.write_text('#include <stdint.h>\n#include <string.h>\ntypedef uint32_t DWORD;typedef unsigned char BYTE;\n'+source[a:b].replace('static int','int'))
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(unit),'-o',str(library)],check=True,capture_output=True)
    lib=ctypes.CDLL(str(library));check=lib.mre_file_ack_payload
    current=struct.pack('<8I',1,87,48,0x7803e000,116,1024,0,1588);expected=current+phase.encode()+b'\0'
    assert check(ctypes.create_string_buffer(expected),len(expected),ctypes.create_string_buffer(current),phase.encode())==1
    for i in range(len(expected)):
        value=bytearray(expected);value[i]^=1
        assert check(ctypes.create_string_buffer(bytes(value)),len(value),ctypes.create_string_buffer(current),phase.encode())==0
        passed.append('compiled payload byte '+str(i))
    for length in range(len(expected)):
        assert check(ctypes.create_string_buffer(expected),length,ctypes.create_string_buffer(current),phase.encode())==0
        passed.append('compiled truncated length '+str(length))
    assert check(ctypes.create_string_buffer(expected+b'x'),len(expected)+1,ctypes.create_string_buffer(current),phase.encode())==0;passed.append('compiled extra trailing data')
    assert check(ctypes.create_string_buffer(expected),len(expected),ctypes.create_string_buffer(current),b'mre-2-after')==0;passed.append('compiled different phase')
    for index in range(8):
        changed=bytearray(current);changed[index*4]^=1
        assert check(ctypes.create_string_buffer(expected),len(expected),ctypes.create_string_buffer(bytes(changed)),phase.encode())==0
        passed.append('compiled stale request field '+str(index))
    assert 'PeekNamedPipe'not in source and 'GetStdHandle(STD_INPUT_HANDLE)'not in source
    assert 'error!=ERROR_FILE_NOT_FOUND'in source and 'GetTickCount()-start<10000'in source
    report={'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed,
            'positive':'Real filesystem atomic exclusive hard-link publication after complete content+receipt; exact compiled32B request+phase; unchanged source metadata',
            'scope':'Acknowledgement transport only; no actual postcopy Windows preservation claim until required original capture.',
            'sources':{name:hashlib.sha256((PROJECT/'scripts'/name).read_bytes()).hexdigest()for name in('original.py','mre_file.py','mre-file-observer.inc')}}
    print(json.dumps(report,indent=2))
finally:shutil.rmtree(directory)
