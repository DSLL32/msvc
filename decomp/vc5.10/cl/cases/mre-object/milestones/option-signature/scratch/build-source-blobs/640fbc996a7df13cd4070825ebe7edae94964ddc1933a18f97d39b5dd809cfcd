"""Bounded original-only IDB read while its debug event suspends all threads.

Win32 handles are compared to each other. POSIX identities are compared to
themselves; there is deliberately no Windows FileId == POSIX inode claim.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import struct
import time

SCRATCH=Path('/root/.ai-tmp/msvc2')
CAPTURES=SCRATCH/'vc5.10/cl'
PREFIX=Path('/root/.ai-tmp/vc1.0-capture/wine')
MAX_BYTES=16*1024*1024


def metadata(value):
    return {key:getattr(value,'st_'+key) for key in
            ('dev','ino','mode','nlink','uid','gid','size','atime_ns','mtime_ns','ctime_ns')}


def file_info(raw):
    assert len(raw)==52,'wrong Win32 file information size'
    value=struct.unpack('<13I',raw)
    return {'attributes':value[0],'creation_time':value[1]+(value[2]<<32),
            'access_time':value[3]+(value[4]<<32),'write_time':value[5]+(value[6]<<32),
            'volume':value[7],'bytes':value[9]+(value[8]<<32),'links':value[10],
            'index_high':value[11],'index_low':value[12]}


def request(directory,phase):
    directory=Path(directory).absolute()
    assert re.fullmatch('mre-[123]-after',phase),'unknown MRE copy phase'
    assert directory.parent==CAPTURES and re.fullmatch('[a-z0-9-]{1,80}',directory.name),'unowned capture directory'
    assert directory.resolve(strict=True)==directory,'capture path traverses symlinks'
    drive=PREFIX/'dosdevices/s:'
    assert drive.is_symlink() and drive.resolve(strict=True)==SCRATCH.resolve(strict=True),'fixed S: mapping changed'
    base=directory/'observer'/('mre-files-idb-'+phase+'-')
    read=lambda name:Path(str(base)+name).read_bytes()
    raw=read('request.bin');assert len(raw)==32
    schema,event,tid,slot,handle,low,high,pid=struct.unpack('<8I',raw)
    assert schema==1 and event and tid and slot and handle and pid and high==0,'invalid stopped-event request'
    descriptor_raw=read('descriptor.bin');path_raw=read('path.bin')
    assert descriptor_raw==path_raw,'independent Windows path and fd3 identities differ'
    info=file_info(descriptor_raw)
    assert info['bytes'] and info['bytes']<=MAX_BYTES and not info['attributes']&0x410,'directory/reparse/oversized IDB'
    lowio=read('lowio.bin');assert len(lowio)==36 and struct.unpack_from('<I',lowio)[0]==handle and lowio[4]==0x11,'wrong fd3 handle/binary/noinherit state'
    cwd=read('cwd.bin').decode('utf-16le')
    expected='S:\\vc5.10\\cl\\'+directory.name
    assert cwd.endswith('\0') and '\0'not in cwd[:-1] and cwd[:-1].rstrip('\\').lower()==expected.lower(),'child CWD does not select fixed owned path'
    opened=struct.unpack('<6I',read('path-open.bin'))
    assert opened[0]not in(0,0xffffffff)and opened[2]==1 and opened[4:]==(0,7),'path identity was not a successful metadata-only open'
    obj=read('object-basic.bin');assert 8<=len(obj)<=128
    granted_access=struct.unpack_from('<I',obj,4)[0]
    # FILE_READ_DATA and FILE_WRITE_DATA prove direction independently of
    # lowio0x11. Metadata-only path observation does not prove data sharing.
    assert granted_access&3==3,'actual IDB handle is not read/write'
    return {'schema':1,'phase':phase,'event':event,'tid':tid,'debuggee_pid':pid,
            'stop':'outstanding EXCEPTION_DEBUG_EVENT before ContinueDebugEvent; all threads suspended',
            'windows_path':expected+'\\case.idb','descriptor':3,'handle':handle,
            'position':low,'granted_access':granted_access,'windows_information':info,
            'path_identity_access':0,'windows_data_read_sharing_observed':False,
            'request_sha256':hashlib.sha256(raw).hexdigest(),
            'fixed_drive':{'link':str(drive),'target':str(drive.resolve())}}


def copy_stopped_file(directory,phase):
    started=time.monotonic();directory=Path(directory).absolute()
    receipt=request(directory,phase)
    root=os.open(directory,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW)
    try:
        before_path=os.stat('case.idb',dir_fd=root,follow_symlinks=False)
        assert stat.S_ISREG(before_path.st_mode),'IDB is not a regular file'
        descriptor=os.open('case.idb',os.O_RDONLY|os.O_NOFOLLOW|os.O_NOATIME,dir_fd=root)
        try:
            before=os.fstat(descriptor)
            assert metadata(before)==metadata(before_path),'path changed before independent open'
            assert before.st_size==receipt['windows_information']['bytes']<=MAX_BYTES,'Win32/POSIX sizes differ'
            parts=[];remaining=before.st_size
            while remaining:
                assert time.monotonic()-started<8,'bounded IDB copy exceeded deadline'
                value=os.read(descriptor,min(65536,remaining));assert value,'IDB truncated during copy'
                parts.append(value);remaining-=len(value)
            assert os.read(descriptor,1)==b'','IDB grew during copy'
            after=os.fstat(descriptor);after_path=os.stat('case.idb',dir_fd=root,follow_symlinks=False)
            assert metadata(before)==metadata(after)==metadata(after_path),'IDB identity/size/times changed during NOATIME copy'
        finally:os.close(descriptor)
    finally:os.close(root)
    raw=b''.join(parts);base=directory/'observer'/('mre-files-idb-'+phase+'-')
    destination=Path(str(base)+'contents.bin')
    with destination.open('xb')as file:file.write(raw)
    receipt.update(posix_before=metadata(before),posix_after=metadata(after),
                   posix_path_after=metadata(after_path),bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),
                   identity_bridge='fixed owned S: path; fd3 duplicate and independent Win32 path handle match; independent POSIX path/open match',
                   seconds=time.monotonic()-started)
    Path(str(base)+'host.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt


def publish_ack(directory,phase,receipt):
    """Atomically publish one exact request+phase acknowledgement, never replace.

    The helper checks these original32 request bytes and the phase, rather than
    introducing a hashing implementation into the original-only debugger.
    """
    directory=Path(directory).absolute()
    current=request(directory,phase)
    assert all(receipt[key]==value for key,value in current.items()),'ack request differs from copied request'
    base=directory/'observer'/('mre-files-idb-'+phase+'-')
    path=lambda suffix:Path(str(base)+suffix)
    assert json.loads(path('host.json').read_text())==receipt,'host copy receipt changed before acknowledgement'
    raw=path('contents.bin').read_bytes()
    assert len(raw)==receipt['bytes'] and hashlib.sha256(raw).hexdigest()==receipt['sha256'],'copied content changed before acknowledgement'
    request_bytes=path('request.bin').read_bytes()
    assert hashlib.sha256(request_bytes).hexdigest()==receipt['request_sha256']
    payload=request_bytes+phase.encode('ascii')+b'\0'
    destination=path('ack.bin');temporary=path('ack.pending')
    assert not destination.exists() and not destination.is_symlink(),'ack already exists'
    descriptor=os.open(temporary,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600)
    try:
        with os.fdopen(descriptor,'wb')as file:
            assert file.write(payload)==len(payload)
            file.flush();os.fsync(file.fileno())
        # Hard-link publication is atomic and fails if an acknowledgement
        # already exists. rename() could overwrite a competing/stale file.
        os.link(temporary,destination,follow_symlinks=False)
    finally:temporary.unlink()
    assert destination.read_bytes()==payload
    return {'bytes':len(payload),'sha256':hashlib.sha256(payload).hexdigest()}
