"""Live names/address heap state after the public writer's automatic frame ends."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct


def complete_module():
    spec=importlib.util.spec_from_file_location('hash_chain_complete',Path(__file__).with_name('address-hash-complete-state.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


def prove(config):
    from state import BINARY,ROOT,sha
    from binary import PE
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    from capstone.x86 import X86_OP_MEM
    binding=config['next_hash_block_policy'];path=BINARY/binding['path'];assert sha(path)==binding['sha256'],'changed next-block policy'
    policy=json.loads(path.read_text());assert policy['target_sha256']==config['target']['sha256'];pe=PE(ROOT/config['target']['path']);decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    intervals=[(0x4100d0,0x4100d8,set()),(0x40d84c,0x40d998,{0x412d34,0x412d40,0x412d38,0x415628,0x4161e0}),(0x406c5a,0x406ca4,{0x415640,0x412124,0x412d40,0x415654,0x415660,0x4153b8}),(0x401a80,0x401a9e,{0x41565c})]
    assert len(policy['integer_intervals'])==len(intervals)
    for row,(start,end,allowed) in zip(policy['integer_intervals'],intervals):
        assert row['extent']==[start,end];body=pe.read(start,end-start);assert hashlib.sha256(body).hexdigest()==row['sha256'];code=list(decoder.disasm(body,start));assert sum(i.size for i in code)==len(body)
        assert not any(i.mnemonic.startswith(('f','v','xsave','xrstor'))for i in code),'next-block raw FP consumer'
        assert all(o.mem.disp in allowed for i in code for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index),'next-block unexpected absolute/retired-root consumer'
    assert pe.read(0x4100d0,8)==bytes.fromhex('8b4424048b4008c3') and pe.read(0x4100c0,4)==bytes.fromhex('8b442404')
    return dict(policy_sha256=sha(path),integer_intervals=policy['integer_intervals'],validator_sha256=sha(Path(__file__)),scope='finite live names/address chains and write_publics return through actual write_globals; automatic arenas historical, no future consumer permission')


def decode(events):
    state=dict(calls=[],current={})
    for line in events.splitlines():
        if ' eax ' in line:continue
        f=line.split()
        if line.startswith('hash-chain-call '):
            _,index,site,esp,ret,arg,eax,*regs=f;assert int(index)==len(state['calls'])<3 and (not state['calls']or'result'in state['calls'][-1]);state['calls'].append(dict(site=int(site,16),entry_esp=int(esp,16),return_pc=int(ret,16),argument=int(arg,16),observed_eax=int(eax,16),before_nonvolatile=[int(v,16)for v in regs]))
        elif line.startswith(('hash-chain-input ','hash-chain-after-input ')):
            label,address,size,payload=f;row=state['calls'][-1];key='before_input'if label=='hash-chain-input'else'after_input';assert key not in row and int(address,16)==row['argument'] and len(bytes.fromhex(payload))==int(size);row[key]=bytes.fromhex(payload)
        elif line.startswith('hash-chain-return '):
            _,index,site,esp,result,*regs=f;row=state['calls'][-1];assert int(index)==len(state['calls'])-1 and 'result'not in row;row.update(return_site=int(site,16),exit_esp=int(esp,16),result=int(result,16),after_nonvolatile=[int(v,16)for v in regs])
        elif line.startswith('hash-chain-address-copy '):
            assert 'copy'not in state;state['copy']=[int(v,16)for v in f[1:]]
        elif line.startswith(('hash-chain-name-','hash-chain-address-')):
            label,address,size,payload=f;parts=label.split('-');name='names'if parts[2]=='name'else'addresses';key='block'if parts[3]=='block'else'payload';row=state['current'].setdefault(name,{});assert key not in row and len(bytes.fromhex(payload))==int(size);row[key]=bytes.fromhex(payload);row[key+'_address']=int(address,16)
        elif line=='hash-chain-observation-failed':raise AssertionError('incomplete hash-chain observations')
    return state


def phase(events,native):
    lines=events.splitlines();begin=[i for i,l in enumerate(lines)if l=='hash-next-phase-entry 004100d0'];end=[i for i,l in enumerate(lines)if l=='hash-next-phase-end 004100d0'];assert len(begin)==len(end)==1 and 0<begin[0]<end[0] and lines[begin[0]-1]=='entry 004100d0','next-block phase lacks genuine entry bounds'
    assert not any(l.startswith('frontier ')for l in lines[:end[0]+1]),'next-block phase follows actual trap'
    observed=native['hash_chain']['phase'];old=native['hash_chain_entry_phase'];call=native['hash_chain']['calls'][0]
    assert old['function']=='next_hash_output_block' and old['frontier_caller']=='write_publics' and old['pc']==observed['typed_pc'] and old['entry_esp']==observed['typed_esp'] and call['entry_pc']==observed['physical_pc'] and call['entry_esp']==observed['physical_esp'],'next-block physical/typed phase identity differs'
    complete=complete_module();start,body=complete.native_symbol('next_hash_output_block');delta=observed['typed_pc']-observed['physical_pc']
    # One pointer formal: exact frame/GOT setup and argument-slot load before
    # instrumentation or block->next dereference. Real entry ESP is independent.
    assert delta==25 and body[:15]==bytes.fromhex('5589e55383ec14e8000000005b81c3') and body[19:25]==bytes.fromhex('895df48b4508'),'next-block typed checkpoint includes semantic effects'
    assert observed['typed_esp']==observed['physical_esp']-28,'next-block typed checkpoint stack differs'
    prefix=lines[:end[0]+1];prefix[begin[0]-1]='frontier 004100d0'
    return '\n'.join(prefix)+'\n',old,dict(begin=begin[0],end=end[0],raw_phase_sha256=hashlib.sha256(('\n'.join(lines[:end[0]+1])+'\n').encode()).hexdigest(),native_prologue_sha256=hashlib.sha256(body[:delta]).hexdigest(),kind='fresh actual113 candidate-entry historical phase, actual113 mask retained; no real trap claimed')


def validate(config,data,events,native,artifacts,views,compare,rawdecode):
    policy=prove(config);phase_events,previous,phase_proof=phase(events,native)
    prefix=compare(dict(config,observe_next_hash_output_block=False),data,phase_events,previous,artifacts,views)
    lines=events.splitlines();current=[];inside=False
    for line in lines:
        if line.startswith(('addresshash-phase-entry ','hash-next-phase-entry ')):assert not inside;inside=True;continue
        if line.startswith(('addresshash-phase-end ','hash-next-phase-end ')):assert inside;inside=False;continue
        if not inside or line.startswith('mapped-view '):current.append(line)
    assert not inside
    phase_lines=phase_events.splitlines();a=phase_lines.index('addresshash-phase-entry 0040edc0');b=phase_lines.index('addresshash-phase-end 0040edc0')
    historical_events='\n'.join(phase_lines[:a]+[line for line in phase_lines[a:b+1]if line.startswith('mapped-view ')]+phase_lines[b+1:])+'\n'
    original=rawdecode('\n'.join(current)+'\n');historical=rawdecode(historical_events);old=decode(events);new=native['hash_chain']
    old['address_phase']=complete_module().decode(phase_events)['current']
    return validate_current(config,data,original,native,historical,previous,old,new,prefix,phase_proof,policy)


def validate_current(config,data,original,native,historical,previous,old,new,prefix,phase_proof,policy):
    assert original['address']==0x40d9a0 and original['stack'][0]==0x406ca4 and native['function']=='write_globals' and native['frontier_caller']=='flush_packed_file','next-block final real frontier differs'
    assert len(old['calls'])==len(new['calls'])==3,'next-block helper inventory differs'
    names=historical['name_hash_complete'];other_names=previous['name_hash_complete']
    # The current address owner is explicitly typed in the live113 phase;
    # original parser retains it in complete address raw observations.
    actual_phase=old['address_phase'];native_addresses=previous['address_hash_complete']['current']
    roots=[(names,other_names),(actual_phase,native_addresses)]
    complete=complete_module()
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    caller_start,caller_body=complete.native_symbol('write_publics');bias=native['function_addresses']['write_publics']-caller_start
    caller_code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(caller_body,caller_start))
    for index,(a,b)in enumerate(zip(old['calls'],new['calls'])):
        first=index==1;owner,other=roots[1 if first or index==2 else 0];expected_arg=actual_phase['arena_address']if first else owner['block_address'];expected_other=other['arena_address']if first else other['block_address'];before=owner['arena']if first else owner['block'];other_before=bytes.fromhex(other['arena']if first else other['block']);site=0x4100c4 if first else 0x4100d0 if index==0 else 0x4100d4;name='first_hash_output_block'if first else'next_hash_output_block';width=24 if first else 12
        assert a['site']==site and a['return_site']==site+(7 if index==0 else 3) and a['return_pc']==(0x40d86b if first else 0x40d84c if index==0 else 0x40d8de) and b['name']==name and b['owner']=='write_publics' and b['width']==width and b['formal_type']==('TypeArena *'if first else'TypeArenaBlock *'),'next-block actual helper sequence/ABI differs'
        if index:assert a['observed_eax']==a['argument'],'next-block observed argument-load EAX differs'
        assert a['argument']==expected_arg and b['argument']==expected_other and a['before_input']==a['after_input']==before and bytes.fromhex(b['before_input'])==bytes.fromhex(b['after_input'])==other_before,'next-block helper live input/alias differs'
        assert a['result']==(owner['block_address']if first else 0) and b['result']==(other['block_address']if first else 0),'next-block helper actual result differs'
        helper_start,_=complete.native_symbol(name);returns={i.address+i.size for i in caller_code if i.mnemonic=='call' and i.op_str==hex(helper_start)}
        assert b['entry_pc']==native['function_addresses'][name] and b['return_pc']-bias in returns,'next-block native actual CALL role differs'
        assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'next-block helper return ABI differs'
    assert old['copy'][0]==old['calls'][2]['entry_esp']+8,'next-block original inline-copy stack differs'
    assert old['copy'][1:]==[actual_phase['block_address'],actual_phase['payload_address']+540,original['output_buffer_state']['address']+8292],'next-block actual address copy boundary differs'
    for name,owner,other in [('names',names,other_names),('addresses',actual_phase,native_addresses)]:
        a=old['current'][name];b=new['current'][name]
        for key in ('block','payload'):
            assert a[key+'_address']==owner[key+'_address'] and b[key+'_address']==other[key+'_address'] and a[key]==owner[key] and bytes.fromhex(b[key])==bytes.fromhex(other[key]),'next-block current live heap owner/bytes differ'
    expected=historical['output_buffer_state']['bytes']+actual_phase['payload'];assert len(expected)==8292
    a=original['output_buffer_state'];b=native['output_buffer_state'];assert a['bytes']==bytes.fromhex(b['bytes'])==expected and a['used']==b['used']==8292 and a['capacity']==b['capacity']==65512 and a['address']==historical['output_buffer_state']['address'] and b['address']==previous['output_buffer_state']['address'],'next-block complete packed output differs'
    rows={row['name']:row for row in data}
    def root(state,name):return int.from_bytes(state['memory'][rows[name]['original']],'little')
    def actual(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
    for name,row in rows.items():
        if name in ('output_used','directory_count','image_read_mode'):
            expected={'output_used':8292,'directory_count':59,'image_read_mode':4944+8292}[name];assert root(original,name)==actual(native,name)==expected,'next-block changed scalar value differs: '+name
        else:assert original['memory'][row['original']]==historical['memory'][row['original']] and native['globals'][name]==previous['globals'][name],'next-block unexpected changed global: '+name
    assert original['stack'][1]==root(original,'directory_entries')+59*12 and original['stack'][2]==8292,'next-block original next output arguments differ'
    assert native['observed_argument_types']==[dict(name='entry',kind='pointer',width=4,native_type='CodeViewDirectoryEntry *',pointee_width=12),dict(name='offset',kind='integer',width=4,native_type='unsigned int')] and native['parameters']==dict(entry=dict(root='directory_entries',index=59,extent=12),offset=8292),'next-block actual next typed output arguments differ'
    # No allocations, releases, input reads, resource/dependency changes or
    # mapping/file APIs occur after this phase; address reuse cannot create a
    # new generation here. No dead automatic descriptor is read at the end.
    for key in ('checked_allocations','cleared_allocations','image_allocations','module_allocations','subsection_allocations','main_free_calls','source_family_allocations','source_family_frees','source_index_allocations','output_buffer_allocations','subsection_calls','file_calls','mapping_calls','file_reads','name_hash_allocations','address_hash_allocations'):
        assert original[key]==historical[key] and native[key]==previous[key],'next-block unexpected lifecycle/API continuation: '+key
    excluded={r['original']for r in data}|{historical['esp'],names['block_address']}
    assert {k:v for k,v in original['memory'].items()if k not in {r['original']for r in data}|{original['esp']}}=={k:v for k,v in historical['memory'].items()if k not in excluded},'next-block current original generic memory changed/leaked'
    assert native['memory']==previous['memory'] and native['strings']==previous['strings'] and original['strings']==historical['strings'],'next-block current generic objects changed/leaked'
    for side,state,before in [('original',original,historical),('native',native,previous)]:
        env=state['output_buffer_state']['environment'];baseline=before['output_buffer_state']['environment'];keys=('fp','raw_errors','teb','tid','tls_index','tls_slot','ptd')if side=='original'else('fp','errno','doserrno','last_error','errno_address','doserrno_address','last_error_address')
        assert all(env[k]==baseline[k]for k in keys),'next-block own full FP/error preservation differs'
        assert state['file_handles']==before['file_handles'],'next-block actual file identity/state differs'
    assert not any(k in native for k in ('public_writer_frame','name_hash_complete','address_hash_complete')),'next-block dead frame relabeled current'
    return dict(prefix=prefix,phase=phase_proof,policy=policy,parameters=native['parameters'],hash_chain=dict(calls=3,next_returns_null=2,names_initialized_bytes=564,address_initialized_bytes=540,current_heap_initialized_bytes=1128,packed_output_initialized_bytes=8292,own_unwritten_packed_tail_bytes_uncompared=57220,current_directory_count=59,next_offset=8292,dead_automatic_descriptors_historical_only=True,next_consumer='write_globals(directory_entries[59],8292)'))
