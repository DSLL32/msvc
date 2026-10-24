"""Typed direct diagnostics observations; qualifications require explicit proof."""
import struct
NAMES={0x26b40:'main',0x35760:'add_default_libraries',0x35520:'print_module_debug_info'}
def words(raw):
    b=bytes.fromhex(raw);return list(struct.unpack('<'+'I'*(len(b)//4),b))
def modules(m,owners_required):
    assert m['count']==len(m['rows']) and m['count'] in [0,2]
    if not m['count']:assert m['owner']==0;return []
    libs={};out=[]
    for i,r in enumerate(m['rows']):
        w=words(r['raw']);assert len(w)==19 and r['address']==m['owner']+i*76
        names=r['names'];assert len(names)==3 and all(bytes.fromhex(s)[-1:]==b'\0' for s in names)
        if owners_required:
            edge=r['owner_edges'];assert w[0]==edge['module'] and w[1]==edge['library_name'] and w[2]==edge['path'] and edge['source_is_path']==int(w[3]==w[2])
            assert not edge['source_is_path'];libs.setdefault(edge['library'],len(libs));group=libs[edge['library']]
        else:group=0
        out.append((names,w[4:],group))
    return out

def libraries(l,input_base):
    if l is None:return None
    assert l['list']==l['image']+0x144;h=words(l['header']);rows=l['rows'];assert len(h)==4
    assert h[:3]==[0,l['list'],0] and h[3]==(rows[0]['address'] if rows else 0)
    out=[]
    for i,r in enumerate(rows):
        b=bytes.fromhex(r['raw']);assert len(b)==12;n,p=struct.unpack_from('<2I',b)
        assert n==(rows[i+1]['address'] if i+1<len(rows) else 0)
        assert not input_base<=p<input_base+256
        assert b[8]==1 and bytes.fromhex(r['name'])[-1:]==b'\0'
        # malloc-backed DefaultLibrary+9..11 has no writer/reader. Preserve
        # raw and within-image immutability, never compare/serialize padding.
        out.append((r['name'],b[8]))
    return out

def compare(original,native,frontier=None,qualified=False,owners_required=True):
    assert all(e['event'] in ['entry','return','frontier','before_crt','clock_boundary','debug_context_before_watch','last_error_watch_armed','continuation','summary','trap'] for e in original)
    assert all(e['event'] in ['entry','return','frontier','before_crt','last_error_watch_armed'] for e in native['events'])
    left=[e for e in original if e.get('target') in NAMES and e['event'] in ['entry','return','frontier'] or e['event']=='before_crt']
    right=[e for e in native['events'] if e['event'] in ['entry','return','frontier','before_crt']]
    assert left and len(left)==len(right),(len(left),len(right));active={};bases=[]
    for rows in [left,right]:bases.append(next(e for e in rows if e.get('phase')==2 and e['event']!='return')['arguments'][0]-32)
    main_states=[None,None];module_states=[None,None];list_states=[None,None]
    for a,b in zip(left,right):
        name='external_continuation' if a['event']=='before_crt' else NAMES[a['target']]
        assert (a['event'],a['phase'],name)==(b['event'],b['phase'],b['name'])
        assert a['errors'][:2]==b['errors'][:2]==[0,0]
        if a['phase']==0 and a['event']=='entry':assert a['errors'][2]==b['errors'][2]==0
        elif qualified:assert a['errors'][2]==1224 and b['errors'][2]==0
        else:assert a['errors'][2]==b['errors'][2]
        assert a['fp_control']&65535==b['fp_control']==0x027f and a['fp_tag']&65535==b['fp_tag']==0xffff
        sa=a['fp_status']&65535;sb=b['fp_status']
        if qualified:assert (sa&~0x100)==(sb&~0x100)==0
        else:assert sa==sb
        assert a['input']==b['input'] and a['pending']==b['pending']
        assert a['stdout_file'][1]==b['stdout_file'][1] and a['stdout_file'][3:]==b['stdout_file'][3:]
        for e in [a,b]:assert len(bytes.fromhex(e['pending']))==e['stdout_file'][0]-e['stdout_file'][2]
        assert modules(a['modules'],owners_required)==modules(b['modules'],owners_required)
        assert libraries(a['libraries'],bases[0])==libraries(b['libraries'],bases[1])
        for i,e in enumerate([a,b]):
            g=bytes.fromhex(e['guards']);assert len(g)==16 and g[4:]==b'\xa5'*12
            if e['libraries']:assert struct.unpack_from('<I',g)[0]==e['libraries']['image']
            else:assert g==b'\xa5'*16
            if e['phase']==0 and e['event']=='return':
                assert e['eax']==0;main_states[i]=e;module_states[i]=e['modules']
            if e['phase']>0:assert module_states[i]==e['modules'],('mutated/freed source modules',e['phase'])
            if e['phase']==2 and e['event']=='return':list_states[i]=e['libraries']
            if e['phase']>2:assert list_states[i]==e['libraries'],('mutated default list',e['phase'])
        if a['event']=='before_crt':
            assert not active
            for before,after in zip(main_states,[a,b]):
                assert before['errors']==after['errors']
                assert all((before[k]&65535)==(after[k]&65535) for k in ['fp_control','fp_status','fp_tag'])
            continue
        if a['event']!='return':
            expected=2 if name in ['main','add_default_libraries'] else 0
            assert len(a['arguments'])==len(b['arguments'])==expected
            if name=='main':
                assert a['arguments'][0]==b['arguments'][0]==3 and a['argv_names']==b['argv_names']
            elif name=='add_default_libraries':
                for e,base in zip([a,b],bases):assert e['arguments']==[base+(32 if e['phase']==2 else 160),e['libraries']['list']]
            active[a['phase']]=(a,b)
        else:
            ea,eb=active.pop(a['phase'])
            for before,after in [(ea,a),(eb,b)]:assert after['esp']==before['esp']+4 and all(before[k]==after[k] for k in ['ebx','esi','edi','ebp'])
    if frontier:assert right[-1]['event']=='frontier' and right[-1]['name']==frontier
    else:
        assert not active and len(right)==9 and native['terminal']==[dict(event='exit',code=0)]
        assert original[-1]==dict(event='summary',failed=0,stopped=1,exit=0,phase=6,stdout_bytes=203,stderr_bytes=0)
        previous=[e for e in original if e['event']=='debug_context_before_watch'];assert len(previous)==1 and not previous[0]['dr7']&255
        for events in [original,native['events']]:
            watches=[(i,e) for i,e in enumerate(events) if e['event']=='last_error_watch_armed'];assert len(watches)==1
            index,watch=watches[0];assert watch['address'] and watch['width']==4 and watch['access']=='readwrite'
            first_support=next(i for i,e in enumerate(events) if e.get('phase')==2 and e['event']=='entry');assert index<first_support
        ow=next(e for e in original if e['event']=='last_error_watch_armed');assert ow['dr7']==0xf0001 and ow['address']&4095==0x34
        assert any(e['event']=='continuation' and e['main_result']==0 and e['restored_eip']==0x50a85 and e['nonvolatiles_preserved'] for e in original)
        assert sum(e['event']=='last_error_watch_armed' for e in original)==sum(e['event']=='last_error_watch_armed' for e in native['events'])==1
        assert not any(e['event'] in ['last_error_access','last_error_entry'] for e in original+native['events'])
    return dict(status='pass',observations=len(right),frontier=frontier,qualification=qualified,owners_required=owners_required,scope='Actual main2formals/newparser2formals/newdiagnostic0formals+required hiddenglobals/typedowners, fullpayload/list/source immutability and savedmainreturn/CRTcontinuation. Qualified only reviewed incomingC0 and unconsumed post-mainLastError; errno/DOS/live state strict.')
