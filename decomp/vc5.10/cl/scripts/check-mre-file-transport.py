#!/usr/bin/env python3
"""Pre-use MRE transport controls: Linux filesystem and compiled C guards only."""
import ctypes
import hashlib
import itertools
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time
import uuid
from contextlib import contextmanager

import mre_file
from original import mre_response_mode
from accept import reconstruction_capture

PROJECT=Path(__file__).resolve().parents[1]
passed=[];started=time.perf_counter()
directory=mre_file.CAPTURES/('mre-transport-controls-'+uuid.uuid4().hex)
directory.mkdir();(directory/'observer').mkdir()
phase='mre-1-after';base=directory/'observer'/('mre-files-idb-'+phase+'-')
file=lambda name:Path(str(base)+name)
content=bytes(range(256))*257


@contextmanager
def replaced(obj,name,value):
    previous=getattr(obj,name);setattr(obj,name,value)
    try:yield
    finally:setattr(obj,name,previous)


def seed():
    source=directory/'case.idb'
    if source.is_symlink():source.unlink()
    source.write_bytes(content)
    for suffix in ('contents.bin','host.json'):
        file(suffix).unlink(missing_ok=True)
    file('request.bin').write_bytes(struct.pack('<8I',1,82,4,0x7803e000,116,1024,0,20))
    info=struct.pack('<13I',0x20,1,0,2,0,3,0,45,0,len(content),1,0,678)
    for name in ('descriptor.bin','path.bin'):file(name).write_bytes(info)
    file('lowio.bin').write_bytes(struct.pack('<I',116)+b'\x11'+bytes(31))
    file('cwd.bin').write_bytes(('S:\\vc5.10\\cl\\'+directory.name+'\\\0').encode('utf-16le'))
    file('path-open.bin').write_bytes(struct.pack('<6I',120,0,1,0,0,7))
    file('object-basic.bin').write_bytes(struct.pack('<II',0,0x12019f)+bytes(48))


def deny(label,action):
    seed();action()
    try:mre_file.copy_stopped_file(directory,phase)
    except (AssertionError,OSError,UnicodeError):passed.append(label)
    else:raise AssertionError('transport accepted '+label)
    assert not file('contents.bin').exists(),'denied copy emitted valid bytes'


try:
    seed();before=mre_file.metadata((directory/'case.idb').stat())
    value=mre_file.copy_stopped_file(directory,phase)
    assert value['sha256']==hashlib.sha256(content).hexdigest()
    assert file('contents.bin').read_bytes()==content
    assert before==mre_file.metadata((directory/'case.idb').stat())
    for index,label in [(0,'schema'),(1,'event'),(2,'tid'),(3,'lowio address'),(4,'handle'),(7,'pid')]:
        def mutate(index=index):
            raw=bytearray(file('request.bin').read_bytes());struct.pack_into('<I',raw,index*4,0);file('request.bin').write_bytes(raw)
        deny('missing '+label,mutate)
    deny('64-bit position outside observed scope',lambda:file('request.bin').write_bytes(struct.pack('<8I',1,82,4,0x7803e000,116,1024,1,20)))
    deny('fd3/path different Windows identity',lambda:file('path.bin').write_bytes(bytes(52)))
    for opened in((0xffffffff,32,0,0,0,7),(0,0,1,0,0,7),(120,0,0,5,0,7),(120,0,1,0,0x80000000,7),(120,0,1,0,0,1),(120,0,1,0,0,0)):
        deny('wrong metadata-only path open '+repr(opened),lambda opened=opened:file('path-open.bin').write_bytes(struct.pack('<6I',*opened)))
    for attributes in(0x10,0x400):
        def mutate(attributes=attributes):
            raw=bytearray(file('descriptor.bin').read_bytes());struct.pack_into('<I',raw,0,attributes)
            for name in('descriptor.bin','path.bin'):file(name).write_bytes(raw)
        deny('directory/reparse '+str(attributes),mutate)
    for size in(0,len(content)+1,mre_file.MAX_BYTES+1):
        def mutate(size=size):
            raw=bytearray(file('descriptor.bin').read_bytes());struct.pack_into('<I',raw,36,size)
            for name in('descriptor.bin','path.bin'):file(name).write_bytes(raw)
        deny('wrong bounded size '+str(size),mutate)
    deny('wrong fd3 handle',lambda:file('lowio.bin').write_bytes(struct.pack('<I',117)+b'\x11'+bytes(31)))
    for flags in(1,0x91,0x10):
        deny('wrong binary/noinherit flags '+str(flags),lambda flags=flags:file('lowio.bin').write_bytes(struct.pack('<I',116)+bytes([flags])+bytes(31)))
    for cwd in('S:\\vc5.10\\cl\\elsewhere\0','N:\\vc5.10\\cl\\'+directory.name+'\0','S:\\vc5.10\\cl\\'+directory.name):
        deny('wrong or unterminated child CWD '+repr(cwd),lambda cwd=cwd:file('cwd.bin').write_bytes(cwd.encode('utf-16le')))
    for access in(0,1,2):
        deny('unproved read/write access '+str(access),lambda access=access:file('object-basic.bin').write_bytes(struct.pack('<II',0,access)+bytes(48)))
    def symlink():
        (directory/'case.idb').unlink();(directory/'case.idb').symlink_to(PROJECT/'cases/minimal-c/reference/test.obj')
    deny('symlink source',symlink)
    seed()
    with replaced(mre_file,'PREFIX',directory/'unknown-prefix'):
        try:mre_file.copy_stopped_file(directory,phase)
        except AssertionError:passed.append('unknown fixed S mapping')
        else:raise AssertionError('accepted unknown drive mapping')
    seed();real_open=mre_file.os.open;attempts=[]
    def noatime_denied(path,flags,*args,**kwargs):
        if flags&mre_file.os.O_NOATIME:attempts.append(flags);raise PermissionError('NOATIME denied control')
        return real_open(path,flags,*args,**kwargs)
    with replaced(mre_file.os,'open',noatime_denied):
        try:mre_file.copy_stopped_file(directory,phase)
        except PermissionError:passed.append('NOATIME denial never falls back')
        else:raise AssertionError('accepted denied NOATIME')
    assert len(attempts)==1 and not file('contents.bin').exists()
    seed();real_read=mre_file.os.read;changed=[False]
    def mutate_after_read(descriptor,count):
        raw=real_read(descriptor,count)
        if not changed[0]:
            changed[0]=True
            current=(directory/'case.idb').stat()
            mre_file.os.utime(directory/'case.idb',ns=(current.st_atime_ns,current.st_mtime_ns+1000000000))
        return raw
    with replaced(mre_file.os,'read',mutate_after_read):
        try:mre_file.copy_stopped_file(directory,phase)
        except AssertionError:passed.append('source metadata mutation during read')
        else:raise AssertionError('accepted source metadata mutation')
    assert not file('contents.bin').exists()
    config=json.loads((PROJECT/'functions.json').read_text())
    assert mre_response_mode(config,False,'mre-object',(False,)*7,True)=='CL_MRE_PREFIX_RESPONSE_V2\n'
    for baseline,case,others in[(True,'mre-object',(False,)*7),(False,'mre-prefix',(False,)*7)]+[(False,'mre-object',tuple(i==index for i in range(7)))for index in range(7)]:
        try:mre_response_mode(config,baseline,case,others,True)
        except AssertionError:passed.append('Python wrong/mixed response mode '+repr((baseline,case,others)))
        else:raise AssertionError('accepted mixed response mode')
    for name in('_prepare_jobs','_execute_pass'):
        bad={**config,'accepted':[n for n in config['accepted']if n!=name]}
        try:mre_response_mode(bad,False,'mre-object',(False,)*7,True)
        except AssertionError:passed.append('unaccepted caller '+name)
        else:raise AssertionError('accepted absent caller')
    for baseline in(False,True):
        try:reconstruction_capture({'baseline':baseline,'mre_response':True})
        except AssertionError:passed.append('native provider hold '+str(baseline))
        else:raise AssertionError('accepted unfinished MRE provider')
    observer=(PROJECT/'scripts/mre-observer.inc').read_text()
    guards=observer[observer.index('static int mre_observation_mode('):observer.index('static void mre_observation_install(')]
    transport=(PROJECT/'scripts/mre-file-observer.inc').read_text()
    guards+=transport[:transport.index('static void mre_file_ack(')]
    unit=directory/'guards.c';library=directory/'guards.so'
    unit.write_text('#include <stdint.h>\n#include <string.h>\ntypedef uint32_t DWORD;typedef unsigned char BYTE;\n'+guards.replace('static int','int'))
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(unit),'-o',str(library)],check=True,capture_output=True)
    lib=ctypes.CDLL(str(library));assert lib.mre_observation_response_mode(1,0)==1
    for args in((0,0),(0,1),(1,1),(2,0)):
        assert lib.mre_observation_response_mode(*args)==0;passed.append('compiled response mode '+repr(args))
    assert lib.mre_observation_response_marker(b'CL_MRE_PREFIX_RESPONSE_V2\n')==1
    for marker in(b'CL_MRE_PREFIX_RESPONSE_V1\n',b'CL_MRE_PREFIX_RESPONSE_V2',b'CL_MRE_PREFIX_ANALYSIS_V2\n'):
        assert lib.mre_observation_response_marker(marker)==0;passed.append('compiled metadata-only marker '+repr(marker))
    good=b'S:\\vc5.10\\cl\\mre-object-prefix-1';assert lib.mre_file_directory(good)==1
    for path in(b'N:\\vc5.10\\cl\\owned',b'S:\\vc5.10\\cl\\..\\elsewhere',b'S:\\vc5.10\\cl\\owned\\file',b'S:\\vc5.10\\cl\\',b'S:\\vc5.10\\cl\\UPPER',b'S:\\vc5.10\\cl\\' + b'a'*81):
        assert lib.mre_file_directory(path)==0;passed.append('compiled owned path '+repr(path))
    assert 'if (!mre_response_observer) {'in observer and 'if (!mre_response_observer && address==target_base+0x393a)'in observer
    assert 'if (after && mre_response_observer) mre_file_copy'in observer
    assert 'debug_event_pid!=debuggee_process_id'in transport and 'debug_event_tid!=tid'in transport
    assert 'ReadFile(duplicate'not in transport and 'ReadFile(independent'not in transport
    assert 'memcmp(&before_context,&after_context,sizeof(before_context))'in transport
    report={'passed':True,'fresh_negative_controls':len(passed),'controls':passed,'seconds':time.perf_counter()-started,
            'positive':'Complete bytes and unchanged POSIX identity/size/all timestamps under actual O_NOATIME|NOFOLLOW',
            'scope':'No Wine execution. Actual Windows fd/path/context/CRT preservation remains to be observed during the reviewed required masked capture.',
            'sources':{name:hashlib.sha256((PROJECT/'scripts'/name).read_bytes()).hexdigest()for name in('mre_file.py','mre-file-observer.inc','mre-observer.inc','original.py','accept.py')}}
    print(json.dumps(report,indent=2))
finally:shutil.rmtree(directory)
