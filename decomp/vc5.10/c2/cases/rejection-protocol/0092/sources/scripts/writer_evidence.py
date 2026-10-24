"""Finite writer storage, recovered allocator ABI and pending callback guards."""
import json
import re
from state import BINARY,TARGET,sha
from data_evidence import reader


def model(config):
    if '_initialize_object_output' not in config['accepted']:return None
    raw=(BINARY/config['writer_callback_boundary']).read_bytes()
    assert sha(raw)==config['writer_callback_boundary_sha256']
    value=json.loads(raw);assert value['target_sha256']==sha(TARGET.read_bytes())
    assert value['record']['size']==32 and len(value['targets'])==3
    return value


def verify(config,graph):
    value=model(config)
    if value is None:return None
    definitions={r['name']:r for r in config['driver']+config['stubs']}
    allocator=value['targets'][0]
    typed='abi_evidence' in allocator
    allocator_body=typed and allocator['logical']in config['accepted']
    for target in value['targets']:
        row=definitions[target['logical']]
        assert row['native']==target['native']and row['start']==target['original']and row['kind']=='application'
        if allocator_body and target==allocator:
            assert row['end']==0x41eb96 and not row['pending_extent']
        else:
            assert row['end']==row['start']+1 and row['pending_extent']
            assert row['name']not in config['accepted']+config['trial']
        assert row['pending_signature'] is (not typed or target!=allocator)
    assert graph['original_accepted_graphs'][value['owner']]==value['original_graph']
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    original,rebuilt=reader(TARGET),reader(BINARY/'build/c2.exe')
    assert original(0x48e020,96)==rebuilt(maps['_c2_pools'],96)==bytes(96)
    for key,address in [('object_output',0x497e08),('object_output_open',0x497e1c),('incremental_data',0x49e31c),('directive_section',0x4973fc),('fpo_section',0x497dc0)]:
        assert original(address,4)==rebuilt(maps['_'+key],4)==bytes(4)
    if '@initialize_coff_output@4'in config['accepted']:
        from globals_state import COFF_GLOBALS
        for key,(address,size)in COFF_GLOBALS.items():
            expected=(0xffffffff if key=='coff_reuse_index'else 1 if key in ('coff_pending_c90','function_option_23')else 0).to_bytes(size,'little')
            assert original(address,size)==rebuilt(maps['_'+key],size)==expected,'COFF global initial storage/width differs: '+key
    if '@add_coff_section@16'in config['accepted']:
        verify_reuse_array(TARGET,0x49e6e0)
        verify_reuse_array(BINARY/'build/c2.exe',maps['_coff_reuse_sections'])
    if '@record_output_section@4'in config['accepted']:
        verify_reuse_array(TARGET,0x497d00,16)
        verify_reuse_array(BINARY/'build/c2.exe',maps['_coff_section_buckets'],16)
    fields={r['name']:r for r in value['record']['fields']if r['name'].startswith('output_')}
    assert {r['offset']for r in fields.values()}=={16,20,24}
    if typed:
        raw=(BINARY/allocator['abi_evidence']).read_bytes()
        assert sha(raw)==allocator['abi_evidence_sha256']
        abi=json.loads(raw);row=definitions[allocator['logical']]
        body=abi['original'];assert body['start']==0x41eb8a and body['end']==0x41eb96
        assert original(body['start'],12)==bytes.fromhex(body['bytes'])
        assert sha(original(body['start'],12))==body['sha256']
        assert abi['target_sha256']==value['target_sha256'] and row['abi']==abi['abi']
        assert row['symbol']==abi['actual_symbol']=='@allocate_output_bytes@4'
        assert abi['tail_target']['function'] in config['accepted']
        assert abi['type']=='void *(__fastcall *C2OutputAllocator)(unsigned bytes)'
        assert abi['field']=={'writer_offset':16,'global':0x497d88,'source_copy_site':0x453c96,'global_store_site':0x453ca1}
    if typed and not allocator_body:
        assert re.search(r'void \*__fastcall allocate_output_bytes\(unsigned bytes\)\s*\{\s*__asm \{ int 3 \}\s*return NULL;\s*\}',(BINARY/'src/stubs.c').read_text()),'allocator must remain an exact named trap'
    if allocator_body:
        assert re.search(r'void \*__fastcall allocate_output_bytes\(unsigned bytes\)\s*\{\s*return c2_pool_allocate\(7, bytes\);\s*\}',(BINARY/'src/coff-output.c').read_text()),'allocator body must retain its exact ordinary typed call'
    pending={name:row for name,row in fields.items()if not typed or name!='output_allocator'}
    target_names=[r['native']for r in value['targets']if not typed or r!=allocator]
    # Exact finite line forms: a new use fails rather than interpreting an
    # incomplete-prototype target as a callable API. Typed allocator use is
    # permitted only after its separately bound complete original ABI proof.
    for path in (BINARY/'src').glob('*'):
        if path.suffix not in ('.c','.h'):continue
        for line in path.read_text().splitlines():
            stripped=line.strip()
            for name,row in pending.items():
                if not re.search(r'\b'+name+r'\b',line):continue
                target='writer_callback_'+format(row['target'],'x')
                forms={('coff-output.c','writer->'+name+'.target = '+target+';'),
                    ('coff-output.h','C2PendingCallback '+name+';')}
                if typed:forms.add(('coff-output.c',name+'_handler = writer->'+name+';'))
                layout=path.name=='coff-output.h' and re.fullmatch(r'typedef char C2ObjectOutput\w+Check\[offsetof\(C2ObjectOutput, '+name+r'\) == '+str(row['offset'])+r' \? 1 : -1\];',stripped)
                assert (path.name,stripped)in forms or layout,'pending writer callback invocation/escape requires recovered actual signature'
            if typed and re.search(r'\boutput_(error|clock)_handler\b',line):
                assert (path.name=='coff-output.h'and stripped=='extern C2PendingCallback output_error_handler, output_clock_handler;')or(path.name=='coff-output.c'and stripped in ('C2PendingCallback output_error_handler, output_clock_handler;','output_error_handler = writer->output_error;','output_clock_handler = writer->output_clock;')),'pending copied callback invocation/escape requires recovered signature'
            for name in target_names:
                if not re.search(r'\b'+name+r'\b',line):continue
                assert (path.name=='coff-output.c'and re.fullmatch(r'writer->output_\w+\.target = '+name+r';',stripped))or(path.name=='coff-output.h'and stripped=='void '+name+'();')or(path.name=='stubs.c'and stripped=='void '+name+'() { __asm { int 3 } }'),'pending writer target may only be declared, stored, or trap'
            if typed and re.search(r'\ballocate_output_bytes\b',line):
                assert (path.name=='coff-output.c'and(stripped=='writer->output_allocator = allocate_output_bytes;'or allocator_body and stripped=='void *__fastcall allocate_output_bytes(unsigned bytes)'))or(path.name=='coff-output.h'and stripped=='void *__fastcall allocate_output_bytes(unsigned bytes);')or(path.name=='stubs.c'and stripped=='void *__fastcall allocate_output_bytes(unsigned bytes)'),'allocator source must match its pending/independently accepted role'
    return {'scope':'Typed allocator; body credit only in independently audited accepted set. Error/clock stores and copied identities only; their bodies pending',
        'boundary_sha256':config['writer_callback_boundary_sha256'],'allocator_abi_sha256':allocator.get('abi_evidence_sha256'),
        'fields':list(fields.values()),'targets':[{'original':r['original'],'rebuilt':maps[definitions[r['logical']].get('symbol','_'+r['native'])],'native':r['native']}for r in value['targets']],
        'original_graph':value['original_graph']}


def verify_reuse_array(path,address,count=8):
    """Explicit typed eight/sixteen-pointer arrays: exact file zero or loader tail."""
    assert count in (8,16)
    length=count*4
    import struct
    try:
        assert reader(path)(address,length)==bytes(length)
        return
    except AssertionError:
        data=path.read_bytes();header=struct.unpack_from('<I',data,60)[0];opt=header+24
        base=struct.unpack_from('<I',data,opt+28)[0];section=opt+struct.unpack_from('<H',data,header+20)[0]
        tails=[]
        for i in range(struct.unpack_from('<H',data,header+6)[0]):
            virtual,rva,raw=struct.unpack_from('<III',data,section+i*40+8)
            if base+rva+raw<=address and address+length<=base+rva+virtual:tails.append([base+rva+raw,base+rva+virtual])
        assert len(tails)==1,'reuse array is neither file-zeroed nor in a proven PE zero-filled tail'
