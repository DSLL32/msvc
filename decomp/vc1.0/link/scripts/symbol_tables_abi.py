"""Typed direct-table observations; raw snapshots remain in capture receipts."""
import struct
NAMES={0x26510:'print_external_table',0x2dcf0:'copy_defined_names'}
def words(raw):
    b=bytes.fromhex(raw);return list(struct.unpack('<'+'I'*(len(b)//4),b))
def table(t,original=False):
    if t is None:return None
    h=words(t['header']);q=words(t['hash_header']);entries=t['entries'];assert len(h)==10 and len(q)==11
    assert len(entries)==q[3] and q[9]==0
    assert q[8]==0x26220 if original else t['key_owner']
    # Rebase only owned pointer edges, not scalar fields or symbol bytes.
    pointers={0:None,t['address']+32:'undefined-head'}
    for e in entries:
        name=e['name'];pointers[e['external']]=('external',name);pointers[e['external']+28]=('undefined-link',name);pointers[e['node']]=('node',name)
    def edge(p):assert p in pointers,('unowned pointer',hex(p));return pointers[p]
    nodes=[]
    for e in entries:
        raw=bytes.fromhex(e['raw']);assert len(raw)==56
        nodes.append((e['bucket'],e['name'],edge(e['next']),raw[:24].hex(),edge(struct.unpack_from('<I',raw,24)[0]),edge(struct.unpack_from('<I',raw,28)[0]),raw[32:].hex()))
    s=bytes.fromhex(t['strings']);assert len(s)==h[2]
    # The 4-byte size prefix is reserved, never written/consumed by these bodies.
    # Preserve raw bytes separately and require same-image source immutability.
    return dict(header=[bool(h[0]),bool(h[1]),h[2],h[3],bool(h[4]),*h[5:8],edge(h[8]),edge(h[9])],hash=q[:8]+[True,q[9],bool(q[10])],strings_defined=s[4:].hex(),nodes=nodes)
def fp(a,b):
    assert a['fp_control']&65535==b['fp_control']==0x027f
    assert a['fp_tag']&65535==b['fp_tag']==0xffff
    sa=a['fp_status']&65535;sb=b['fp_status'];assert not (sb&~65535)
    # ONLY dead incoming C0: proof in analysis/fp-condition-scope.json.
    assert (sa&~0x0100)==(sb&~0x0100)==0
    assert b['fp_raw'].count('Empty')==8
    # All eight registers are empty: stale empty-register payload is not live.
    # Reserved FSAVE halves and producer/IP are retained, not claimed equal.
    return dict(original_status=sa,native_status=sb,qualified_bit=0x0100)
def compare(original,native,frontier=None):
    left=[e for e in original if e.get('target') in NAMES];right=native['events'];assert left and len(left)==len(right),(len(left),len(right))
    active={};qualified=[]
    for a,b in zip(left,right):
        event='frontier' if b['event']=='frontier' else a['event']
        assert (a['phase'],NAMES[a['target']],event)==(b['phase'],b['name'],b['event'])
        assert a['inputs_unchanged'] and b['inputs_unchanged']
        assert a['errors']==b['errors'] and a['short_name']==b['short_name'] and a['pending']==b['pending']
        for e in [a,b]:
            f=e['stdout_file'];assert len(f)==8 and len(bytes.fromhex(e['pending']))==f[0]-f[2]
        assert a['stdout_file'][1]==b['stdout_file'][1] and a['stdout_file'][3:]==b['stdout_file'][3:]
        qualified.append(fp(a,b))
        assert [table(t,True) for t in a['tables']]==[table(t) for t in b['tables']],('typed table',a['phase'],event)
        for e in [a,b]:
            g=bytearray.fromhex(e['guards']);assert len(g)==64
            for offset,index in [(32,0),(48,2)]:
                if e['tables'][index]:assert struct.unpack_from('<I',g,offset)[0]==e['tables'][index]['address'];g[offset:offset+4]=b'\xa5'*4
            assert g==b'\xa5'*64
        source_index=0 if a['phase'] in [3,4] else 1 if a['phase']==5 else 2 if a['phase'] in [7,8] else 3
        assert len(a['arguments'])==len(b['arguments'])==1
        if a['event']!='return':
            for e in [a,b]:assert e['arguments'][0]==e['tables'][source_index]['address']
            active[a['phase']]=(a,b)
        else:
            ea,eb=active.pop(a['phase'])
            for before,after in [(ea,a),(eb,b)]:
                assert after['esp']==before['esp']+4
                assert all(before[k]==after[k] for k in ['ebx','esi','edi','ebp'])
                assert before['tables'][source_index]==after['tables'][source_index],('mutated source',a['phase'])
                if a['target']==0x2dcf0:assert after['eax']==after['tables'][1 if a['phase']==4 else 3]['address']
    if frontier:
        assert right[-1]['event']=='frontier' and right[-1]['name']==frontier
    else:
        assert not active and len(left)==12 and native['terminal']==[dict(event='exit',code=0)]
    return dict(status='pass',observations=len(left),frontier=frontier,FP_qualified_C0=qualified,scope='One formal per root, full typed tables and owned graph edges, live stdout/errors/guards, per-side source immutability and cdecl return/nonvolatiles; only dead incoming C0 qualified by explicit def-use proof.')
