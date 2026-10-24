"""Actual direct resize ABI/typed-state comparison, separate from fixtures."""
import json
from pathlib import Path
NAMES={0x3c340:'resize_file',0x3c380:'resize_mapped_file',0x3c3f0:'resize_buffered_file'}
REGISTERS=['ebx','esi','edi','ebp']
def compare(original,native,phase):
    left=[e for e in original if e.get('target') in NAMES];right=[e for e in native['events'] if e['event'] in ['entry','return']]
    assert len(left)==len(right)==(10 if phase=='mapped' else 12),'nonempty complete entry/return inventory required'
    active_left={};active_right={}
    page_owners=[]
    for rows in [left,right]:
        known={}
        for e in rows:
            for row in e.get("pages",[]):
                if row["address"] not in known:known[row["address"]]=len(known)
        page_owners.append(known)
    for a,b in zip(left,right):
        name=NAMES[a['target']];assert (a['event'],a['phase'],name)==(b['event'],b['phase'],b['name'])
        assert len(a['arguments'])==len(b['formal_words'])==2
        if name=='resize_file':assert a['arguments']==b['formal_words']
        else:
            assert a['arguments'][0]==a['slot_address'] and b['formal_words'][0]==b['slot_address']
            assert a['arguments'][1]==b['formal_words'][1]
        for e in [a,b]:assert len(e['slot'])==10
        fields=[1,2,3,7,8] if phase=='mapped' else [1,2,3,6,7]
        for i in fields:assert a['slot'][i]==b['slot'][i],('slot scalar',a['phase'],i)
        assert a['errors']==b['errors'] and a['read_buffer']==b['read_buffer']
        if phase=='buffered':
            for e,addresses in [(a,page_owners[0]),(b,page_owners[1])]:
                e['_page_edges']=[]
                for row in e.get('pages',[]):
                    assert row['tail'][0]-row['address']==24+row['head'][3]-row['head'][1]
                    e['_page_edges'].append([None if not p else addresses[p] for p in row['tail'][1:]])
            assert a['_page_edges']==b['_page_edges']
            assert [r['head'] for r in a.get('pages',[])]==[r['head'] for r in b.get('pages',[])]
            assert [v if v<=1 else 2 for v in a.get('page_table',[])]==[v if v<=1 else 2 for v in b.get('page_table',[])]
        if a['event']=='entry':active_left[name]=a;active_right[name]=b
        else:
            assert a['eax']==b['eax']
            for e,active in [(a,active_left),(b,active_right)]:
                entry=active.pop(name);assert e['esp']==entry['esp']+4 and all(e[r]==entry[r] for r in REGISTERS)
    assert not active_left and not active_right
    ao=[e for e in original if e['event'].startswith('api_')];an=[e for e in native['events'] if e['event'].startswith('api_')];assert len(ao)==len(an)==6
    calls=[0x5b188,0x5b18e,0x5b188] if phase=='mapped' else [0x53740]*3
    owners=['resize_mapped_file']*3 if phase=='mapped' else ['resize_buffered_file']*3
    api_names={0x5b188:'recon_SetFilePointer',0x5b18e:'recon_SetEndOfFile',0x53740:'recon_chsize'}
    callers=[0x3c3ba,0x3c3c3,0x3c3d6] if phase=='mapped' else [0x3c45e]*3
    for i,(a,b) in enumerate(zip(ao,an)):
        k=i//2;assert a['api']==calls[k] and b['name']==api_names[calls[k]] and a['event']==b['event']
        assert a['caller']==callers[k]
        assert a['arguments'][1:]==b['arguments'][1:]
        if phase=='buffered':assert a['arguments'][0]==b['arguments'][0]==3
        else:assert a['arguments'][0]==left[0]['slot'][4] and b['arguments'][0]==right[0]['slot'][4]
        assert a['errors']==b['errors']
        for key,value in [('fp_control',0x027f),('fp_status',0),('fp_tag',0xffff)]:assert a[key]&65535==b[key]==value
        # Original FSAVE retains reserved DWORD high halves/selectors. Check the
        # actual all-empty payload separately; do not call those halves equal.
        assert bytes.fromhex(a['fp_raw'])[28:108]==bytes(80)
        assert b['fp_raw'].count('0x00000000000000000000')==8 and b['fp_raw'].count('Empty')==8
        if a['event']=='api_entry':assert b['actual_caller']==owners[k]
        else:assert a['eax']==b['eax']
    assert native['terminal']==[dict(event='exit',code=0)]
    return dict(status='pass',application_entry_returns=len(left),public_boundary_entry_returns=len(ao),phase=phase,slot_owned_bytes=40,FP='Actual architectural CW027f/SW0/tagffff and all8empty zero payloads match; raw reserved transport fields retained')
def load(original,native):
    return [json.loads(x) for x in Path(original).read_text().splitlines()],json.loads(Path(native).read_text())
