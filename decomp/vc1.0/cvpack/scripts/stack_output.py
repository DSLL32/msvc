"""Narrow typed caller-local identity for reviewed output-buffer callsites."""
import hashlib,re,json
from state import BINARY

def validate(item,pe):
    if item.get('register'):assert item['register']=='edx','unsupported typed output register'
    evidence=BINARY/item['evidence']
    assert hashlib.sha256(evidence.read_bytes()).hexdigest()==item['evidence_sha256'],'changed stack-output proof'
    proof=json.loads(evidence.read_text())
    assert 0<proof['size']<=1024 and proof['offset']==0
    for part in ('callsite','frame',*(['wrapper']if 'wrapper'in proof else []),*(['consumed_callee']if 'consumed_callee'in proof else [])):
        r=proof[part];assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'changed stack-output instruction evidence'
    if proof.get('location')=='allocation':
        a=proof['allocation']
        assert (proof['owner'],proof['local'],proof['size'])==('collect_strip_records','record',28),'unreviewed constructor allocation output'
        assert a==dict(size=28,callee=0x2cfb0,call=0x2844d,returned=0x28452,argument=0x2844b,carrier=0x28455),'unreviewed allocation birth/lifetime'
        assert pe.read(a['argument'],2)==b'\x6a\x1c'and pe.read(a['carrier'],2)==b'\x8b\xd8','allocation size/carrier def-use changed'
        assert pe.read(a['call'],1)==b'\xe8'and a['returned']+int.from_bytes(pe.read(a['call']+1,4),'little',signed=True)==a['callee'],'allocation callee changed'
        if proof['base_register']=='ebx':
            if proof['return_slots']==[dict(slot=0,address=0x285e6)]:
                assert proof['mode']=='input'and proof['defined_bytes']==28 and proof['children']=='coff-function','unreviewed publication record'
                assert [(r['start'],r['end'])for r in proof['child_sources']]==[(0x28990,0x289ea),(0x28b90,0x28cab)],'unreviewed child allocation sources'
                for r in proof['child_sources']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'changed child allocation/copy instructions'
            elif proof['return_slots']in([dict(slot=0,address=0x2848c)],[dict(slot=0,address=0x285be)]):
                assert proof.get('mode','output')=='output','unexpected constructor output mode'
            else:
                assert proof['return_slots']==[dict(slot=0,address=0x28521)]and proof['mode']=='input'and proof['consumed_ranges']==[[8,12],[24,26]],'unreviewed allocation consumer'
                assert proof['consumed_callee']['start']==0x28b90 and proof['consumed_callee']['end']==0x28cab,'unreviewed allocation consumed fields'
        else:
            assert proof['base_register']=='esi','unreviewed allocation carrier'
            contexts={0x28d16:(82,0x28d10,'input'),0x28d25:(83,0x28d1f,'output'),0x28d64:(82,0x28d5e,'output'),0x28d73:(82,0x28d6d,'output'),0x28d7d:(82,0x28d77,'output')}
            returned=proof['return_slots'][0]['address'];assert returned in contexts,'unreviewed forwarded allocation call'
            slot,push,mode=contexts[returned]
            assert proof['return_slots']==[dict(slot=0,address=returned),dict(slot=slot,address=0x2848c)]and proof['mode']==mode,'unreviewed forwarded allocation caller'
            assert pe.read(0x28cb9,7)==bytes.fromhex('8bb42440010000')and pe.read(push,1)==b'\x56','constructor allocation forwarding changed'
            if mode=='input':assert proof['defined_bytes']==28 and proof['initial_bytes']=='00'*28,'unproved constructor initialized state'
    else:
        assert proof.get('location','stack')=='stack','unknown typed output location'
        local_offset=proof['esp_offset']-proof['frame'].get('entry_bias',0)
        assert 0<=local_offset and proof['size']+local_offset<=proof['frame']['allocation'],'buffer exceeds reviewed stack frame'
    return proof

def compare(item,descriptions,words,parameters,events,pe):
    proof=validate(item,pe)
    for r in proof['return_slots']:assert words[r['slot']]==r['address'],'unknown stack-output caller'
    esp=int(re.search(r'^registers .* esp ([0-9a-f]+) ',events,re.M)[1],16)
    value=words[item['register']]if item.get('register')else words[item['slot']];actual=parameters[item['name']]
    if proof.get('location')=='allocation':assert value==words[proof['base_register']],'output pointer differs from original live allocation carrier'
    else:assert value==esp+proof['esp_offset'],'output pointer differs from original caller local'
    if 'size_argument'in item:
        count=next(r for r in descriptions if r['name']==item['size_argument'])
        assert words[count['slot']]==parameters[count['name']]==proof['size'],'output count differs from declared local'
    assert actual['address']==actual['local_address']+proof['offset'],'native raw output address differs from local base'
    local=dict(owner=proof['owner'],name=proof['local'],size=proof['size'],offset=proof['offset'])
    if proof.get('member'):local['member']=proof['member']
    assert not actual['null'] and actual['local']==local,'native output pointer differs from typed caller local'
    label='input'if proof.get('mode')=='input'else 'output'
    match=re.search(r'^%s-buffer %08x ([0-9a-f]+)$'%(label,value),events,re.M)
    assert match and len(bytes.fromhex(match[1]))==proof['size']
    assert len(bytes.fromhex(actual['prebytes']))==proof['size']
    if 'initial_bytes'in proof:
        assert match[1]==actual['prebytes']==proof['initial_bytes'],'initialized stack output local differs'
    defined=None
    if proof.get('mode')=='input':
        old=bytes.fromhex(match[1]);new=bytes.fromhex(actual['prebytes'])
        if proof.get('children')=='coff-function':
            left,right=bytearray(old),bytearray(new);children=actual.get('children',{});assert set(children)=={'0','16'},'missing COFF child observations'
            for field in (0,16):
                pointer=int.from_bytes(old[field:field+4],'little');native_pointer=int.from_bytes(new[field:field+4],'little')
                matches=re.findall(r'^coff-child %08x %d ([0-9a-f]+) (\d+) ([0-9a-f]*)$'%(value,field),events,re.M)
                assert len(matches)==1,'missing/duplicate original COFF child'
                address,size,payload=matches[0];native=children[str(field)];size=int(size)
                assert int(address,16)==pointer!=0 and native['address']==native_pointer!=0,'COFF child pointer/owner differs'
                assert len(bytes.fromhex(payload))==size<=1024 and payload==native['bytes'],'COFF child payload differs'
                if field:assert size==int.from_bytes(old[20:24],'little')*6,'COFF line allocation extent differs'
                else:assert payload.endswith('00')and b'\0'not in bytes.fromhex(payload)[:-1],'invalid COFF filename'
                left[field:field+4]=right[field:field+4]=b'\0'*4
            assert left==right,'defined COFF parent fields differ'
            defined=actual_length=proof['size']
        elif 'consumed_ranges'in proof:
            ranges=proof['consumed_ranges'];assert all(0<=a<b<=proof['size']for a,b in ranges),'consumed field exceeds allocation'
            assert all(old[a:b]==new[a:b]for a,b in ranges),'consumed allocation field differs'
            defined=actual_length=sum(b-a for a,b in ranges)
        elif proof.get('defined_cstring'):
            assert b'\0'in old and b'\0'in new,'unterminated typed string input'
            defined=old.index(0)+1;actual_length=new.index(0)+1
        elif 'defined_bytes'in proof:
            defined=actual_length=proof['defined_bytes'];assert defined==proof['size'],'fixed input definition exceeds typed local'
        else:
            r=proof['defined_length'];defined=int.from_bytes(old[r['offset']:r['offset']+r['width']],'little')+r['bias']
            actual_length=int.from_bytes(new[r['offset']:r['offset']+r['width']],'little')+r['bias']
        assert defined==actual_length and 1<=defined<=proof['size'],'invalid defined stack input length'
        if 'consumed_ranges'not in proof and not proof.get('children'):assert old[:defined]==new[:defined],'defined stack input bytes differ'
    return dict(defined_input_bytes=defined,original_address=value,native_address=actual['address'],typed_local=actual['local'],original_undefined_bytes=match[1],native_undefined_bytes=actual['prebytes'])
