"""The single reached static-FILE no-op branch; no general CRT identity claim."""
import json,re
from state import BINARY,sha

def validate(config,pe):
    policy=config.get('freebuf_boundary')
    if policy is None:return None
    expected=dict(case='hello-debug-dll',start=0x2ff90,end=0x2ffc6,return_site=0x2e155,iob=0x39390,slot=3,size=32,flag=0x80,phase=3,every_invocation=True,expected_calls=1,body_sha256='e325a15bb828e50fe14829b574ed07638647d03318107c2b2187d38b238f6047',evidence='cases/runtime-variants/freebuf.json',classification='case-specific-guarded-CRT-no-op',application_coverage=False,unconditionally_unmask=False)
    assert policy==expected,'unapproved freebuf no-op scope'
    import hashlib
    assert hashlib.sha256(pe.read(policy['start'],54)).hexdigest()==policy['body_sha256'],'freebuf original body changed'
    proof=json.loads((BINARY/policy['evidence']).read_text())
    assert proof['original_bytes_sha256']==policy['body_sha256']and proof['original_extent']==[policy['start'],policy['end']]
    runtime=json.loads((BINARY/'runtime.json').read_text())
    assert runtime['global_bindings']['__iob']==[policy['iob']]
    assert not any(r['start']==policy['start']for r in config.get('support_unmasked',[])),'guarded freebuf cannot become unconditional support'
    return policy

def compare(events,native,policy):
    rows=[r for r in events.splitlines()if r.startswith('freebuf-')]
    assert len(rows)==4 and len(native)==policy['expected_calls']==1,'missing/extra freebuf observations'
    assert rows[1]=='freebuf-restored 0002ff90 0002ffc6'and rows[2]=='freebuf-rearmed 0002ff90 0002ff91','missing every-entry restore/rearm'
    def parse(line,stage):
        m=re.fullmatch(r'freebuf-'+stage+r' thread (\d+) esp ([0-9a-f]+) file ([0-9a-f]{64}) errno (\d+) doserrno (\d+) last_error (\d+) handle ([0-9a-f]+) osfile (\d+) fp ([0-9a-f]+)',line)
        assert m,'invalid freebuf observation'
        raw=bytes.fromhex(m[3]);words=[int.from_bytes(raw[i:i+4],'little')for i in range(0,32,4)]
        assert words[:4]==[0,0,0,128]and words[5:]==[0,0,0]and 0<=words[4]<64,'unsupported FILE state'
        assert int(m[8])==1 and len(bytes.fromhex(m[9]))==112,'missing descriptor/x87 state'
        return dict(thread=int(m[1]),esp=int(m[2],16),file=m[3],errno=int(m[4]),doserrno=int(m[5]),last_error=int(m[6]),handle=int(m[7],16),osfile=int(m[8]),fp=m[9])
    old_entry,old_return=parse(rows[0],'entry'),parse(rows[3],'return')
    assert old_entry['thread']==old_return['thread']and old_return['esp']==old_entry['esp']+4,'freebuf invocation/frame mismatch'
    assert all(old_entry[k]==old_return[k]for k in('file','errno','doserrno','last_error','handle','osfile','fp')),'original no-op state changed'
    new=native[0];assert not new.get('error')and new['owner']=='recon_iob[3]'and new['caller']=='recon_fclose'and new['outer_caller']=='update_image_checksum','native FILE/caller identity differs'
    assert new['thread']==new['return_thread']and new['entry']==new['return'],'native no-op state changed'
    assert new['entry']['file']==old_entry['file']and new['entry']['osfile']==1,'native full FILE state differs'
    assert all(new['entry'][k]==old_entry[k]for k in('errno','doserrno','last_error')),'freebuf error state differs'
    assert set(new['entry']['fp'])=={'st'+str(i)for i in range(8)}|{'fctrl','fstat','ftag','fiseg','fioff','foseg','fooff','fop'},'native x87 observation incomplete'
    return dict(original=dict(entry=old_entry,returned=old_return),native=new,scope='Only actual static slot3 flag80 with no owned buffer, normal fclose caller; both full FILE/private descriptor/error/x87 states unchanged. No CRT application credit, no cross-platform raw x87 baseline equality or void EAX claim.')
