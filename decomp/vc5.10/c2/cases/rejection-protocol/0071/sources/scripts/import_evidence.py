"""Narrow byte-bound evidence for reviewed cached imports, beyond Rust IAT calls.

This does not discover graphs or guess register dataflow. The complete bounded
live interval is reviewed explicitly and pinned; unknown changed bytes fail.
"""
import re
import struct
from pathlib import Path
from state import BINARY, TARGET, CANONICAL_CRT, ROOT, load, offset, sha


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


def verify_free_provider(path, expected_sha256):
    content=path.read_bytes();assert sha(content)==expected_sha256
    proof=__import__('json').loads(content)
    provider=TARGET.parent/'MSVCRT.DLL';data=provider.read_bytes()
    assert sha(data)==proof['provider_sha256']==proof['actual_hfile_sha256']==CANONICAL_CRT
    read=pe_read(provider);header=struct.unpack_from('<I',data,60)[0];opt=header+24
    base=struct.unpack_from('<I',data,opt+28)[0];export_rva=struct.unpack_from('<I',data,opt+96)[0]
    export=struct.unpack('<IIHHIIIIIII',read(base+export_rva,40))
    ordinal,count,names,functions,name_table,ordinals=export[5:]
    assert 0<names<=count<65536
    found=[]
    for index in range(names):
        address=base+struct.unpack('<I',read(base+name_table+4*index,4))[0]
        name=bytearray()
        for number in range(256):
            char=read(address+number,1)
            if char==b'\0':break
            name+=char
        else:raise AssertionError('unbounded provider export name')
        if name==b'free':
            entry=struct.unpack('<H',read(base+ordinals+2*index,2))[0];assert entry<count
            target=base+struct.unpack('<I',read(base+functions+4*entry,4))[0]
            found.append({'dll':'msvcrt.dll','name':'free','ordinal':ordinal+entry,'address':target})
    assert found==[proof['public_export']], 'public canonical free export identity differs'
    assert sha((ROOT/proof['declaration']['path']).read_bytes())==proof['declaration']['sha256']
    body=proof['body'];assert body['start']==found[0]['address'] and body['end']==0x780013f2
    assert sha(read(body['start'],body['end']-body['start']))==body['sha256']
    assert [item['address']for item in proof['normal_returns']]==[0x780013d4,0x780013f1]
    for item in proof['normal_returns']:
        assert item['preceding_start']==item['address']-2 and item['bytes']=='5e59c3'
        assert read(item['preceding_start'],3)==bytes.fromhex(item['bytes'])
    return proof



def verify_writer_provider(packet):
    """Exact public boundaries, not a private dependency implementation."""
    import json
    public=packet['writer_provider_evidence']
    assert {(r['symbol'],Path(r['path']).name)for r in public}=={
        ('fopen','MSVCRT.DLL'),('fclose','MSVCRT.DLL'),
        ('StreamQueryCb','MSPDB50.DLL'),('StreamRead','MSPDB50.DLL')}
    for row in public:
        path=ROOT/row['path']
        assert path.parent==TARGET.parent and path.name in ('MSVCRT.DLL','MSPDB50.DLL')
        data=path.read_bytes();assert sha(data)==row['sha256']
        if path.name=='MSVCRT.DLL':assert row['sha256']==CANONICAL_CRT
        read=pe_read(path);header=struct.unpack_from('<I',data,60)[0];opt=header+24
        base=struct.unpack_from('<I',data,opt+28)[0];exports=struct.unpack('<IIHHIIIIIII',read(base+struct.unpack_from('<I',data,opt+96)[0],40))
        ordinal,count,names,functions,name_table,ordinals=exports[5:]
        assert 0<names<=count<65536
        actual=[]
        for index in range(names):
            address=base+struct.unpack('<I',read(base+name_table+index*4,4))[0]
            name=bytearray()
            for i in range(256):
                char=read(address+i,1)
                if char==b'\0':break
                name+=char
            else:raise AssertionError('unbounded provider export')
            if name.decode('ascii')==row['symbol']:
                entry=struct.unpack('<H',read(base+ordinals+index*2,2))[0];assert entry<count
                actual.append({'ordinal':ordinal+entry,'address':base+struct.unpack('<I',read(base+functions+entry*4,4))[0]})
        assert actual==[row['export']]
        for interval in row['ranges']:
            assert interval['start']<interval['end']
            assert sha(read(interval['start'],interval['end']-interval['start']))==interval['sha256']
        if row['symbol']=='StreamQueryCb':assert read(row['export']['address'],8)==bytes.fromhex('8b4c24048b01ff20')
        if row['symbol']=='StreamRead':assert read(row['export']['address'],22)==bytes.fromhex('ff7424108b4c2408ff7424108b01ff742410ff5004c3')
    declaration=packet['mspdb_declaration'];path=BINARY/declaration['path']
    assert sha(path.read_bytes())==declaration['sha256']
    later=json.loads(path.read_text())
    assert later['commit']=='805655a28bd8198004be2ac27e6e0290121a5e89'
    assert later['scope'].startswith('Later published Microsoft declaration')
    # Existing reader13 EBP claims remain exactly13; only eight finite targets
    # acquire EDI, including two publicCRT calls and four pending stream helpers.


def verify(config,graph):
    _,original,inventory=load();rebuilt=pe_read(BINARY/'build/c2.exe')
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',
            (BINARY/'build/c2.map').read_text(),re.M)}
    evidence=[]
    enabled=sorted(r['name']for r in config['driver']if r['name']in config['accepted'] and r.get('register_imports',{}).get('enabled'))
    if enabled:
        inputs=graph['register_import_inputs']
        assert inputs['enabled_functions']==enabled, 'per-owner resolver scope differs'
        assert inputs['original_sha256']==sha(original) and inputs['rebuilt_sha256']==sha((BINARY/'build/c2.exe').read_bytes())
        assert inputs['preservation_contracts']==config['register_imports']['preservation_contracts'], 'callee-preservation inputs differ'
    for record in config['driver']:
        if record['name'] not in config['accepted'] or not record.get('register_import_sites'):continue
        result=graph['functions'][record['name']]['register_imports']
        assert result['require_all_register_calls_resolved'] is True
        for side in ('original','rebuilt'):
            observed=result[side]
            assert not observed['unresolved_register_calls'] and not observed['other_unresolved_calls'], 'unresolved cached import transfer'
            assert observed['resolved_register_calls']==record['register_import_sites'][side], 'cached import site inventory differs'
            base=record['start'] if side=='original' else maps[record.get('symbol',record['name'])]
            direct={str(base+int(site)):identity for site,identity in record['direct_import_offsets'][side].items()}
            assert observed['direct_import_calls']==direct, 'direct import site inventory differs'
        if record['name']=='@read_initializer_records@4':
            verify_free_provider(BINARY/record['provider_abi_evidence'],record['provider_abi_evidence_sha256'])
        else:assert record['name']=='_initialize_object_output','unknown resolver proof owner'
        contracts=config['register_imports']['preservation_contracts']
        paths={entry['evidence']['path'] for entry in contracts}
        assert len(paths)==1, 'reader ABI witness scope changed'
        path=BINARY/next(iter(paths));content=path.read_bytes()
        assert all(entry['evidence']['sha256']==sha(content) for entry in contracts)
        packet=__import__('json').loads(content)
        assert packet['target_sha256']==sha(original) and packet['rebuilt_sha256']==sha((BINARY/'build/c2.exe').read_bytes())
        if 'writer_scope'in packet:
            verify_writer_provider(packet)
            reader_targets={__import__('json').dumps(v,sort_keys=True)for v in packet['reader_scope_preserved']}
            assert len(reader_targets)==13
            assert {__import__('json').dumps(v['target'],sort_keys=True)for v in contracts if 'ebp'in v['preserves']}==reader_targets
            expected_edi={'@c2_pool_allocate@8','@c2_report_internal@8','_query_incremental_stream_size',
                '@read_incremental_stream@8','_read_incremental_byte','_validate_incremental_options'}
            assert {v['target']['function']for v in contracts if 'edi'in v['preserves']and 'function'in v['target']}==expected_edi
            assert {v['target']['import']['name']for v in contracts if 'edi'in v['preserves']and 'import'in v['target']}=={'fopen','fclose'}
            assert all(set(v['preserves'])<= {'ebp','edi'}for v in contracts)
        definitions={r['name']:r for r in config['driver']+config['stubs']}
        declared={entry['target']['function'] for entry in contracts if 'function' in entry['target']}
        assert declared=={entry['function']for entry in packet['callee_preservation']}
        for entry in packet['callee_preservation']:
            definition=definitions[entry['function']]
            contract=next(v for v in contracts if v['target'].get('function')==entry['function'])
            assert entry['abi']==definition['abi'] and entry['source_symbol']==definition['native'] and entry['preserves']==contract['preserves']
            assert entry['rebuilt_entry']==maps[definition.get('symbol',definition['name'])]
            for chunk in entry['original']:
                start,end=chunk['start'],chunk['end'];pos=offset(inventory,start,end-start)
                assert start<end and sha(original[pos:pos+end-start])==chunk['sha256'], 'callee ABI byte witness changed'
    for record in config['driver']:
        if record['name']not in config['accepted']:continue
        for cache in record.get('cached_imports',[]):
            identity={'dll':cache['dll'].lower(),'name':cache['symbol']}
            for side in ('original','rebuilt'):
                item=cache[side];start=record['start']if side=='original'else maps[record['name']]
                read=(lambda address,size:original[offset(inventory,address,size):offset(inventory,address,size)+size])if side=='original'else rebuilt
                slot=item['slot'];assert graph['import_catalogs'][side]['slots'][str(slot)]==identity
                for data in item.get('accessed_data',[]):
                    assert graph['import_catalogs'][side]['slots'][str(data['slot'])]=={'dll':data['dll'].lower(),'name':data['symbol']},'cached import context data identity changed'
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
