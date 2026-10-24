#!/usr/bin/env python3
"""Affected raw-state/file/ack controls, refreshing transport hashes each time."""
import argparse,hashlib,json,re,shutil,struct,tempfile,time
from pathlib import Path
from mre_object_packet import decode,PROJECT

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);args=parser.parse_args()
started=time.perf_counter();passed=[];original_packet=decode(args.original)
with tempfile.TemporaryDirectory(prefix='mre-object-packet-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as scratch:
    directory=Path(scratch)/'raw';shutil.copytree(args.original,directory)
    original_capture=(directory/'capture.json').read_bytes();original_files={};capture=json.loads(original_capture)
    def write(name,raw):
        path=directory/name
        if name not in original_files:original_files[name]=path.read_bytes()
        path.write_bytes(raw);capture['files'][name]={'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
    def change_json(name,mutate):
        value=json.loads((directory/name).read_bytes());mutate(value);write(name,(json.dumps(value,indent=2)+'\n').encode())
    def word(name,offset,value):
        raw=bytearray((directory/name).read_bytes());struct.pack_into('<I',raw,offset,value);write(name,raw)
    def deny(label,mutate):
        try:
            mutate();(directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
            try:decode(directory)
            except(AssertionError,ValueError,KeyError,struct.error):passed.append(label)
            else:raise AssertionError('accepted '+label)
        finally:
            for name,raw in original_files.items():(directory/name).write_bytes(raw)
            original_files.clear();capture.clear();capture.update(json.loads(original_capture));(directory/'capture.json').write_bytes(original_capture)
    prefix='observer/mre-files-idb-mre-1-after-';host=prefix+'host.json'
    for label,mutate in[
        ('postcopy POSIX identity',lambda v:v['posix_after'].__setitem__('ino',v['posix_after']['ino']+1)),
        ('postcopy POSIX path timestamp',lambda v:v['posix_path_after'].__setitem__('mtime_ns',v['posix_path_after']['mtime_ns']+1)),
        ('host Windows identity',lambda v:v['windows_information'].__setitem__('index_low',v['windows_information']['index_low']+1)),
        ('host stale request SHA',lambda v:v.update(request_sha256='0'*64)),
        ('host access direction',lambda v:v.update(granted_access=1)),
        ('host PID',lambda v:v.update(debuggee_pid=v['debuggee_pid']+1)),
        ('invented Windows read sharing',lambda v:v.update(windows_data_read_sharing_observed=True)),
        ('host metadata access',lambda v:v.update(path_identity_access=0x80000000)),
        ('host content hash',lambda v:v.update(sha256='0'*64)),
        ('host content extent',lambda v:v.update(bytes=v['bytes']-1)),
        ('host phase',lambda v:v.update(phase='mre-2-after')),
        ('host fd position',lambda v:v.update(position=0)),
        ('unknown fixed drive',lambda v:v['fixed_drive'].update(target='/elsewhere'))]:
        deny(label,lambda mutate=mutate:change_json(host,mutate))
    for label,mutate in[
        ('ack stale event',lambda:word(prefix+'ack.bin',4,0)),
        ('ack wrong PID',lambda:word(prefix+'ack.bin',28,0)),
        ('ack wrong phase',lambda:write(prefix+'ack.bin',(directory/(prefix+'request.bin')).read_bytes()+b'mre-2-after\0')),
        ('ack truncated',lambda:write(prefix+'ack.bin',(directory/(prefix+'ack.bin')).read_bytes()[:-1])),
        ('ack trailing bytes',lambda:write(prefix+'ack.bin',(directory/(prefix+'ack.bin')).read_bytes()+b'x')),
        ('changed postcopy Windows identity',lambda:word(prefix+'path-reopened.bin',48,0)),
        ('changed postcopy fd position',lambda:word(prefix+'position-after.bin',0,0)),
        ('changed postcopy x87 status',lambda:word(prefix+'context-after.bin',32,1)),
        ('changed postcopy lowio flags',lambda:word(prefix+'lowio-after.bin',4,1)),
        ('missing actual read/write access',lambda:word(prefix+'object-basic.bin',4,1)),
        ('wrong metadata-only requested access',lambda:word(prefix+'path-open.bin',16,0x80000000)),
        ('sharing failure relabelled success',lambda:word(prefix+'path-open.bin',0,0xffffffff)),
        ('wrong source CWD',lambda:write(prefix+'cwd.bin','N:\\elsewhere\0'.encode('utf-16le'))),
        ('altered copied IDB',lambda:write(prefix+'contents.bin',b'x'+(directory/(prefix+'contents.bin')).read_bytes()[1:])),
        ('Open success with nonzero error out',lambda:word('observer/mre-1-after-error-out.bin',0,1)),
        ('wrong opaque driver offset',lambda:word('observer/mre-2-after-driver-out.bin',0,original_packet['opaque_roles']['engine']+8)),
        ('uninitialized PendingWork relabelled defined',lambda:word('observer/mre-frontier-pending.bin',0,1)),
        ('wrong signature formal',lambda:word('observer/mre-frontier-formal-arguments.bin',0,0)),
        ('changed shared scratch',lambda:write('observer/mre-frontier-shared-scratch.bin',b'x'+(directory/'observer/mre-frontier-shared-scratch.bin').read_bytes()[1:])),
        ('postcopy errno change',lambda:word('mktemp-mre-1-after-copy-after-ptd.bin',8,99)),
        ('unexpected frontend entry',lambda:write('trace.txt',(directory/'trace.txt').read_bytes()+b'execute_pass\n')),
        ('missing full postcopy snapshot',lambda:write('observer/mre-events.txt',re.sub(rb'^snapshot phase=mre-1-after-copy-after .*\n',b'',(directory/'observer/mre-events.txt').read_bytes(),flags=re.M))),
        ('analysis baseline',lambda:capture.update(baseline=True)),
        ('wrong case',lambda:capture.update(case='mre-prefix'))]:deny(label,mutate)
    def altered_formal(index,value):
        for phase in('mre-1-before','mre-1-after'):word('observer/'+phase+'-arguments.bin',index*4,value)
        word('observer/mre-1-before-stack.bin',index*4,value)
    deny('wrong hidden reproducible formal',lambda:altered_formal(4,1))
    deny('wrong shared error buffer alias',lambda:altered_formal(3,0x4091bc))
    def cleanup():
        name='observer/mre-1-after-context.bin';old=struct.unpack_from('<I',(directory/name).read_bytes(),196)[0];new=old+4;word(name,196,new)
        raw=(directory/'observer/mre-events.txt').read_bytes()
        for start in(b'snapshot phase=mre-1-after ',b'mre sequence=1 phase=after '):
            pattern=rb'^'+re.escape(start)+rb'.*$'
            match=re.search(pattern,raw,re.M);assert match
            changed=match[0].replace(('esp=%08x'%old).encode(),('esp=%08x'%new).encode())
            raw=raw[:match.start()]+changed+raw[match.end():]
        write('observer/mre-events.txt',raw)
    deny('actual stdcall cleanup differs despite consistent logs',cleanup)
    def changed_file0():
        for name in('observer/mre-1-after-copy-after-static-iob.bin','streams-mre-1-after-copy-after-iob.bin'):
            word(name,12,0x201)
    deny('postcopy FILE0 state with consistent raw mirrors',changed_file0)
    # The representation distinction is intentional, not an accidentally
    # unchecked equality: changing each POSIX inode consistently must not
    # require changing the independent Windows FileId.
    def independent_inode(value):
        for key in('posix_before','posix_after','posix_path_after'):value[key]['ino']+=1000000
    change_json(host,independent_inode);(directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
    independence=decode(directory)
    assert independence['intermediate_files'][0]['windows_file_information']==original_packet['intermediate_files'][0]['windows_file_information']
    assert independence['intermediate_files'][0]['posix_identity_and_metadata']['ino']!=original_packet['intermediate_files'][0]['posix_identity_and_metadata']['ino']
    report={'passed':True,'negative_controls':len(passed),'controls':passed,'seconds':time.perf_counter()-started,
            'positive_control':'Synthetic coherent POSIX inode representation change passes without altering Windows FileId; this is not evidence of an actual replacement',
            'capture_sha256':hashlib.sha256((args.original/'capture.json').read_bytes()).hexdigest(),
            'decoder_sha256':hashlib.sha256((PROJECT/'scripts/mre_object_packet.py').read_bytes()).hexdigest()}
    print(json.dumps(report,indent=2))
