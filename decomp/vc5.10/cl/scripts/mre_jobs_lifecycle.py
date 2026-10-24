"""Actual Jobs/Release/Close effects from the current authenticated original."""
import hashlib,json,re,struct
from binary import u32
from mre_file import file_info
from mre_jobs_controls import first_scan
sha=lambda raw:hashlib.sha256(raw).hexdigest()


def finish(directory,read,phases,contexts,objects,calls,copies,events,target,frame):
    for phase in phases:
        assert first_scan(read,phase)==1,'actual CL scan state changed'
    b,a=contexts['jobs-before'],contexts['jobs-after']
    stack=struct.unpack('<16I',read('observer/jobs-before-stack.bin'))
    assert stack[:2]==(0x403697,0)and read('observer/jobs-before-arguments.bin')==bytes(4)
    assert target.read(0x403692,5)==bytes.fromhex('e8c6040000')
    assert target.read(0x403b5d,49)==bytes.fromhex('8b4c240433d25633c085c9741b85d28d71f475048bc6eb02893285c98bd674048b09ebe533c9ebe185d274038322005ec3')
    assert u32(b,184)==0x403b5e and u32(a,184)==0x403698
    assert u32(a,196)==u32(b,196)+4 and u32(a,176)==0
    for offset in(156,160,164,180):assert u32(b,offset)==u32(a,offset)
    assert u32(b,180)==frame
    assert re.findall(r'^jobs phase=before entry=00403b5d return=00403697 event=(\d+)$',events,re.M)==[str(phases['jobs-before']['event'])]
    assert re.findall(r'^jobs phase=after entry=00403b5d return=00403697 eax=00000000 event=(\d+)$',events,re.M)==[str(phases['jobs-after']['event'])]
    for bp,ap,allowed in [('jobs-before','jobs-after',()),('mre-5-before','mre-5-after',()),('mre-6-before','mre-6-after',('open_descriptors',))]:
        for name in phases[bp]:
            if name not in('event','directory')+allowed:assert phases[bp][name]==phases[ap][name],'lifecycle changed '+bp+' '+name
        assert contexts[bp][28:140]==contexts[ap][28:140]
        for suffix in('static-iob.bin','mb-state.bin','ctype.bin','crt-environment.bin','os-environment.bin','pioinfo-pointers.bin'):
            assert read('observer/'+bp+'-'+suffix)==read('observer/'+ap+'-'+suffix),'changed actual provider '+suffix
    # Closure affects only the genuine descriptor3 record; every other lowio
    # entry and allocated table remains authenticated by phase_snapshot.
    before=read('observer/mre-6-before-lowio-0.bin');after=read('observer/mre-6-after-lowio-0.bin')
    assert before[:108]==after[:108]and before[144:]==after[144:]
    assert before[112]==0x11 and u32(after,108)==0xffffffff and after[112]==0
    assert before[113:144]==after[113:144]
    assert phases['mre-6-after']['open_descriptors']==phases['mre-6-before']['open_descriptors'][:3]
    phase='mre-6-after';post=phase+'-copy-after';prefix='observer/mre-files-idb-'+phase+'-'
    raw=read(prefix+'request.bin');assert len(raw)==32
    schema,event,tid,slot,handle,lo,hi,pid=struct.unpack('<8I',raw)
    assert schema==2 and event==phases[phase]['event']and tid==phases[phase]['thread']and pid==u32(read('streams-'+phase+'-teb.bin'),0x20)
    assert slot==u32(read('observer/'+phase+'-pioinfo-pointers.bin'),0)and handle==0xffffffff and lo==hi==0
    assert read(prefix+'ack.bin')==raw+phase.encode()+b'\0'
    lowio=read(prefix+'lowio.bin');assert lowio==read(prefix+'lowio-after.bin')==after[108:144]
    last_raw=read(prefix+'last-live.bin');assert last_raw==read('observer/mre-files-idb-mre-5-after-descriptor.bin')
    current_raw=read(prefix+'path.bin');assert current_raw==read(prefix+'path-after.bin')==read(prefix+'path-reopened.bin')
    current,last=file_info(current_raw),file_info(last_raw)
    assert current['bytes']==25600 and not current['attributes']&0x410
    for field in('volume','index_high','index_low'):assert current[field]==last[field]
    opened=struct.unpack('<6I',read(prefix+'path-open.bin'));assert opened[0]not in(0,0xffffffff)and opened[2]==1 and opened[4:]==(0,7)
    assert read(prefix+'context-before.bin')==read(prefix+'context-after.bin')==contexts[phase]==contexts[post]
    for field in phases[phase]:
        if field!='directory':assert phases[phase][field]==phases[post][field],'closed copy changed '+field
    cwd=read(prefix+'cwd.bin').decode('utf-16le');assert cwd.endswith('\0')and cwd[:-1].rstrip('\\')==phases[phase]['cwd'].rstrip('\\')
    dos=read(prefix+'dos-path.bin');assert dos==(phases[phase]['cwd'].rstrip('\\')+'\\case.idb\0').encode('ascii')
    host=json.loads(read(prefix+'host.json'));content=read(prefix+'contents.bin')
    assert host['schema']==2 and host['closed']is True and host['position']is None and host['granted_access']is None
    assert host['phase']==phase and host['event']==event and host['tid']==tid and host['debuggee_pid']==pid
    assert host['handle']==handle and host['descriptor']==3 and host['windows_information']==current and host['last_live_windows_information']==last
    assert host['path_identity_access']==0 and host['windows_data_read_sharing_observed']is False
    assert host['request_sha256']==sha(raw)and host['windows_path']==dos[:-1].decode('ascii')
    assert host['bytes']==len(content)==current['bytes']and host['sha256']==sha(content)
    assert host['posix_before']==host['posix_after']==host['posix_path_after']
    assert host['posix_before']['size']==len(content)and host['posix_before']['mode']&0o170000==0o100000
    assert host['fixed_drive']=={'link':'/root/.ai-tmp/vc1.0-capture/wine/dosdevices/s:','target':'/root/.ai-tmp/msvc2'}
    assert re.findall(r'^file-copy-closed phase='+phase+r' event=(\d+) tid=(\d+) all-threads-held=debug-event cursor=none$',events,re.M)==[(str(event),str(tid))]
    assert content==read('case.idb')and sha(content)!=copies[-1]['sha256'],'Close output not current complete file'
    assert len({c['sha256']for c in copies})==1
    copies.append({'phase':phase,'request_sha256':sha(raw),'windows_file_information':current,'last_live_windows_file_information':last,'posix_identity_and_metadata':host['posix_before'],'closed':True,'actual_granted_access':None,'position':None,'lowio_flags':0,'bytes':len(content),'sha256':sha(content),'owned_contents':prefix+'contents.bin','postcopy_context_and_target_state_unchanged':True,'identity_qualification':'Actual closed fd3, no cursor/data-sharing claim. Current Windows path FileId matches last authenticated live handle; fixed owned S: path bridges independently preserved POSIX path/open identity. No FileId/inode numerical equality.'})
    for index in(4,5):
        calls[index]['return_qualification']='Actual BOOL1 consumed by CL; all ordinary stdcall nonvolatiles and full112-byte x87/public state retained.'
    return {'jobs_call':{'symbol':'jobs_from_deferred','address':0x403b5d,'formal_arguments':[0],'entry_esp':u32(b,196),'caller_return':stack[0],'normal_return_esp':u32(a,196),'result':0,'nonvolatile_preserved':True,'before':phases['jobs-before'],'after':phases['jobs-after']},'first_dependency_scan':{p:first_scan(read,p)for p in phases},'close_commit':1,'release_keeps_live_descriptor':True,'close_closes_descriptor':True,'close_changes_file':True,'final_idb':{'bytes':len(content),'sha256':sha(content)},'normal_status':0,'later_opaque_object_reads':'None in actual remaining trace; object contents/destructor mechanics unobserved/unclaimed.','final_public_state':'Last actual observation is the preserved post-Close snapshot, not an invented exit snapshot.'}
