"""Offline defined-state/ABI contract for the four fixed resource units."""
import json,re
from pathlib import Path

def records(text,when):
    rows=re.findall(r'^resource-node '+when+r' (\d+) (\w+) ((?:[0-9a-f]{8} ?){13})$',text,re.M);assert len(rows)==5
    nodes=[]
    for index,pointer,words in rows:
        assert int(index)==len(nodes);nodes.append((int(pointer,16),[int(v,16)for v in words.split()]))
    names={int(i):name for i,name in re.findall(r'^resource-string '+when+r'-(\d+) \w+ (.*)$',text,re.M)};assert len(names)==5
    for i,(pointer,words)in enumerate(nodes):assert words[12]==(nodes[i+1][0]if i<4 else 0)
    return nodes,names

def projection(text,when):
    rows,names=records(text,when);root_buffer=rows[4][1][5];out=[]
    for i,(_,w)in enumerate(rows):
        out.append(dict(name=names[i],capacity=w[0],handle=w[1],saved_line=w[2],buffer_relative=w[5]-root_buffer,index=w[6]&65535,available=w[7],offset=w[8],packing=w[9],kind=w[10],condition=w[11],saved_cursor=(w[4]-rows[i+1][1][5]if i<4 else w[4])))
    return out

def buffers(text,boundary=None):
    if boundary is not None:
        marker='resource-streams '+boundary+'\n';assert marker in text;text=text.split(marker,1)[1].split('resource-streams ',1)[0]
    result={int(i):None if size=='unopened'else value for i,size,value in re.findall(r'^buffer (\d+) (unopened|\d+)(?: ([0-9a-f]*))?$',text,re.M)};assert set(result)==set(range(7));return result

def validate(original,native,plan,entry=False):
    original=Path(original);native=Path(native);ot=(original/'events').read_text();nt=(native/'report').read_text();unit=plan['unit'];assert projection(ot,'before')==projection(nt,'before'),'real owner state differs'
    assert re.search(r'^resource-state before depth 4 errno 2 dos 2$',ot,re.M)and re.search(r'^resource-state before depth 4 errno 2 dos 2$',nt,re.M)
    for text in [ot,nt]:
        assert re.search(r'^resource-string kind \w+ include$',text,re.M)and re.search(r'^resource-string missing \w+ absent-resource.h$',text,re.M)
    state=json.loads((native/'state.json').read_text())if(native/'state.json').exists()else None
    if entry:
        assert state and state['status']=='entry'and len(state['calls'])==1
        exception=re.search(r'^exception 80000003 rva (\w+) base (\w+) first 1$',ot,re.M);assert exception and int(exception[1],16)+int(exception[2],16)==plan['start']
        tail=ot[exception.end():];slots={int(i):int(v,16)for i,v in re.findall(r'^stack (\d+) (\w+)$',tail,re.M)};assert slots[0]==0x4aa90
        if unit in[0,3]:assert slots[1]==(96 if unit==0 else 2)
        if unit==2:
            rows,_=records(ot,'before');m=re.search(r'^resource-context unit 2 marker \w+ esp \w+ line (\d+)$',ot,re.M);assert m
            strings={key:int(ptr,16)for key,ptr in re.findall(r'^resource-string (kind|missing) (\w+) ',ot,re.M)}
            assert [slots[i]for i in range(1,6)]==[83,strings['kind'],strings['missing'],rows[0][1][3],int(m[1])]
    else:
        if unit!=3:assert buffers(ot)==buffers(nt,'after'if unit<2 else'before')
        if unit<2:
            left_errors=re.search(r'^resource-state after depth 4 errno (\d+) dos (\d+)$',ot,re.M);right_errors=re.search(r'^resource-state after depth 4 errno (\d+) dos (\d+)$',nt,re.M)
            assert left_errors and right_errors and left_errors.groups()==right_errors.groups()
            assert projection(ot,'after')==projection(nt,'after')
            before=projection(ot,'before');after=projection(ot,'after')
            for i in range(5):
                expected=dict(before[i]);
                if unit==1 and i==2:expected['handle']=0xffffffff
                assert expected==after[i]
            ret=re.search(r'^direct-return eax (\w+) edx \w+ esp (\w+) expected (\w+) nonvolatile 1 fctrl (\w+) before (\w+)$',ot,re.M);assert ret and ret[2]==ret[3]and ret[4]==ret[5]
            assert 'direct-canary a5a5a5a5 a5a5a5a5 a5a5a5a5 a5a5a5a5'in ot
            assert (int(ret[1],16)!=0)if unit==0 else(int(ret[1],16)==1)
            if state:assert 'returned_registers'in state['calls'][0]
        elif unit==2:
            terminal=re.search(r'^resource-terminal unit 2 site \w+ esp \w+ ((?:\w+ ?){7})$',ot,re.M);assert terminal
            frame=[int(v,16)for v in terminal[1].split()];rows,_=records(ot,'before');assert frame[1]==83 and frame[5]==rows[0][1][3]and frame[6]==12
            if state:assert len(state['forwarded'])==1
        else:
            assert re.search(r'^resource-terminal unit 3 site 00053530 esp \w+ \w+ 00008000$',ot,re.M)
            assert buffers(ot,'exit')==buffers(nt,'exit')
        if unit>=2:
            receipt=json.loads((native/'receipt.json').read_text());assert receipt['full_terminal_status']==(1 if unit==2 else 32768)
    if not entry:
        assert (original/'stdout').read_bytes()==(native/'stdout').read_bytes()
        assert (original/'stderr').read_bytes()==(native/'stderr').read_bytes()
        receipt=json.loads((native/'receipt.json').read_text());work=Path(receipt['work'])
        left={p.name:p.read_bytes()for p in(original/'work').iterdir()if p.is_file()};right={p.name:p.read_bytes()for p in work.iterdir()if p.is_file()};assert left==right,'complete owned files differ'
    return dict(status='pass',unit=unit,public_reachability=False,scope='Genuine missing include owner/defined state, direct ABI and owned output contract')
