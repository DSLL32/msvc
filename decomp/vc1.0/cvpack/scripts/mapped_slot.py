"""Active Win32 HANDLE in the concretely reached mapped input slot."""
import re

def compare(pointer,old,new,actual,events,native,mapped_seek=False):
    assert len(old)==len(new)==40 and old[4:16]==new[4:16]and old[28:36]==new[28:36],'mapped slot scalar inputs differ'
    assert old[32:40]==new[32:40]==bytes(8),'initial cursor/next not NULL'
    if mapped_seek:
        assert all(int.from_bytes(raw[i:i+4],'little')for raw in(old,new)for i in(20,24)),'mapping/view successful result missing'
    else:assert old[20:28]==new[20:28]==bytes(8),'mapping/view not yet proved NULL'
    assert int.from_bytes(old[4:8],'little')==(11 if mapped_seek else 3),'unexpected mapped-input discriminator'
    requests=re.findall(r'^mapped-open-request thread (\d+) slot ([0-9a-f]+) esp ([0-9a-f]+) args ((?:[0-9a-f]+ ?){7}) name ([0-9a-f]+)$',events,re.M)
    returns=re.findall(r'^mapped-open-return thread (\d+) slot ([0-9a-f]+) handle ([0-9a-f]+)$',events,re.M)
    sources=re.findall(r'^mapped-slot-source slot ([0-9a-f]+) handle ([0-9a-f]+) type (\d+) size (\d+) position (\d+) after (\d+) volume (\d+) index ([0-9a-f]+) attrs (\d+)$',events,re.M)
    assert len(requests)==len(returns)==len(sources)==len(native)==1,'missing/extra actual mapped-input ownership proof'
    request,returned,source=requests[0],returns[0],sources[0];args=[int(v,16)for v in request[3].split()]
    assert int(request[1],16)==int(returned[1],16)==int(source[0],16)==pointer and request[0]==returned[0],'mapped-input slot/invocation differs'
    assert int.from_bytes(old[:4],'little')==args[0]and args[1:]==[0xc0000000,0,0,4,128,0],'actual original mapped open differs'
    assert bytes.fromhex(request[4]).decode('latin1')=='embedded-cpp.dll'
    assert int(returned[2],16)==int(source[1],16)==int.from_bytes(old[16:20],'little')!=0,'slot source is not actual CreateFile result'
    assert int(source[2])==1 and int(source[3])==int.from_bytes(old[12:16],'little')==int.from_bytes(old[28:32],'little')==2097152 and int(source[4])==int(source[5])==0,'original live file state differs'
    call=native[0];assert not call.get('error')and call['slot']==actual['address'],'native mapped input slot differs'
    assert {k:call[k]for k in('name','access','share','security','disposition','attrs','template')}==dict(name='embedded-cpp.dll',access=0xc0000000,share=0,security=0,disposition=4,attrs=128,template=0),'actual native mapped open differs'
    state=call['return'];assert state['handle']==int.from_bytes(new[16:20],'little')!=0 and state['type']==1 and state['size']==2097152 and state['position']==0 and state['access_mode']==2 and state['owned'],'native live source HANDLE identity differs'
    return dict(original_request=request,original_return=returned,original_live_handle=source,native=call,scope='Actual CreateFileA request/result carried to published FileSlot.source; live original/native disk/size/position identity. Raw mapping/view results retained without contents or cross-image pointer equivalence; actual creation checked separately.'if mapped_seek else 'Actual CreateFileA existing-input request/result carried to published FileSlot.source; live duplicated original disk/size/position identity and native host-fd identity. Mapping/view pointers remain NULL, not normalized. Full current input artifacts compared independently.')


def compare_creation(pointer,old,new,actual,events,native):
    requests=re.findall(r'^mapped-api-request stage (\d+) thread (\d+) slot ([0-9a-f]+) esp ([0-9a-f]+) callback ([0-9a-f]+) args ([0-9a-f ]+)$',events,re.M)
    returns=re.findall(r'^mapped-api-return stage (\d+) thread (\d+) slot ([0-9a-f]+) esp ([0-9a-f]+) value ([0-9a-f]+)$',events,re.M)
    assert len(requests)==len(returns)==len(native)==2,'missing actual mapping request/return'
    proof=[]
    def word(raw,offset):return int.from_bytes(raw[offset:offset+4],'little')
    for index,(request,returned,observed)in enumerate(zip(requests,returns,native),1):
        assert int(request[0])==int(returned[0])==observed['stage']==index and request[1]==returned[1],'mapping invocation differs'
        assert int(request[2],16)==int(returned[2],16)==pointer and observed['slot']==actual['address'],'mapping owner differs'
        assert int(returned[3],16)==int(request[3],16)+(24 if index==1 else 20),'mapping callback ABI/frame differs'
        assert int(request[4],16)!=0,'missing actual mapping callback'
        args=[int(x,16)for x in request[5].split()]
        expected=[word(old,16),0,4,0,word(old,28),0]if index==1 else [word(old,20),0xf001f,0,0,0]
        assert args==expected and int(returned[4],16)==word(old,20 if index==1 else 24)!=0,'original mapping request/result differs'
        expected_native=dict(file=word(new,16),security=0,protection=4,high=0,low=word(new,28),name=0)if index==1 else dict(handle=word(new,20),access=0xf001f,high=0,low=0,bytes=0)
        assert observed['arguments']==expected_native and observed['value']==word(new,20 if index==1 else 24)!=0,'native mapping request/result differs'
        proof.append(dict(original_request=request,original_return=returned,native=observed))
    return dict(calls=proof,scope='Actual creation requests/results bound to the source slot. Non-NULL results prove neither view contents nor cross-image pointer equivalence; view is not dereferenced by the next seek function. Real extent/bytes/ownership required before later consumption.')
