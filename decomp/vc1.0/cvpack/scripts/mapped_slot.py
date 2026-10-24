"""Active Win32 HANDLE in the concretely reached mapped input slot."""
import re

def compare(pointer,old,new,actual,events,native,mapped_seek=False,mapped_write=False,mapped_close=False):
    assert len(old)==len(new)==40 and old[4:16]==new[4:16]and old[28:36]==new[28:36],'mapped slot scalar inputs differ'
    # close_file already moved this slot onto the free list. close_mapped_file
    # never reads/writes next(+36); retain raw links without equality credit.
    if not mapped_close:assert old[36:40]==new[36:40]==bytes(4),'next slot not NULL'
    if not mapped_write:assert old[32:36]==new[32:36]==bytes(4),'initial cursor not zero'
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
    assert int(source[2])==1 and int(source[3])==int.from_bytes(old[28:32],'little')==2097152 and int(source[4])==int(source[5]),'original live file state differs'
    if mapped_close:
        current=actual['current_source'];assert current==dict(handle=int.from_bytes(new[16:20],'little'),physical_size=int(source[3]),position=int(source[4]),owned=True),'current native handle state differs'
    else:assert int.from_bytes(old[12:16],'little')==2097152 and int(source[4])==0,'initial live file size/position differs'
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


def compare_view_input(pointer,old,actual,events,words,file):
    assert words[0]==0x2a423 and words[4]==0x1f294 and words[3]==64,'unknown mapped-read caller/count'
    rows=re.findall(r'^mapped-view-input slot ([0-9a-f]+) mapping ([0-9a-f]+) view ([0-9a-f]+) extent (\d+) cursor (\d+) count (\d+) region ([0-9a-f]+) ([0-9a-f]+) (\d+) (\d+) (\d+) (\d+) bytes ([0-9a-f]+)$',events,re.M)
    assert len(rows)==1,'missing actual consumed view bytes/extent'
    r=rows[0];view=int.from_bytes(old[24:28],'little');mapping=int.from_bytes(old[20:24],'little');new=actual['view_input']
    assert int(r[0],16)==pointer and int(r[1],16)==mapping and int(r[2],16)==view,'original view owner differs'
    assert int(r[3])==int(r[8])==2097152 and int(r[4])==0 and int(r[5])==64,'view extent/cursor/count differs'
    assert int(r[6],16)==int(r[7],16)==view and [int(r[i])for i in(9,10,11)]==[0x1000,4,0x40000],'original committed mapped RW region differs'
    assert new['slot']==actual['address']and new['mapping']==int.from_bytes(bytes.fromhex(actual['bytes'])[20:24],'little')and new['view']==int.from_bytes(bytes.fromhex(actual['bytes'])[24:28],'little'),'native view owner differs'
    assert new['registered_view']and new['owned_mapping']and new['extent']==2097152 and new['cursor']==0 and new['count']==64,'native mapped extent/input differs'
    assert len(bytes.fromhex(r[12]))==64 and r[12]==new['bytes']==file.read_bytes()[:64].hex(),'actual consumed mapped bytes differ'
    return dict(original=r,native=new,scope='Actual first64-byte DOS-header input from the real2MB mapped allocation; original region and actual mapping returns, native mapping/view registry and source-file identity. No comparison credit for unconsumed mapping payload or undefined DOS output prebytes.')


def compare_destination(pointer,old,actual,events,words):
    rows=re.findall(r'^mapped-destination slot ([0-9a-f]+) mapping ([0-9a-f]+) view ([0-9a-f]+) extent (\d+) cursor (\d+) count (\d+) region ([0-9a-f]+) ([0-9a-f]+) (\d+) (\d+) (\d+) (\d+)$',events,re.M)
    assert words[0]==0x2a693 and len(rows)==1,'missing actual mapped output caller/owner'
    r=rows[0];new=actual['view_input'];raw=bytes.fromhex(actual['bytes'])
    word=lambda data,offset:int.from_bytes(data[offset:offset+4],'little')
    assert int(r[0],16)==pointer and int(r[1],16)==word(old,20)and int(r[2],16)==word(old,24),'original mapped destination differs'
    assert int(r[3])==int(r[8])==2097152 and int(r[4])==word(old,32)and int(r[5])==words[3],'actual destination extent/cursor/count differs'
    assert int(r[6],16)==int(r[7],16)==word(old,24)and [int(r[i])for i in(9,10,11)]==[0x1000,4,0x40000],'unknown original destination mapping'
    assert new['slot']==actual['address']and new['mapping']==word(raw,20)and new['view']==word(raw,24)and new['registered_view']and new['owned_mapping'],'native destination owner differs'
    assert new['extent']==int(r[3])and new['cursor']==int(r[4])and new['count']==int(r[5])and 0<new['count']<=new['extent']-new['cursor'],'native destination range differs'
    return dict(original=r,native=new,scope='Actual writable2MB mapped destination owner and overwritten range. The write reads no previous destination payload; source defined output prefix is independently compared. No destination-byte or arbitrary pointer equivalence claim.')
