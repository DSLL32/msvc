"""Narrow byte-bound evidence for reviewed cached imports, beyond Rust IAT calls.

This does not discover graphs or guess register dataflow. The complete bounded
live interval is reviewed explicitly and pinned; unknown changed bytes fail.
"""
import re
import struct
from pathlib import Path
from state import BINARY, TARGET, load, offset, sha


def pe_read(path):
    data=path.read_bytes();header=struct.unpack_from('<I',data,60)[0];optional=header+24
    base=struct.unpack_from('<I',data,optional+28)[0]
    count=struct.unpack_from('<H',data,header+6)[0];section=optional+struct.unpack_from('<H',data,header+20)[0]
    def read(address,size):
        for j in range(count):
            _,rva,raw,file=struct.unpack_from('<IIII',data,section+40*j+8)
            if base+rva<=address and address+size<=base+rva+raw:
                return data[file+address-base-rva:file+address-base-rva+size]
        raise AssertionError('unbacked reviewed import evidence')
    return read


def verify(config,graph):
    _,original,inventory=load();rebuilt=pe_read(BINARY/'build/c2.exe')
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',
            (BINARY/'build/c2.map').read_text(),re.M)}
    evidence=[]
    for record in config['driver']:
        if record['name']not in config['accepted']:continue
        for cache in record.get('cached_imports',[]):
            identity={'dll':cache['dll'].lower(),'name':cache['symbol']}
            for side in ('original','rebuilt'):
                item=cache[side];start=record['start']if side=='original'else maps[record['name']]
                read=(lambda address,size:original[offset(inventory,address,size):offset(inventory,address,size)+size])if side=='original'else rebuilt
                slot=item['slot'];assert graph['import_catalogs'][side]['slots'][str(slot)]==identity
                for instruction in item['instructions']:
                    address=start+instruction['offset'];expected=bytes.fromhex(instruction['bytes'])
                    assert read(address,len(expected))==expected,'reviewed cached-import instruction changed'
                intervals=item.get('lifetime_ranges',[item.get('lifetime')])
                hashes=item.get('lifetime_sha256s',[item.get('lifetime_sha256')])
                assert intervals and len(intervals)==len(hashes)
                for interval,digest in zip(intervals,hashes):
                    low=start+interval[0];high=start+interval[1]
                    assert low<high
                    if side=='original':
                        assert any(a<=low<high<=b for a,b in [[record['start'],record['end']]]+record.get('code_chunks',[])),'cached import interval exceeds owned function code'
                    assert sha(read(low,high-low))==digest,'cached-import live interval changed: review new compiled bytes'
            evidence.append({'function':record['name'],'identity':identity,'abi':cache['abi'],
                             'original':cache['original'],'rebuilt':cache['rebuilt'],
                             'scope':'Qualified slot/load/calls and manually reviewed complete live interval; Rust generic indirect edge is not import identity proof'})
    return evidence
