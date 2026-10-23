#!/usr/bin/env python3
"""DLL export/entry policy, separate from shared function-graph discovery."""
import json
import re
import struct
from pathlib import Path
from protocol import HERE,ROOT,sha

def image(path):
    data=path.read_bytes();pe=struct.unpack_from('<I',data,60)[0];opt=pe+24
    assert data[:2]==b'MZ' and data[pe:pe+4]==b'PE\0\0'
    machine,count=struct.unpack_from('<HH',data,pe+4);size,flags=struct.unpack_from('<HH',data,pe+20)
    assert machine==0x14c and flags&0x2000 and struct.unpack_from('<H',data,opt)[0]==0x10b
    sections=[struct.unpack_from('<IIII',data,opt+size+i*40+8)for i in range(count)]
    def offset(rva):
        for _,start,size,raw in sections:
            if start<=rva<start+size:return raw+rva-start
        raise AssertionError('unbacked PE RVA')
    directory=struct.unpack_from('<I',data,opt+96)[0];at=offset(directory)
    ordinal,count,names,functions,name_table,ordinals=struct.unpack_from('<IIIIII',data,at+16)
    result={}
    for i in range(names):
        name_rva=struct.unpack_from('<I',data,offset(name_table)+4*i)[0];n=offset(name_rva);name=data[n:data.index(b'\0',n)].decode('ascii')
        index=struct.unpack_from('<H',data,offset(ordinals)+2*i)[0]
        result[name]=ordinal+index
    assert struct.unpack_from('<II',data,opt+96+9*8)==(0,0),'unexpected TLS lifecycle'
    resource_rva=struct.unpack_from('<I',data,opt+96+2*8)[0]
    resource=None
    if resource_rva:
        root=offset(resource_rva)
        def level(relative):
            at=root+relative;named,ids=struct.unpack_from('<HH',data,at+12);assert named==0
            return dict(struct.unpack_from('<II',data,at+16+i*8)for i in range(ids))
        types=level(0)
        if 11 in types:
            assert types[11]&0x80000000
            names=level(types[11]&0x7fffffff);assert set(names)=={1}
            languages=level(names[1]&0x7fffffff);assert set(languages)=={1033}
            leaf=languages[1033];assert not leaf&0x80000000
            rva,size,codepage,reserved=struct.unpack_from('<IIII',data,root+leaf)
            resource=data[offset(rva):offset(rva)+size]
    return dict(resource=resource,exports=result,entry=struct.unpack_from('<I',data,opt+16)[0],base=struct.unpack_from('<I',data,opt+28)[0])
old=image(ROOT/'toolchains/5.10/bin/RCDLL.DLL');new=image(HERE/'build/RCDLL.DLL')
assert old['exports']==new['exports']==dict(Handler=1,RC=2)
assert new['resource']==(HERE/'build/help-messages.bin').read_bytes(),'real DLL lacks the encountered message table'
resource=new['resource'];count=struct.unpack_from('<I',resource)[0];assert count==2
ids=[]
for i in range(count):
    low,high,at=struct.unpack_from('<III',resource,4+i*12);assert low==high
    size,flags=struct.unpack_from('<HH',resource,at);assert flags==0 and size%4==0
    ids.append(low)
assert ids==[10001,20001]
text=(HERE/'build/rcdll.map').read_text()
for name in ['_Handler@4','_RC@24','_recon_dll_entry@12','_DllMain@12']:assert re.search(r'\s'+re.escape(name)+r'\s',text),name
assert '_rcdll_main@12' not in text,'obsolete application-entry symbol'
entry=re.findall(r'\s_recon_dll_entry@12\s+([0-9a-fA-F]+)\s',text);assert len(entry)==1
assert int(entry[0],16)==new['base']+new['entry']
print(json.dumps(dict(status='pass',exports=new['exports'],entry_symbol='_recon_dll_entry@12',application_entry_symbol='_DllMain@12',tls=False,message_ids=ids,message_language=1033,rebuilt_sha256=sha(HERE/'build/RCDLL.DLL'))))
