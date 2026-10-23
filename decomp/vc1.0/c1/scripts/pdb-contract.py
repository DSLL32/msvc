"""Owned PDB object/list contracts from raw, separately retained observations."""
import re
import struct


def memory(text):
    result={}
    for name,address,size,hexadecimal in re.findall(r'^pdb-memory (\S+) ([0-9a-f]+) (\d+) ([0-9a-f]*)$',text,re.M):
        raw=bytes.fromhex(hexadecimal)
        assert len(raw)==int(size) and name not in result
        result[name]=dict(address=int(address,16),data=raw)
    return result


def cleanup_entry(text,boundary,bad_header):
    m=memory(text);obj=m['object-'+boundary];entry=m['list-'+boundary]
    raw=obj['data'];head,tail,count=struct.unpack('<III',entry['data'])
    assert len(raw)==116 and raw[:64]==bad_header[:64], 'actual bad header differs'
    assert obj['address']+72==entry['address'], 'list is not the real constructor member'
    assert raw[64:72]==bytes(8), 'unexpected type allocation in magic-failure context'
    assert raw[72:84]==entry['data'] and head==tail and head and count==0, 'initial owned list differs'
    assert struct.unpack_from('<iII',raw,100)==(-1,1,1), 'constructor descriptor/mode state differs'
    node=m['node-'+boundary+'-0']
    assert node['address']==head and struct.unpack('<III',node['data'])==(0,head+12,0), 'real twelve-byte constructor buffer differs'
    assert not any(k.startswith('node-'+boundary+'-') and k!='node-'+boundary+'-0'for k in m), 'unexpected cleanup chain'
    return dict(header=raw[:64].hex(),list_count=count,nodes=1,node_bytes=12,
        descriptor=-1,writable=1,incremental=1,
        unwritten_constructor_fields='loaded_tail+96 and cumulative+112 remain raw; neither is consumed by this destructor. Freed block-list pointers are retained but never dereferenced.')


def cleanup_return(text,bad_header):
    summary=cleanup_entry(text,'before',bad_header);m=memory(text)
    assert m['object-before']==m['object-after'] and m['list-before']==m['list-after'], 'destructor rewrote caller-owned fields'
    freed=[int(v,16)for v in re.findall(r'^pdb-free ([0-9a-f]+)$',text,re.M)]
    head=struct.unpack_from('<I',m['list-before']['data'])[0]
    assert freed==[head], 'wrong free ownership/order'
    entry=re.search(r'^pdb-registers entry (.*)$',text,re.M)
    returned=re.search(r'^pdb-registers return (.*)$',text,re.M)
    assert entry and returned
    a=[int(v,16)for v in entry[1].split()];b=[int(v,16)for v in returned[1].split()]
    assert a[1]==m['list-before']['address'] and b[0]==a[0]+4 and a[2:]==b[2:], 'ECX this/RET0/nonvolatile ABI differs'
    summary.update(ABI='ECX this/RET0; void EAX/EDX unspecified',freed_nodes=1,caller_object_unchanged=True)
    return summary
