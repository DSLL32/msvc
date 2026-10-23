#!/usr/bin/env python3
"""Index named pending target callees and explicit original/rebuilt SEH roots."""
import json
import re
import struct
from state import BINARY, load, offset


def pe(path):
    data=path.read_bytes();header=struct.unpack_from('<I',data,60)[0];optional=header+24
    base=struct.unpack_from('<I',data,optional+28)[0];sections=[]
    count=struct.unpack_from('<H',data,header+6)[0]
    for i in range(count):
        p=optional+struct.unpack_from('<H',data,header+20)[0]+40*i
        size,rva,raw,off=struct.unpack_from('<IIII',data,p+8)
        sections.append((base+rva,raw,off))
    def read(address,size):
        for start,length,p in sections:
            if start<=address and address+size<=start+length:return data[p+address-start:p+address-start+size]
        raise AssertionError('unbacked rebuilt data')
    return read


def main():
    config,original,inventory=load()
    symbols={}
    for line in (BINARY/'build/c2.map').read_text().splitlines():
        m=re.match(r'\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',line)
        if m:symbols[m[1]]=int(m[2],16)
    addresses=sorted(set(symbols.values()))
    rebuilt=[]
    for record in config['stubs']:
        start=symbols[record['name']]
        end=next(p for p in addresses if p>start)
        rebuilt.append({'name':record['name'],'start':start,'end':end,'pending_extent':True,
                        'kind':'named-target-stub','evidence':None})
    (BINARY/'build/runtime.json').write_text(json.dumps(rebuilt,indent=2)+'\n')
    read=pe(BINARY/'build/c2.exe')
    metadata={'schema':1,'functions':{},'callback_tables':[]}
    for record in config['driver']:
        if record.get('seh_table'):
            table=record['seh_table']
            enclosing,flt,handler=struct.unpack('<III',original[offset(inventory,table,12):offset(inventory,table,12)+12])
            assert enclosing==0xffffffff
            start=symbols[record['name']]
            prefix=read(start,16)
            assert prefix[:6]==bytes.fromhex('558bec6aff68')and prefix[10]==0x68,'unexpected rebuilt SEH prologue'
            rebuilt_table=struct.unpack_from('<I',prefix,6)[0]
            rebuilt_enclosing,rf,rh=struct.unpack('<III',read(rebuilt_table,12));assert rebuilt_enclosing==0xffffffff
            metadata['functions'][record['name']]={'original':{'roots':[flt,handler]},'rebuilt':{'roots':[rf,rh]},
                'evidence':{'original_scope_table':hex(table),'rebuilt_scope_table':hex(rebuilt_table),
                            'source':'explicit -1 enclosing scope, filter and handler roots from actual compiled PE scope tables'}}
    (BINARY/'build/graph-metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')

if __name__=='__main__':main()
