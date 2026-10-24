"""Active Win32 HANDLE in the concretely reached mapped input slot."""
import re

def compare(pointer,old,new,actual,events,native):
    assert len(old)==len(new)==40 and old[4:16]==new[4:16]and old[28:36]==new[28:36],'mapped slot scalar inputs differ'
    assert old[20:28]==new[20:28]==bytes(8)and old[32:40]==new[32:40]==bytes(8),'mapping/view/cursor/next not yet proved NULL'
    assert int.from_bytes(old[4:8],'little')==3,'unexpected mapped-input discriminator'
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
    return dict(original_request=request,original_return=returned,original_live_handle=source,native=call,scope='Actual CreateFileA existing-input request/result carried to published FileSlot.source; live duplicated original disk/size/position identity and native host-fd identity. Mapping/view pointers remain NULL, not normalized. Full current input artifacts compared independently.')
