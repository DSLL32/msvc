#!/usr/bin/env python3
"""Meaningful negative checks for acceptance/identity failure paths."""
from graph import graph, names
from runtime import identify
from state import mask
from state import ROOT
from state import BINARY
from binary import PE
from audit import publics,validate_ownership,audited_graph
from accept import validate_snapshot,compare_integer
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from pathlib import Path
from objects import compare_walks,original_walks
import struct
import subprocess
import tempfile
import copy
import json
from state import load,sha
from seams import approved,validate as validate_seam,snapshot_policy,check_policy_snapshot,compare_returns,check_guard_references

class Bytes:
    def __init__(self,data):self.data=data
    def read(self,address,size):return self.data[address:address+size]

def reject(label, action, reason=None):
    try:action()
    except (AssertionError,ValueError,IndexError,KeyError)as error:
        if reason:assert reason in str(error),(label,error)
        print('rejected:',label)
        return
    raise AssertionError('accepted invalid '+label)

def main():
    reject('truncated RET cleanup',lambda:graph(Bytes(b'\xc2\x04\x00'),0,2))
    reject('truncated immediate',lambda:graph(Bytes(b'\xb8\0\0\0\0\xc3'),0,3))
    reject('missing terminal',lambda:graph(Bytes(b'\x90\xc3'),0,1))
    reject('branch into operand',lambda:graph(Bytes(b'\x75\x01\xb8\0\0\0\0\xc3'),0,8))
    reject('undeclared indirect jump',lambda:graph(Bytes(b'\xff\xe0'),0,2))
    reject('conditional outside function',lambda:graph(Bytes(b'\x75\x20\xc3'),0,3))
    reject('branch into text data',lambda:graph(Bytes(b'\xeb\0\xc3'),0,3,data=[(2,3)]))
    reject('unresolved application call',lambda:names({0x12345678},{}))
    aligned=Bytes(b'\xeb\x05\x05\0\0\0\0\xc3')
    audited_graph(aligned,0,8,padding=[(2,7)])
    reject('undeclared flag-writing alignment',lambda:audited_graph(aligned,0,8))
    reject('declared alignment on executable path',lambda:audited_graph(Bytes(b'\x90\x90\x05\0\0\0\0\xc3'),0,8,padding=[(2,7)]))
    reject('nonalignment bytes in reviewed padding',lambda:audited_graph(Bytes(b'\xeb\x05\xb8\0\0\0\0\xc3'),0,8,padding=[(2,7)]))
    reject('overlapping reviewed alignment declarations',lambda:audited_graph(aligned,0,8,padding=[(2,7),(2,7)]))
    reject('alignment declaration overlaps text data',lambda:audited_graph(aligned,0,8,data=[(2,7)],padding=[(2,7)]))
    positive=graph(Bytes(b'\x74\x01\x90\xc2\x08\x00'),0,6)
    assert positive['returns']=={8} and positive['end']==6
    # A tiny wrapper with identical nonrelocation bytes must fail when its
    # external `_main` target is mapped to the wrong application identity.
    target=ROOT/'toolchains/1.0-nt/bin/CVPACK.EXE'
    good=identify(target,{'_main':0x17000})
    configuration=json.loads((BINARY/'functions.json').read_text());context=json.loads((BINARY/'data.json').read_text())
    duplicate=dict(context[0],name='wrong-overlapping-owner',original=context[0]['original']+1,size=1)
    reject('overlapping contextual data owners',lambda:validate_ownership(configuration,PE(target),good,[*context,duplicate]))
    bad=identify(target,{'_main':0x17001})
    assert any(r['name']=='_mainCRTStartup' and r['start']==0x2c260 for r in good['functions'])
    assert not any(r['name']=='_mainCRTStartup' and r['start']==0x2c260 for r in bad['functions'])
    print('rejected: CRT startup with incorrect relocated application target')
    binary_spec=[dict(name='record',size=4,pointer_fields=[0],arrays=[dict(offset=0,fixed_count=6,max_count=6,element_size=1,pointers=False,when=dict(offset=0,mask=0,value=0))])]
    old_binary={'record':[dict(address=0x100,bytes=(0x200).to_bytes(4,'little').hex(),arrays={'0':dict(address=0x200,bytes='060001020000')})]}
    new_binary=copy.deepcopy(old_binary);new_binary['record'][0]['address']=0x300;new_binary['record'][0]['bytes']=(0x400).to_bytes(4,'little').hex();new_binary['record'][0]['arrays']['0']['address']=0x400
    compare_walks(binary_spec,old_binary,new_binary)
    changed=copy.deepcopy(new_binary);changed['record'][0]['arrays']['0']['bytes']='060001020001'
    reject('binary record differing trailing NUL byte',lambda:compare_walks(binary_spec,old_binary,changed))
    changed=copy.deepcopy(new_binary);changed['record'][0]['arrays']['0']['bytes']='0600'
    reject('binary record truncated after embedded NUL',lambda:compare_walks(binary_spec,old_binary,changed))
    config,original=load()
    from pdb_checks import check as pdb_checks
    pdb_checks(config,original,reject)
    from symbol_output_checks import check_append_buffer
    append_pair=BINARY/'cases/hello-debug-dll/evidence/0074-save_type'
    if append_pair.is_dir():check_append_buffer(append_pair)
    if any(r['name']=='finish_recursive_types'and r['status']!='stub'for r in config['functions']):
        from runpy import run_path
        run_path(str(BINARY/'scripts/check-native-guards.py'))['check']()
    from find_named_checks import check as check_named_output
    named_pair=BINARY/'cases/hello-debug-dll/evidence/0076-finish_recursive_types'
    if named_pair.is_dir():check_named_output(named_pair)
    global_pair=BINARY/'cases/hello-debug-dll/evidence/0077-find_named_type'
    if global_pair.is_dir():
        check_named_output(global_pair,function='find_global_type')
        from named_graph_checks import check as check_named_graph
        check_named_graph(config,original,reject)
    local_pair=BINARY/'cases/hello-debug-dll/evidence/0078-find_global_type'
    if local_pair.is_dir():
        check_named_output(local_pair,function='find_local_type')
        from named_graph_checks import check_global as check_global_graph
        check_global_graph(config,original,reject)
    global_cache_pair=BINARY/'cases/hello-debug-dll/evidence/0079-find_local_type'
    if global_cache_pair.is_dir():
        from global_type_cache_checks import check as check_global_cache
        check_global_cache(global_cache_pair)
    module_type_pair=BINARY/'cases/hello-debug-dll/evidence/0080-record_derived_types'
    if module_type_pair.is_dir():
        from module_type_checks import check as check_module_types
        check_module_types(module_type_pair)
    resolver_pair=BINARY/'cases/hello-debug-dll/evidence/0081-convert_global_data32'
    if resolver_pair.is_dir():
        check_named_output(resolver_pair,function='get_type_record')
        from resolver_checks import check as check_resolver
        check_resolver(resolver_pair)
    resolver_return_pair=BINARY/'cases/hello-debug-dll/evidence/0082-get_type_record'
    if resolver_return_pair.is_dir():
        from resolver_return_checks import check as check_resolver_returns
        check_resolver_returns(resolver_return_pair)
    recursive_offset_pair=BINARY/'cases/hello-debug-dll/evidence/0083-add_recursive_offset'
    if recursive_offset_pair.is_dir():
        from recursive_offset_checks import check as check_recursive_offset
        check_recursive_offset(recursive_offset_pair)
    recursive_intern_pair=BINARY/'cases/hello-debug-dll/evidence/0084-intern_type'
    if recursive_intern_pair.is_dir():
        from recursive_intern_checks import check as check_recursive_intern
        check_recursive_intern(recursive_intern_pair)
    recursive_hash_pair=BINARY/'cases/hello-debug-dll/evidence/0085-hash_recursive_type'
    if recursive_hash_pair.is_dir():
        from recursive_hash_checks import check as check_recursive_hash
        check_recursive_hash(recursive_hash_pair)
    recursive_publish_pair=BINARY/'cases/hello-debug-dll/evidence/0086-commit_recursive_type'
    if recursive_publish_pair.is_dir():
        from recursive_publish_checks import check as check_recursive_publish
        check_recursive_publish(recursive_publish_pair)
    recursive_arena_pair=BINARY/'cases/hello-debug-dll/evidence/0087-publish_recursive_type'
    if recursive_arena_pair.is_dir():
        from recursive_arena_checks import check as check_recursive_arena
        check_recursive_arena(recursive_arena_pair)
    recursive_pointer_pair=BINARY/'cases/hello-debug-dll/evidence/0088-hash_referenced_type'
    if recursive_pointer_pair.is_dir():
        from recursive_pointer_checks import check as check_recursive_pointer
        check_recursive_pointer(recursive_pointer_pair)
    packed_publication_pair=BINARY/'cases/hello-debug-dll/evidence/0089-publish_packed_type'
    if packed_publication_pair.is_dir():
        from packed_publication_checks import check as check_packed_publication
        check_packed_publication(packed_publication_pair)
    procedure_pair=BINARY/'cases/hello-debug-dll/evidence/0090-convert_procedure32'
    if procedure_pair.is_dir():
        from procedure_checks import check as check_procedure
        check_procedure(procedure_pair)
    module_address_pair=BINARY/'cases/hello-debug-dll/evidence/0091-address_in_module'
    if module_address_pair.is_dir():
        from module_address_checks import check as check_module_address
        check_module_address(module_address_pair)
    bp_relative_pair=BINARY/'cases/hello-debug-dll/evidence/0092-convert_bp_relative32'
    if bp_relative_pair.is_dir():
        from bp_relative_checks import check as check_bp_relative
        check_bp_relative(bp_relative_pair)
    register_symbol_pair=BINARY/'cases/hello-debug-dll/evidence/0093-convert_register_symbol'
    if register_symbol_pair.is_dir():
        from register_symbol_checks import check as check_register_symbol
        check_register_symbol(register_symbol_pair)
    completed_symbols_pair=BINARY/'cases/hello-debug-dll/evidence/0094-end_symbol_block'
    if completed_symbols_pair.is_dir():
        from completed_symbols_checks import check as check_completed_symbols
        check_completed_symbols(completed_symbols_pair)
    from buffer_storage_checks import check as check_buffer_storage
    buffer_directory=BINARY/'cases/hello-debug-dll/evidence/0075-append_buffer'
    if buffer_directory.is_dir():check_buffer_storage(buffer_directory)
    if named_pair.is_dir():
        from buffer_storage_checks import check_extension
        check_extension(named_pair)
    from type_pages_checks import check as check_type_pages
    pages=BINARY/'cases/hello-debug-dll/evidence/0074-save_type'
    if pages.is_dir():check_type_pages(pages)
    from type_hash_checks import check as check_type_hash_tables
    check_type_hash_tables(config,original,reject)
    from symbol_output_checks import check_saved_type
    saved_symbols=BINARY/'cases/hello-debug-dll/evidence/0073-hash_type'
    if saved_symbols.is_dir():check_saved_type(saved_symbols)
    from symbol_output_checks import check_type_hash
    hashed_symbols=BINARY/'cases/hello-debug-dll/evidence/0072-store_type'
    if hashed_symbols.is_dir():check_type_hash(hashed_symbols)
    from symbol_output_checks import check_stored_list
    stored_symbols=BINARY/'cases/hello-debug-dll/evidence/0071-compact_type_list'
    if stored_symbols.is_dir():check_stored_list(stored_symbols)
    from symbol_output_checks import check_type_list
    list_symbols=BINARY/'cases/hello-debug-dll/evidence/0070-compact_pointer'
    if list_symbols.is_dir():check_type_list(list_symbols)
    from symbol_output_checks import check_pointer
    pointer_symbols=BINARY/'cases/hello-debug-dll/evidence/0069-compact_new_type'
    if pointer_symbols.is_dir():check_pointer(pointer_symbols)
    from symbol_output_checks import check_compact
    compact_symbols=BINARY/'cases/hello-debug-dll/evidence/0068-intern_global'
    if compact_symbols.is_dir():check_compact(compact_symbols)
    from symbol_output_checks import check_nested_udt
    nested_symbols=BINARY/'cases/hello-debug-dll/evidence/0067-convert_udt_symbol'
    if nested_symbols.is_dir():check_nested_udt(nested_symbols)
    from symbol_output_checks import check_object_copy
    copied=BINARY/'cases/hello-debug-dll/evidence/0066-convert_object_name'
    if copied.is_dir():check_object_copy(copied)
    from symbol_output_checks import check_search_write
    search=BINARY/'cases/hello-debug-dll/evidence/0065-write_search_symbol'
    if search.is_dir():check_search_write(search)
    from symbol_output_checks import check as check_symbol_output
    packed=BINARY/'cases/hello-debug-dll/evidence/0064-pack_new_types'
    if packed.is_dir():check_symbol_output(packed)
    from callback_table import describe as callback_description,check_rebuilt as check_callbacks
    converters=next((r for r in context if r['name']=='type_converters'),None)
    if converters:
        symbols=publics();raw=PE(target).read(converters['original'],8);new=PE(BINARY/'build/CVPACK.EXE').read(symbols[converters['symbol']],8)
        check_callbacks(converters,raw,new,configuration['functions'],symbols)
        reject('type converter table reversed targets',lambda:check_callbacks(converters,raw,new[4:]+new[:4],configuration['functions'],symbols))
        reject('type converter table truncated',lambda:check_callbacks(converters,raw,new[:4],configuration['functions'],symbols))
        reject('type converter unreviewed stride',lambda:callback_description(dict(converters,stride=8),raw,configuration['functions']))
    table=next((r for r in context if r.get('kind')=='callback-table'),None)
    if table:
        original_table=PE(target).read(table['original'],table['size']);symbols=publics();rebuilt_table=PE(BINARY/'build/CVPACK.EXE').read(symbols[table['symbol']],table['size'])
        check_callbacks(table,original_table,rebuilt_table,configuration['functions'],symbols)
        altered=bytearray(rebuilt_table);struct.pack_into('<I',altered,4,symbols['_end_symbol_block'])
        reject('callback table redirected to another known function',lambda:check_callbacks(table,original_table,altered,configuration['functions'],symbols))
        altered=bytearray(rebuilt_table);altered[0]^=1
        reject('callback table changed symbol kind',lambda:check_callbacks(table,original_table,altered,configuration['functions'],symbols))
        altered=bytearray(original_table);struct.pack_into('<I',altered,4,1)
        reject('callback table unknown original target',lambda:callback_description(table,altered,configuration['functions']))
    from type_cache_checks import check as check_cache,compiled_checks as check_cache_compiler
    cached=BINARY/'cases/hello-debug-dll/evidence/0061-index_new_types'
    if cached.is_dir():check_cache(cached);check_cache_compiler()
    from native_line_rebind import validate_native_fields
    historical=BINARY/'cases/hello-debug-dll/evidence/0060-convert_new_type/native-line-rebind'
    if historical.is_dir():
        before=json.loads((historical/'old-read-proof.json').read_text());after=json.loads((historical/'new-read-proof.json').read_text())
        validate_native_fields(before,after)
        for field in ('call','returned','owner','size'):
            changed=copy.deepcopy(after);changed['sites'][0][field]='wrong'if field=='owner'else changed['sites'][0][field]+1
            reject('native-line rebind changes original '+field,lambda:validate_native_fields(before,changed))
    from read_progress_checks import check as progress_checks,check_nested,check_types
    progress_checks(config,original,reject)
    check_nested(config,original,reject)
    check_types(config,original,reject)
    from stack_output import compare as compare_output,validate as validate_output
    outputs=next(r for r in config['functions']if r['name']=='read_file')['parameters']
    output=next(r for r in outputs if r['name']=='buffer')
    if output['kind']=='stack-output':
        proof=json.loads((BINARY/output['evidence']).read_text())
        words={0:proof['return_slots'][0]['address'],1:0,2:0x1000+proof['esp_offset'],3:64}
        parameters=dict(handle=0,size=64,buffer=dict(null=False,address=0x4000,local_address=0x4000,local=dict(owner='read_image_debug',name='dos',size=64,offset=0),prebytes='cc'*64))
        events='registers eax 00000000 esp 00001000 eip 0002a401\noutput-buffer %08x '%words[2]+'00'*64+'\n'
        compare_output(output,outputs,words,parameters,events,original)
        changed=copy.deepcopy(parameters);changed['buffer']['address']+=4
        reject('stack output native raw pointer differs from local',lambda:compare_output(output,outputs,words,changed,events,original))
        changed=copy.deepcopy(parameters);changed['buffer']['local']['name']='file'
        reject('stack output points to different native local',lambda:compare_output(output,outputs,words,changed,events,original))
        changed=dict(words);changed[2]+=4
        reject('stack output original pointer differs from reviewed local',lambda:compare_output(output,outputs,changed,parameters,events,original))
        changed=dict(words);changed[0]+=1
        reject('stack output unknown caller return',lambda:compare_output(output,outputs,changed,parameters,events,original))
        changed=copy.deepcopy(parameters);changed['size']=65
        reject('stack output count exceeds typed local',lambda:compare_output(output,outputs,words,changed,events,original))
        changed=copy.deepcopy(output);changed['evidence_sha256']='0'*64
        reject('stack output stale frame/callsite proof',lambda:validate_output(changed,original))
    description=next(r for r in config['functions']if r['name']=='open_buffered_file')['parameters']
    permission=next(r for r in description if r['name']=='permission')
    if permission.get('inactive_when'):
        assert compare_integer(permission,description,{2:0x8002,3:0},dict(mode=0x8002,permission=0xdeadbeef))
        reject('differing active create permissions',lambda:compare_integer(permission,description,{2:0x8102,3:0},dict(mode=0x8102,permission=0x80)))
        reject('mismatched conditional ABI mode',lambda:compare_integer(permission,description,{2:0x8002,3:0},dict(mode=0x8000,permission=0)))
    from graph_identities import read as read_identities,payload_sha
    identity_path=BINARY/'build/graph-identities.json'
    if identity_path.exists():
        identity=json.loads(identity_path.read_text())
        with tempfile.TemporaryDirectory(prefix='identity-negatives-',dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as directory:
            modified=Path(directory)/'identities.json'
            for field in ('original_sha256','rebuilt_sha256','archive_sha256','functions_sha256','map_sha256','runtime_anchors_sha256','runtime_index_sha256'):
                changed=copy.deepcopy(identity);changed['provenance'][field]='0'*64;changed['identity_payload_sha256']=payload_sha(changed);modified.write_text(json.dumps(changed))
                reject('stale identity cache '+field,lambda:read_identities(modified,BINARY/'build/CVPACK.EXE'))
            changed=copy.deepcopy(identity);changed['rebuilt_runtime']['functions'][0]['name']='wrong_identity';modified.write_text(json.dumps(changed))
            reject('mutated identity payload',lambda:read_identities(modified,BINARY/'build/CVPACK.EXE'))
    context=json.loads((BINARY/'data.json').read_text())
    conditional=approved(config,original,good)
    if conditional:check_guard_references()
    for record in conditional:
        name=record['guard']['name'];mode=record['guard']['mode'];access=record['access'];hexname=name.encode('latin1').hex()
        sample=('crt-entry 0002dcb0 %08x 00000040 00000000 %s\n'%(mode,hexname)+
            'crt-restored 0002dcb0 0002e0a0\ncrt-rearm 0002dcb0 0002dcb3\n'+
            'crt-createfile 0002dcb0 access %08x share 3 disposition 3 attrs 128 inherit 1 name %s\n'%(access,hexname)+
            'crt-createfile-rearm 0002dcb0 0002be52\ncrt-return 0002dcb0 fd 3 errno 0 osfile 01 type 1 position 0 owned 1 input 1 last_error 0\n')
        observed=[dict(name=name,mode=mode,share=0x40,permission=438,
            createfile=dict(access=access,share=3,disposition=3,attrs=128,inherit=False,name=name),
            **{'return':dict(descriptor=3,errno=0,last_error=0,osfile=1,type=1,position=0,owned=True,input_matches=True,input_sha256=record['input']['sha256'],access_mode=0 if mode==0x8000 else 2)})]
        compare_returns(sample,observed,[record])
        changed=copy.deepcopy(observed);changed[0]['return']['input_matches']=False
        reject('both conditional CRT input observations false',lambda:compare_returns(sample.replace('input 1','input 0'),changed,[record]))
        changed=copy.deepcopy(observed);changed[0]['return']['input_sha256']='0'*64
        reject('conditional CRT native input hash mismatch',lambda:compare_returns(sample,changed,[record]))
        changed=copy.deepcopy(observed);changed[0]['return']['last_error']=1
        reject('conditional CRT LastError differs',lambda:compare_returns(sample,changed,[record]))
        changed=copy.deepcopy(observed);changed[0]['createfile']['access']^=0x40000000
        reject('conditional CRT actual access differs',lambda:compare_returns(sample,changed,[record]))
        changed=copy.deepcopy(observed);changed[0]['createfile']['disposition']=2
        reject('conditional CRT actual open disposition differs',lambda:compare_returns(sample,changed,[record]))
        changed=copy.deepcopy(observed);changed[0]['return']['access_mode']=1
        reject('conditional CRT native descriptor access differs',lambda:compare_returns(sample,changed,[record]))
        reject('conditional CRT missing actual CreateFile observation',lambda:compare_returns('\n'.join(line for line in sample.splitlines()if not line.startswith('crt-createfile')),observed,[record]))
        for label,field,value in [('conditional CRT text mode','mode',0x4002),('conditional CRT other share','share',0x10)]:
            changed=copy.deepcopy(record);changed['guard'][field]=value
            reject(label,lambda changed=changed:validate_seam(config,original,good,changed))
        changed=copy.deepcopy(record);changed['start']+=1
        reject('conditional CRT changed entry evidence',lambda:validate_seam(config,original,good,changed))
        changed=copy.deepcopy(record);changed['body_sha256']='0'*64
        reject('conditional CRT changed whole-body evidence',lambda:validate_seam(config,original,good,changed))
        changed=copy.deepcopy(record);changed['evidence_sha256']='0'*64
        reject('conditional CRT changed CFG/callee evidence',lambda:validate_seam(config,original,good,changed))
    if config.get('read_observations'):
        from reads import validate as validate_reads,compare as compare_reads
        sites=validate_reads(config,original);site=sites[0]
        sample=('read-entry %08x 0 00004000 4 0 0 3 01 4096 aabbccdd\nread-rearm %08x 0002a400\nread-return %08x 2 0 2 3 01 4096 0600ccdd\nread-rearm %08x %08x\n')%(site['call'],site['call'],site['call'],site['returned'],site['returned']+3)
        observed=[dict(site=site['call'],handle=0,address=0x5000,count=4,entry_errno=0,entry_position=0,descriptor=3,entry_osfile=1,entry_host_position=4096,prebytes='eeee1234',returned=2,errno=0,position=2,descriptor_return=3,osfile=1,host_position=4096,postbytes='06001234')]
        compare_reads(sample,observed,sites)
        reject('missing per-read observations',lambda:compare_reads('',[],sites))
        reject('missing read breakpoint rearm',lambda:compare_reads(sample.replace('read-rearm %08x 0002a400\n'%site['call'],''),observed,sites))
        changed=copy.deepcopy(observed);changed[0]['postbytes']='07001234'
        reject('defined read output differs',lambda:compare_reads(sample,changed,sites))
        changed=copy.deepcopy(observed);changed[0]['postbytes']='06001235'
        reject('short read altered undefined tail',lambda:compare_reads(sample,changed,sites))
        changed=copy.deepcopy(observed);changed[0]['returned']=3
        reject('read return count differs',lambda:compare_reads(sample,changed,sites))
        changed=copy.deepcopy(observed);changed[0]['host_position']+=1
        reject('read underlying descriptor position differs',lambda:compare_reads(sample,changed,sites))
    validate_ownership(config,original,good,context)
    ownership_mutations={}
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([0x1765c,0x176bc])
    ownership_mutations['duplicate text-data ownership']=changed
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([0x17000,0x17001])
    ownership_mutations['text-data overlaps another application body']=changed
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='cvpack_main')['extent'][1]=0x174c1
    ownership_mutations['application code includes another function entry']=changed
    changed=copy.deepcopy(config)
    stub=next(r for r in changed['functions']if r['status']=='stub')
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([stub['original'],stub['original']+1])
    ownership_mutations['text-data masks an unreconstructed function entry']=changed
    changed=copy.deepcopy(config)
    next(r for r in changed['functions']if r['name']=='parse_options')['text_data'].append([0x38cf8,0x38cfc])
    ownership_mutations['unmask outside original file-backed code']=changed
    changed=copy.deepcopy(config)
    changed['support_unmasked'].append(copy.deepcopy(changed['support_unmasked'][0]))
    ownership_mutations['duplicate runtime acceptance ownership']=changed
    for label,changed in ownership_mutations.items():
        reject(label,lambda changed=changed:validate_ownership(changed,original,good,context))
    # A runtime identity must never become an application owner, even when the
    # runtime catalogue itself also identifies those bytes.
    changed=copy.deepcopy(config)
    accepted=next(r for r in changed['functions']if r['status']=='accepted')
    runtime_record=dict(name='test-runtime',start=accepted['extent'][0],end=accepted['extent'][1])
    changed['support_unmasked'].append(runtime_record)
    synthetic=copy.deepcopy(good)
    synthetic['functions'].append(runtime_record)
    reject('runtime overlaps accepted application code',lambda:validate_ownership(changed,original,synthetic,context))
    changed_context=[*context,dict(name='unidentified-text-data',original=config['support_unmasked'][0]['start'],size=1)]
    reject('runtime unmask overlaps unrelated contextual data',lambda:validate_ownership(config,original,good,changed_context))
    # Exact same-range runtime aliases require explicit identity and evidence;
    # different extents and duplicate names never qualify as aliases.
    changed=copy.deepcopy(config)
    alias=copy.deepcopy(changed['support_unmasked'][0]);alias['name']='test-alias'
    changed['support_unmasked'].append(alias)
    synthetic=copy.deepcopy(good);synthetic['functions'].append(alias)
    reject('runtime alias without explicit declaration',lambda:validate_ownership(changed,original,synthetic,context))
    changed['runtime_aliases']=[dict(start=alias['start'],end=alias['end'],names=[config['support_unmasked'][0]['name'],alias['name']],evidence='cases/startup/proof.json')]
    validate_ownership(changed,original,synthetic,context)
    scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
    scratch.mkdir(exist_ok=True,parents=True)
    with tempfile.TemporaryDirectory(prefix='snapshot-rejections-',dir=scratch) as temporary:
        directory=Path(temporary)
        old=directory/'original';new=directory/'native';old.mkdir();new.mkdir()
        current=directory/'functions.json';current.write_bytes((BINARY/'functions.json').read_bytes())
        for destination in (old,new):(destination/'configuration.json').write_bytes(current.read_bytes())
        masked=old/'CVPACK.EXE';masked.write_bytes(b'captured masked image')
        executable=directory/'native.exe';executable.write_bytes(b'native executable')
        captured=dict(configuration_sha256=sha(current),original_sha256=sha(target),executable_sha256=sha(masked),debugger_sha256=sha(BINARY/'build/debug-run.exe'))
        native=dict(configuration_sha256=sha(current),original_sha256=sha(target),executable_sha256=sha(executable))
        def snapshot(a=captured,b=native):validate_snapshot(config,current,old,new,a,b,target,executable)
        snapshot()
        policy=directory/'runtime-support.json';policy.write_text('[]\n')
        first=snapshot_policy(old,policy);second=snapshot_policy(new,policy)
        check_policy_snapshot(old,first,policy);check_policy_snapshot(new,second,policy)
        policy.write_text('[]\n\n')
        reject('conditional support mutated during run',lambda:check_policy_snapshot(old,first,policy))
        second=snapshot_policy(new,policy)
        reject('stale original versus native conditional support',lambda:check_policy_snapshot(old,second,policy))
        stale=copy.deepcopy(config);stale['review']['snapshot-negative-test']=True
        for destination,receipt in ((old,captured),(new,native)):
            (destination/'configuration.json').write_text(json.dumps(stale))
            receipt['configuration_sha256']=sha(destination/'configuration.json')
        reject('two mutually matching stale configurations',snapshot)
        for destination,receipt in ((old,captured),(new,native)):
            (destination/'configuration.json').write_bytes(current.read_bytes())
            receipt['configuration_sha256']=sha(current)
        changed=dict(native,configuration_sha256='0'*64)
        reject('native/original configuration receipt mismatch',lambda:snapshot(b=changed))
        changed=dict(captured,original_sha256='0'*64)
        reject('captured original target hash mismatch',lambda:snapshot(a=changed))
        changed=dict(native,original_sha256='0'*64)
        reject('native original-target identity mismatch',lambda:snapshot(b=changed))
        masked.write_bytes(b'changed masked image')
        reject('captured masked image changed after execution',snapshot)
        masked.write_bytes(b'captured masked image')
        changed=dict(captured,debugger_sha256='0'*64)
        reject('captured original controller differs',lambda:snapshot(a=changed),'original capture controller changed')
        executable.write_bytes(b'changed native executable')
        reject('native executable changed after execution',snapshot)
    specifications=[dict(name='pool',original=0x4000,pointer_fields=[4])]
    left={'pool':[dict(address=0x1000,bytes=struct.pack('<III',7,0x1010,11).hex()),dict(address=0x1010,bytes=struct.pack('<III',9,0,13).hex())]}
    right={'pool':[dict(address=0x9000,bytes=struct.pack('<III',7,0x9010,11).hex()),dict(address=0x9010,bytes=struct.pack('<III',9,0,13).hex())]}
    compare_walks(specifications,left,right)
    changed={'pool':[dict(address=0x9000,bytes=struct.pack('<III',8,0x9010,11).hex()),right['pool'][1]]}
    reject('changed observed object byte',lambda:compare_walks(specifications,left,changed))
    changed={'pool':[dict(address=0x9000,bytes=struct.pack('<III',7,0x9000,11).hex()),right['pool'][1]]}
    reject('wrong observed object pointer identity',lambda:compare_walks(specifications,left,changed))
    changed={'pool':[dict(address=0x9000,bytes=struct.pack('<III',7,0xffff,11).hex()),right['pool'][1]]}
    reject('unresolved observed object pointer',lambda:compare_walks(specifications,left,changed))
    reject('unterminated original object list',lambda:original_walks('walk-end 00004000 0 00001000\n',specifications))
    from objects import pointer_identity
    empty_spec=[dict(name='empty',allocation_name='module_data')]
    empty_old={'empty':[dict(address=0x1000,bytes='')]};empty_new={'empty':[dict(address=0x9000,bytes='')]}
    definitions={'module_data':dict(size=0,defined_ranges=[])}
    compare_walks(empty_spec,empty_old,empty_new,definitions)
    assert pointer_identity(empty_old,0x1000)==(('empty',0),0)
    reject('zero allocation invented readable byte',lambda:pointer_identity(empty_old,0x1001))
    reject('scratch allocation missing ownership proof',lambda:compare_walks(empty_spec,empty_old,empty_new))
    reject('scratch allocation missing live object',lambda:compare_walks(empty_spec,{'empty':[]},{'empty':[]},definitions))
    changed={'module_data':dict(size=1,defined_ranges=[])}
    reject('scratch allocation wrong requested extent',lambda:compare_walks(empty_spec,empty_old,empty_new,changed))
    changed={'module_data':dict(size=0,defined_ranges=[[0,1]])}
    reject('scratch defined bytes lack reviewed write proof',lambda:compare_walks(empty_spec,empty_old,empty_new,changed))
    ambiguous={'a':[dict(address=0x1000,bytes='')],'b':[dict(address=0x1000,bytes='')]}
    reject('zero allocation ambiguous lifetime identity',lambda:pointer_identity(ambiguous,0x1000))
    if config.get('allocation_observations'):
        from allocations import validate as validate_allocations,context as allocation_context
        proof=json.loads((BINARY/config['allocation_observations']['evidence']).read_text())
        reject('scratch allocation unreviewed consumer frontier',lambda:allocation_context(config,original,'',{},dict(name='unknown',native='unknown'),'hello-debug-dll'))
        reject('scratch allocation unreviewed case',lambda:allocation_context(config,original,'',{},dict(name='pack_module',native='pack_module'),'other-case'))
        with tempfile.TemporaryDirectory(prefix='allocation-negatives-',dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as directory:
            evidence=Path(directory)/'allocation.json'
            changed=copy.deepcopy(proof);changed['allocations'][0]['original_callsite']['sha256']='0'*64;evidence.write_text(json.dumps(changed))
            modified=copy.deepcopy(config);modified['allocation_observations']=dict(evidence=str(evidence),evidence_sha256=sha(evidence))
            reject('scratch allocation changed creation instructions',lambda:validate_allocations(modified,original))
    from objects import resident_module_fields
    def resident():
        raw=bytearray(48);struct.pack_into('<H',raw,0,1);struct.pack_into('<II',raw,12,0x3000,24)
        return {'module_index':[dict(address=0x1000,bytes=struct.pack('<HHII',1,0,0,0x2000).hex())],
            'module_list':[dict(address=0x2000,bytes=raw.hex())],
            'directory_entries':[dict(address=0x4000,bytes=struct.pack('<HHII',0x120,1,0x3000,24).hex())]}
    assert resident_module_fields(resident())==[4]
    changed=resident();raw=bytearray.fromhex(changed['directory_entries'][0]['bytes']);struct.pack_into('<H',raw,0,0x121);changed['directory_entries'][0]['bytes']=raw.hex()
    reject('resident directory unknown union mode',lambda:resident_module_fields(changed))
    changed=resident();raw=bytearray.fromhex(changed['directory_entries'][0]['bytes']);struct.pack_into('<I',raw,4,0x3004);changed['directory_entries'][0]['bytes']=raw.hex()
    reject('resident directory wrong module record owner',lambda:resident_module_fields(changed))
    changed=resident();raw=bytearray.fromhex(changed['module_list'][0]['bytes']);struct.pack_into('<H',raw,0,2);changed['module_list'][0]['bytes']=raw.hex()
    reject('resident directory wrong module number',lambda:resident_module_fields(changed))
    register=dict(name='type',register='ecx',kind='integer',bits=16)
    assert compare_integer(register,[register],{'ecx':0xdead0040},{'type':0x40})is None
    reject('fastcall argument lowword differs',lambda:compare_integer(register,[register],{'ecx':0xdead0040},{'type':0x41}))
    from allocations import mask_undefined
    scratch_mask={'scratch':dict(size=4,defined_ranges=[[0,2]])}
    left,right=bytearray(b'\x01\x02aa'),bytearray(b'\x01\x02bb');mask_undefined('scratch',left,right,scratch_mask);assert left==right==b'\x01\x02\0\0'
    reject('defined scratch payload byte differs',lambda:mask_undefined('scratch',bytearray(b'\x01\x02aa'),bytearray(b'\x01\x03bb'),scratch_mask))
    reject('defined scratch write exceeds allocation',lambda:mask_undefined('scratch',bytearray(4),bytearray(4),{'scratch':dict(size=4,defined_ranges=[[3,5]])}))
    inputs=next(r for r in config['functions']if r['name']=='intern_public')['parameters']if any(r['name']=='intern_public'for r in config['functions'])else []
    input_buffer=next((r for r in inputs if r.get('kind')=='stack-input'),None)
    if input_buffer:
        from stack_output import compare as compare_buffer
        proof=json.loads((BINARY/input_buffer['evidence']).read_text());words={0:proof['return_slots'][0]['address'],1:0x1000+proof['esp_offset']}
        raw='02000300'+'aa'*508
        events='registers eax 00000000 esp 00001000 eip 0001b311\ninput-buffer %08x '%words[1]+raw+'\n'
        arguments={'record':dict(null=False,address=0x5000,local_address=0x5000,local=dict(owner=proof['owner'],name=proof['local'],size=512,offset=0),prebytes='02000300'+'bb'*508)}
        compare_buffer(input_buffer,inputs,words,arguments,events,original)
        changed=copy.deepcopy(arguments);changed['record']['prebytes']='02000400'+'bb'*508
        reject('defined caller stack input byte differs',lambda:compare_buffer(input_buffer,inputs,words,changed,events,original))
        changed=copy.deepcopy(arguments);changed['record']['prebytes']='ff7f0300'+'bb'*508
        reject('caller stack input length exceeds object',lambda:compare_buffer(input_buffer,inputs,words,changed,events,original))
    from public_arena import layout as arena_layout,comparison as arena_compare,validate as validate_arena
    arena=bytearray(8100);struct.pack_into('<IHHIIH',arena,0,0,12,0,0x1234,17,1)
    struct.pack_into('<HH',arena,18,30,0x203);arena[30]=3;arena[31:34]=b'foo'
    heads=bytearray(16384);struct.pack_into('<I',heads,0x234*4,0x1000)
    arena_layout(arena,0x1000,8048,heads,1,32)
    def arena_reject(label,offset,value,width=4):
        changed=bytearray(arena);changed[offset:offset+width]=value.to_bytes(width,'little')
        reject(label,lambda:arena_layout(changed,0x1000,8048,heads,1,32))
    arena_reject('public arena cyclic node',0,0x1000)
    arena_reject('public arena outside node ownership',0,0x3000)
    arena_reject('public arena truncated record extent',18,200,2)
    arena_reject('public arena invalid name offset',4,100,2)
    arena_reject('public arena wrong bucket hash',8,0x1235)
    reject('public arena incomplete used allocation',lambda:arena_layout(arena,0x1000,8044,heads,1,32))
    reject('public arena negative available bytes',lambda:arena_layout(arena,0x1000,-1,heads,1,32))
    reject('public arena count differs',lambda:arena_layout(arena,0x1000,8048,heads,2,32))
    reject('public arena byte total differs',lambda:arena_layout(arena,0x1000,8048,heads,1,33))
    changed_heads=bytearray(heads);struct.pack_into('<I',changed_heads,0x235*4,0x1000)
    reject('public arena duplicate bucket ownership',lambda:arena_layout(arena,0x1000,8048,changed_heads,1,32))
    # Same allocator block, independent roots/counts: both families must fully
    # partition the used region, and no family may borrow the other's node.
    shared=bytearray(arena);shared[52:104]=arena[:52]
    globals_head=bytearray(16384);struct.pack_into('<I',globals_head,0x234*4,0x1034)
    family=dict(name='global',heads=globals_head,count=1,total=32)
    arena_layout(shared,0x1000,7996,heads,1,32,[family])
    reject('shared symbol global family omitted',lambda:arena_layout(shared,0x1000,7996,heads,1,32))
    reject('shared symbol duplicated family ownership',lambda:arena_layout(shared,0x1000,7996,heads,1,32,[dict(family,heads=heads)]))
    reject('shared symbol wrong independent count',lambda:arena_layout(shared,0x1000,7996,heads,1,32,[dict(family,count=2)]))
    reject('shared symbol wrong independent bytes',lambda:arena_layout(shared,0x1000,7996,heads,1,32,[dict(family,total=33)]))
    reject('shared symbol unknown family',lambda:arena_layout(shared,0x1000,7996,heads,1,32,[dict(family,name='unidentified')]))
    changed_heads=bytearray(globals_head);struct.pack_into('<I',changed_heads,0x234*4,0x1038)
    reject('shared symbol shifted global owner',lambda:arena_layout(shared,0x1000,7996,heads,1,32,[dict(family,heads=changed_heads)]))
    # Uninitialized reserved/tail bytes remain raw evidence; defined payload bytes
    # are still compared after masking and typed pointer normalization.
    changed=bytearray(arena);changed[6:8]=b'xy';changed[50:52]=b'zz';changed[100:103]=b'uvw'
    context=dict(original_available=8048,native_available=8048,original_heads=heads,native_heads=heads,count=1,total=32)
    left,right=bytearray(arena),bytearray(changed)
    arena_compare(left,right,dict(address=0x1000,bytes=arena.hex()),dict(address=0x1000,bytes=changed.hex()),context);assert left==right
    def compare_arena_bytes(a,b):
        left,right=bytearray(a),bytearray(b)
        arena_compare(left,right,dict(address=0x1000,bytes=a.hex()),dict(address=0x1000,bytes=b.hex()),context)
        assert left==right,'defined public record differs'
    changed[31]^=1
    reject('public arena defined record differs',lambda:compare_arena_bytes(arena,changed))
    changed=copy.deepcopy(config);changed['public_arena_observations']['evidence_sha256']='0'*64
    reject('public arena stale source/creation proof',lambda:validate_arena(changed,original))
    from transient import compare as compare_transient,validate as validate_transient
    constructor=next(r for r in config['functions']if r['name']=='type_server_open_file')['parameters'][0]
    def transient_state(base,fill):
        raw=bytearray([fill]*116)
        raw[64:96]=b'\0'*32;raw[100:104]=b'\xff'*4
        children={}
        for offset,child in [(72,base+0x1000),(84,base+0x2000)]:
            struct.pack_into('<II',raw,offset,child,child)
            children[str(offset)]=dict(address=child,bytes=struct.pack('<III',0,child+12,0).hex())
        return dict(null=False,address=base,owner_pointer=base,owner=dict(function='type_server_open',local='server',size=116),prebytes=raw.hex(),children=children)
    old,new=transient_state(0x1000,0xaa),transient_state(0x5000,0xbb)
    events='input-buffer %08x %s\n'%(old['address'],old['prebytes'])
    for offset,child in old['children'].items():events+='transient-child %08x %s %08x %s\n'%(old['address'],offset,child['address'],child['bytes'])
    words={0:0x2ba39,1:old['address'],'esi':old['address']}
    compare_transient(constructor,words,dict(server=new),events,original)
    changed=copy.deepcopy(new);changed['owner_pointer']+=4
    reject('transient object differs from constructor local',lambda:compare_transient(constructor,words,dict(server=changed),events,original))
    changed=copy.deepcopy(new);changed['owner']['size']=120
    reject('transient object requested size differs',lambda:compare_transient(constructor,words,dict(server=changed),events,original))
    changed=copy.deepcopy(new);raw=bytearray.fromhex(changed['prebytes']);raw[64]=1;changed['prebytes']=raw.hex()
    reject('transient initialized null field differs',lambda:compare_transient(constructor,words,dict(server=changed),events,original))
    changed=copy.deepcopy(new);changed['children']['72']['bytes']=struct.pack('<III',0,0x600d,0).hex()
    reject('transient child cursor exceeds explicit self end',lambda:compare_transient(constructor,words,dict(server=changed),events,original))
    changed=copy.deepcopy(new);changed['children']['72']['address']=new['address']
    reject('transient child aliases parent allocation',lambda:compare_transient(constructor,words,dict(server=changed),events,original))
    changed=copy.deepcopy(new);changed['children']['84']=copy.deepcopy(changed['children']['72'])
    reject('transient children alias distinct creations',lambda:compare_transient(constructor,words,dict(server=changed),events,original))
    changed=dict(words);changed[0]+=1
    reject('transient object unknown caller return',lambda:compare_transient(constructor,changed,dict(server=new),events,original))
    changed=dict(words);changed['esi']+=4
    reject('transient original constructor register differs',lambda:compare_transient(constructor,changed,dict(server=new),events,original))
    changed=copy.deepcopy(constructor);changed['evidence_sha256']='0'*64
    reject('transient constructor source proof stale',lambda:validate_transient(changed,original))
    required=next(r for r in config['functions']if r['name']=='type_server_open_file')['required_conditional_calls_by_case']['hello-debug-dll']
    reject('both sides omit all mandatory PDB CRT invocations',lambda:compare_returns('',[],conditional,required))
    # Reuse the first actual successful DLL invocation from the new frozen controls
    # and its matching native record, then remove only the PDB pair.
    pair=BINARY/'cases/hello-debug-dll/evidence/0047-type_server_open_file'
    old=(pair/'original-events').read_text()
    records=json.loads((pair/'native-crt-returns.json').read_text())
    lines=[];seen=0
    for line in old.splitlines():
        if line.startswith('crt-entry '):seen+=1
        if line.startswith('crt-')and seen==1:lines.append(line)
    reject('both sides omit only mandatory PDB CRT invocation',lambda:compare_returns('\n'.join(lines),records[:1],conditional,required))
    from inputs import prepare as prepare_extra,verify as verify_extra,environment as extra_environment
    record=json.loads((BINARY/'cases/hello-debug-dll/case.json').read_text())
    with tempfile.TemporaryDirectory(prefix='ancillary-rejections-',dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as directory:
        work=Path(directory)/'work';work.mkdir();prepare_extra(record,work);env={};extra_environment(record,work,env)
        files=list(Path(env['RECON_DRIVE_Z']).rglob('msvc.pdb'));assert len(files)==1
        files[0].write_bytes(files[0].read_bytes()+b'x')
        reject('ancillary frozen PDB altered after preparation',lambda:verify_extra(record,work))
        files[0].unlink()
        reject('ancillary frozen PDB missing',lambda:verify_extra(record,work))
    from accept import compare_literal
    literal=dict(original=0x3780c,bytes='726300');actual=dict(null=False,string='7263')
    compare_literal(literal,0x3780c,actual,original)
    reject('literal ABI points to different original object',lambda:compare_literal(literal,0x3781c,actual,original))
    reject('literal ABI native bytes differ',lambda:compare_literal(literal,0x3780c,dict(null=False,string='7763'),original))
    segment=next(r for r in next(f for f in config['functions']if f['name']=='hash_public')['parameters']if r['name']=='segment')
    proof=json.loads((BINARY/segment['evidence']).read_text());words={0:0x1b348,4:0x1000+proof['esp_offset']}
    arguments=dict(segment=dict(null=False,address=0x4000,local_address=0x4000,local=dict(owner='intern_public',name='segment',size=2,offset=0),prebytes='0100'))
    events='registers eax 00000000 esp 00001000 eip 0001afe1\noutput-buffer %08x 0100\n'%words[4]
    reject('both initialized stack output values differ from actual zero write',lambda:compare_output(segment,[segment],words,arguments,events,original))
    latest=next(entry for entry in reversed(config['accepted_order'])if entry['case']=='hello-debug-dll')
    owner=next(record for record in config['functions']if record['name']==latest['function'])
    pair=(BINARY/owner['evidence']).parent
    events=(pair/'original-events').read_text()
    state=json.loads((pair/'native-gdb.json').read_text());native_reads=json.loads((pair/'native-read-returns.json').read_text())
    filtered='\n'.join(line for line in events.splitlines()if not(line.startswith(('read-entry 00017e69 ','read-return 00017e69 ','read-rearm 00017e69 ','read-rearm 00017e6e '))))
    from allocations import context as allocation_context
    next_name=json.loads((pair/'proof.json').read_text())['native_frontier']
    frontier=next(r for r in config['functions']if r['native']==next_name)
    from read_progress import compare as compare_progress
    progress=json.loads((pair/'native-read-progress.json').read_text())if (pair/'native-read-progress.json').exists()else []
    partial=compare_progress(config,original,events,progress,frontier,state)
    allocation_context(config,original,events,state,frontier,'hello-debug-dll',native_reads,partial)
    try:allocation_context(config,original,filtered,state,frontier,'hello-debug-dll',[r for r in native_reads if r['site']!=0x17e69],partial)
    except AssertionError as error:assert str(error)=='scratch consumer reads before initialization',error
    else:raise AssertionError('accepted missing actual defining public read')
    print('rejected: scratch consumer has no defining public read on either side')
    # Buffered FileSlot unions: capacity is an integer, the page table contains
    # explicit unloaded-page tag1, and name pointers own exact bounded strings.
    description=dict(name='files',original=0x4000,pointer_fields=[0,20,32,36],string_fields=[0],
        mode_guard=dict(offset=4,mask=8,value=0),arrays=[dict(offset=20,count_offset=28,count_shift=12,max_count=16,when=dict(offset=4,mask=8,value=0),sentinels=[1])])
    def file_state(base):
        raw=struct.pack('<10I',base+0x1000,3,15,4096,3,base+0x2000,0x200000,8192,0,0)
        return dict(files=[dict(address=base,bytes=raw.hex(),strings={'0':dict(address=base+0x1000,bytes=b'hello-debug.dll\0'.hex())},arrays={'20':dict(address=base+0x2000,bytes=struct.pack('<II',1,0).hex())})])
    left,right=file_state(0x1000),file_state(0x9000)
    compare_walks([description],left,right)
    changed=copy.deepcopy(right);changed['files'][0]['strings']['0']['bytes']=b'wrong-name.dll\0'.hex()
    reject('changed allocated filename bytes',lambda:compare_walks([description],left,changed))
    changed=copy.deepcopy(right);changed['files'][0]['arrays']['20']['bytes']=struct.pack('<II',0,0).hex()
    reject('changed buffered page-table tag',lambda:compare_walks([description],left,changed))
    changed=copy.deepcopy(right);raw=bytearray.fromhex(changed['files'][0]['bytes']);struct.pack_into('<I',raw,4,11);changed['files'][0]['bytes']=raw.hex()
    reject('mapped union enters buffered observer',lambda:compare_walks([description],left,changed))
    description['file_descriptor']=True
    descriptor=dict(descriptor=3,osfile=1,type=1,position=4096,owned=True)
    for state in (left,right):state['files'][0]['descriptors']={'16':copy.deepcopy(descriptor)}
    compare_walks([description],left,right)
    changed=copy.deepcopy(right);changed['files'][0]['descriptors']['16']['position']=0
    reject('changed opened file position',lambda:compare_walks([description],left,changed))
    changed_left,changed_right=copy.deepcopy(left),copy.deepcopy(right)
    for state in (changed_left,changed_right):state['files'][0]['descriptors']['16']['owned']=False
    reject('both opened descriptor owners false',lambda:compare_walks([description],changed_left,changed_right))
    for state in (changed_left,changed_right):del state['files'][0]['descriptors']
    reject('both opened descriptor proofs missing',lambda:compare_walks([description],changed_left,changed_right))
    anchors=good['callee_identity_anchors']
    if anchors:
        scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
        scratch.mkdir(exist_ok=True,parents=True)
        with tempfile.TemporaryDirectory(prefix='anchor-mask-',dir=scratch) as directory:
            image=Path(directory)/'exact-closure.exe'
            # Even unmasking EVERY exact runtime/import identity must leave a
            # reviewed semantic variant entirely trapped, with no coverage gain.
            closure=dict(functions=[],support_unmasked=good['functions']+good['imports'])
            pe=PE(target)
            mask(closure,pe,image)
            masked=PE(image)
            for anchor in anchors:
                assert not any(r['start']<=anchor['start']<r['end'] for r in good['functions']+good['imports'])
                assert masked.read(anchor['start'],anchor['end']-anchor['start'])==b'\xcc'*(anchor['end']-anchor['start'])
            for record in conditional:
                assert masked.read(record['start'],record['end']-record['start'])==b'\xcc'*(record['end']-record['start'])
            if conditional:print('rejected: conditional variant unmask through ordinary exact runtime closure')
        print('rejected: reviewed variant unmask through exact runtime closure')
    # Mutate actual compiler output, leaving originals/source/build products intact.
    rebuilt=BINARY/'build/CVPACK.EXE'
    if rebuilt.is_file():
        pe=PE(rebuilt)
        symbols=publics()
        start=symbols['_main']
        end=min(address for address in symbols.values() if address>start)
        instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(start,end-start),start))
        call=next(i for i in instructions if i.mnemonic=='call' and i.op_str=='0x%x'%symbols['_parse_options'])
        # Change only the immediate of an existing callee-cleanup RET. Expanding
        # a one-byte C3 would overwrite live following code in packed layouts.
        terminal=None
        for function in config['functions']:
            if function['status']=='stub'or not function['stack_cleanup']:continue
            a=symbols[function['symbol']]
            b=min(address for address in symbols.values()if address>a)
            for instruction in Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.read(a,b-a),a):
                if instruction.mnemonic=='ret'and instruction.size==3 and instruction.bytes[0]==0xc2:
                    assert int.from_bytes(instruction.bytes[1:],'little')==function['stack_cleanup']
                    terminal=instruction;break
            if terminal:break
        assert terminal,'no existing callee-cleanup RET for ABI mutation'
        mutations={}
        image=bytearray(pe.data)
        struct.pack_into('<i',image,pe.offset(call.address)+1,symbols['_print_banner']-call.address-5)
        mutations['redirected application callee']=image
        image=bytearray(pe.data)
        cleanup=int.from_bytes(terminal.bytes[1:],'little')
        struct.pack_into('<H',image,pe.offset(terminal.address)+1,cleanup+4)
        mutations['changed callee stack cleanup']=image
        image=bytearray(pe.data)
        image[pe.offset(symbols['_show_logo'])]^=1
        mutations['changed contextual global initializer']=image
        if 'type_hash_observations'in config:
            image=bytearray(pe.data);image[pe.offset(symbols['_type_hash_byte'])+1]^=1
            mutations['changed reached type hash lookup table']=image
        from module_write import stores
        start=symbols['_read_types'];end=min(v for v in symbols.values()if v>start)
        write=stores(pe,start,end,symbols['_module_type_buffer'])[0]
        assert write.bytes[0]==0xa3
        image=bytearray(pe.data)
        struct.pack_into('<I',image,pe.offset(write.address)+1,symbols['_current_type_map'])
        mutations['wrong module buffer publication target']=image
        scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
        scratch.mkdir(exist_ok=True,parents=True)
        with tempfile.TemporaryDirectory(prefix='audit-rejections-',dir=scratch) as directory:
            for label,image in mutations.items():
                modified=Path(directory)/'modified.exe'
                modified.write_bytes(image)
                from graph_identities import prepare as prepare_identities
                identity_cache=Path(directory)/'identities.json'
                prepare_identities(identity_cache,modified)
                result=subprocess.run(['python3',str(BINARY/'scripts/audit.py'),'--canonical-only','--rebuilt',str(modified),'--identities',str(identity_cache)],capture_output=True)
                assert result.returncode!=0,'accepted '+label
                assert b'AssertionError' in result.stderr,result.stderr
                assert b'identity cache stale'not in result.stderr,'mutation rejected before its graph/data check'
                if label=='redirected application callee':assert b'graph differs'in result.stderr,result.stderr
                if label=='changed callee stack cleanup':assert b'ABI RET cleanup differs'in result.stderr,result.stderr
                if label=='changed reached type hash lookup table':assert b'initial data differs'in result.stderr and b'type_hash_byte'in result.stderr,result.stderr
                if label=='wrong module buffer publication target':assert b'read_types writes wrong current_type_map global'in result.stderr or b'module buffer publication store missing'in result.stderr,result.stderr
                print('rejected:',label)

if __name__=='__main__':main()
