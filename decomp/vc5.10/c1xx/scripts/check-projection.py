#!/usr/bin/env python3
"""Validate only modeled fields; original opaque bytes never imply native equivalence."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess

PROJECT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('minimal_base',Path(__file__).with_name('check-minimal.py'))
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
s=b.s


def original_fields(directory,suffix):
    raw=(directory/('frontend-state-'+suffix+'.bin')).read_bytes()
    s.require(len(raw)==144,'incomplete raw modeled-field state')
    words=struct.unpack('<36I',raw)
    return dict(zip(('first_invocation','saved_state','cleanup_count','restore_mode'),words[:4]),
                cleanup_callbacks=list(words[4:]))


def disjoint(ranges):
    pairs=sorted(ranges)
    return all(first<last for first,last in pairs) and all(a[1]<=b[0] for a,b in zip(pairs,pairs[1:]))


def field_map(model,observed):
    s.require(model['original_snapshot_bytes']==8520 and model['original_initialized_region']==[0x104cc000,0x104ce148] and
              model['original_bss_region']==[0x104b9000,0x104c869c],'wrong original state-region boundaries')
    names={row['name'] for row in model['fields']}
    s.require(names==set(observed['storage']),'missing/extra declared field mapping')
    known={'first_invocation':(0x104cd7e0,4,'initialized','frontend_data.first_invocation'),
           'saved_state':(0x104c4d50,4,'bss','frontend_bss.saved_state'),
           'cleanup_count':(0x104c4d54,4,'bss','frontend_bss.cleanup_count'),
           'cleanup_callbacks':(0x104c4d58,128,'bss','frontend_bss.cleanup_callbacks'),
           'restore_mode':(0x104cb06c,4,'external','restore_mode'),
           'compiler_mode':(0x104cb008,4,'external','compiler_mode')}
    s.require(model['schema'] in (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18),'unknown state-model schema')
    if model['schema']>=2:
        known['context_stack']=(0x104ba900,12,'bss','frontend_bss.context_stack')
        s.require(observed['context_stack_fields']==(['origin','position','top'] if model['schema']>=5 else ['begin','end','top']),
                  'unmodeled context stack field')
    if model['schema']>=3:
        known['saved_context_stack']=(0x104ba8f0,12,'bss','frontend_bss.saved_context_stack')
    if model['schema']>=4:
        known['nested_context_stacks']=(0x104ba858,108,'bss','frontend_bss.nested_context_stacks')
        s.require(observed['context_array_count']==9,'wrong encountered context-array length')
    if model['schema']>=5:
        known['source_position']=(0x104becb0,8,'bss','frontend_bss.source_position')
        s.require(observed['source_position_fields']==['origin','position'],'unmodeled source position field')
    if model['schema']>=6:
        layout={'data':[0,4],'cursor':[4,4],'capacity':[8,4],'relocation':[12,4],
                'relocation_context':[16,4],'file':[20,4],'ok':[24,4],'descriptor':[28,4],'mode':[32,1]}
        original_offsets={'data':0,'cursor':4,'capacity':8,'relocation':12,'relocation_context':16,
                          'file':20,'ok':24,'descriptor':0x120,'mode':28}
        output_count=min(model['schema']-5,8)
        for i,base in enumerate((0x104bf5c0,0x104c0590,0x104bf358,0x104c0360,0x104c0840,0x104c0230,0x104c00f8,0x104bf228)[:output_count]):
            for name,(_,width) in layout.items():
                known['output_buffer_%02d.'%(i+1)+name]=(base+original_offsets[name],width,'bss','frontend_bss.output_buffers[%d].'%i+name)
        padding={'bytes':3*output_count,'kind':'natural trailing C object padding; no original semantic counterpart','output_buffer_size':36,'output_mapped_bytes':33,'output_count':output_count}
        if model['schema']>=15:padding.update(bytes=3*output_count+3,sentinel_size=12,sentinel_padding_bytes=3)
        s.require(observed['output_native_layout']=={'size':36,'fields':layout} and observed['output_count']==output_count and
                  model['native_padding']==padding,'unexpected compact output layout/padding')
    if model['schema']>=14:known['deferred_work']=(0x104c0578,4,'bss','frontend_bss.deferred_work')
    if model['schema']>=15:
        known.update({'token_value':(0x104ba8c8,8,'bss','frontend_bss.token_value'),'sentinel_token.kind':(0x104c0b80,1,'bss','frontend_bss.sentinel_token.kind'),'sentinel_token.value':(0x104c0b88,8,'bss','frontend_bss.sentinel_token.value')})
        s.require(model['token_projection']=={'original_payload_bytes':40,'modeled_prefix_bytes':8,'unmodeled_tail_bytes':32,'kind_is_discriminator':True,'tested_union_arm':'integer auxiliary only; normal zero prefix copied without interpretation','pointer_arm':'writer-proven opaque object pointer; no raw original/native pointer integer comparison','unknown_consumers':'hard named trap before any unmodeled field read, escape, serialization or comparison'},'wrong token projection/variant scope')
        full=[(0x104ba8c8,0x104ba8f0),(0x104c0b88,0x104c0bb0)]
        s.require(disjoint(full) and all(not any(a<row['original_address']+row['bytes'] and row['original_address']<z for a,z in full) for row in model['fields'] if row['name'] not in ('token_value','sentinel_token.value')),'token payload tail aliases another modeled field')
        s.require(observed['token_value_layout']=={'size':8,'is_union':True,'fields':{'integer':[0,8],'pointer':[0,4]}} and observed['sentinel_native_layout']=={'size':12,'fields':{'kind':[0,1],'value':[4,8]}},'unmodeled token union/sentinel storage')
    if model['schema']>=16:known['source_signature']=(0x104c0834,4,'bss','frontend_bss.source_signature')
    if model['schema']>=17:
        for field,offset in zip(('database','names','types','minimal_rebuild','stream_state','read_mode'),(0,4,8,12,20,24)):
            known['input_pdb_context.'+field]=(0x104cb030+offset,4,'external','input_pdb_context.'+field)
        s.require(observed['pdb_context_layout']=={'size':24,'fields':{name:[i*4,4] for i,name in enumerate(('database','names','types','minimal_rebuild','stream_state','read_mode'))}},'PDB context projection has unknown storage/gaps')
    if model['schema']>=18:
        for field,offset in zip(('database','names','types','minimal_rebuild','stream_state','read_mode'),(0,4,8,12,20,24)):
            known['output_pdb_context.'+field]=(0x104cb050+offset,4,'external','output_pdb_context.'+field)
    s.require(names==set(known),'field mapping is not the encountered typed projection')
    original=[];native=[]
    for row in model['fields']:
        s.require((row['original_address'],row['bytes'],row['class'],row['native_expression'])==known[row['name']],
                  'field mapping differs from actual decoded original storage')
        if model['schema']>=5 and row['name'] in ('context_stack','saved_context_stack','nested_context_stacks','source_position'):
            wanted=['pointer','u32'] if row['name']=='source_position' else ['pointer','u32','pointer']
            if row['name']=='nested_context_stacks':wanted*=9
            s.require(row['component_types']==wanted,'context scalar was treated as a pointer')
        first,last=row['original_address'],row['original_address']+row['bytes']
        address,width=observed['storage'][row['name']]
        s.require(row['bytes']==width and first%4==address%4==0,'field width/alignment differs')
        original.append((first,last));native.append((address,address+width))
        if row['class']=='initialized':
            s.require(model['original_initialized_region'][0]<=first<last<=model['original_initialized_region'][1] and
                      row['original_snapshot_offset']==first-model['original_initialized_region'][0],
                      'bad initialized field/snapshot offset')
        elif row['class']=='bss':
            s.require(model['original_bss_region'][0]<=first<last<=model['original_bss_region'][1], 'bad BSS field')
        else:s.require(not any(a<=first<z for a,z in (model['original_initialized_region'],model['original_bss_region'])),
                       'external field aliases copied/cleared region')
    s.require(disjoint(original) and disjoint(native),'modeled fields overlap/alias storage')
    fields=['saved_state','cleanup_count','cleanup_callbacks']+(['context_stack'] if model['schema']>=2 else [])+(['saved_context_stack'] if model['schema']>=3 else [])
    if model['schema']>=4:fields.append('nested_context_stacks')
    if model['schema']>=5:fields.append('source_position')
    if model['schema']>=6:fields.append('output_buffers')
    if model['schema']>=14:fields.append('deferred_work')
    if model['schema']>=15:fields += ['token_value','sentinel_token']
    if model['schema']>=16:fields.append('source_signature')
    s.require(observed['declared_fields']=={'initialized':['first_invocation'],'bss':fields} and
              observed['known_sizes']=={'initialized':4,'bss':{1:136,2:148,3:160,4:268,5:276,6:312,7:348,8:384,9:420,10:456,11:492,12:528,13:564,14:568,15:588,16:592,17:592,18:592}[model['schema']]},
              'typed structs contain unmodeled/omitted storage')
    for kind in ('initialized','bss'):
        padding=model['native_padding']['bytes'] if kind=='bss' and model['schema']>=6 else 0
        s.require(sum(row['bytes'] for row in model['fields'] if row['class']==kind)+padding==observed['known_sizes'][kind],
                  'typed field-map does not cover all modeled struct storage')
    if model['schema']>=6:
        ranges=sorted((observed['storage'][row['name']][0],observed['storage'][row['name']][0]+row['bytes'])
                      for row in model['fields'] if row['class']=='bss')
        ranges += [(observed['bss_address']+276+i*36+33,observed['bss_address']+276+(i+1)*36) for i in range(output_count)]
        if model['schema']>=15:ranges.append((observed['bss_address']+577,observed['bss_address']+580))
        ranges.sort()
        s.require(ranges[0][0]==observed['bss_address'] and
                  all(a[1]==b[0] for a,b in zip(ranges,ranges[1:])) and
                  ranges[-1][1]==observed['bss_address']+276+36*output_count+(4 if model['schema']>=14 else 0)+(20 if model['schema']>=15 else 0)+(4 if model['schema']>=16 else 0),'unaccounted native padding/storage gap')


def callback_table(model,original,native,state):
    entries=model['initializers']
    s.require(len(entries)==43 and [row['slot'] for row in entries]==list(range(43)), 'incomplete initializer table')
    raw=(original/'initfe.bin').read_bytes()
    s.require(len(raw)==172 and list(struct.unpack('<43I',raw))==[row['original_address'] for row in entries],
              'actual original callback table differs')
    elf=native/'native-executable.elf'
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(elf)],text=True)
    names={name:int(address,16) for address,size,kind,name in re.findall(r'^(\w+) (\w+) ([Tt]) (\S+)$',symbols,re.M)}
    expected=[0]
    for index,row in enumerate(entries[1:],1):
        name='initialize_frontend_%02d'%index
        s.require(row['symbol']=='_'+name and row['original_address']!=0 and name in names,'callback identity absent')
        expected.append(names[name])
    s.require(entries[0]['original_address']==0 and entries[0]['symbol'] is None and
              state['frontend']['callback_slots']==expected and len(set(expected))==43,
              'native initializer order/NULL slot/storage identity differs')
    return expected


def elf_byte(native,address):
    elf=(native/'native-executable.elf').read_bytes()
    s.require(elf[:6]==b'\x7fELF\x01\x01','not the archived little-endian ELF32 executable')
    offset=struct.unpack_from('<I',elf,28)[0];size,count=struct.unpack_from('<HH',elf,42)
    segments=[struct.unpack_from('<8I',elf,offset+i*size) for i in range(count)]
    matching=[row for row in segments if row[0]==1 and row[6]&1 and row[2]<=address<row[2]+row[4]]
    s.require(len(matching)==1,'frontier outside archived executable ELF segment')
    return elf[matching[0][1]+address-matching[0][2]]


def first_copy_aux(original,native,trial,text,pre,phase,state,slots):
    """A declared debugger return, never evidence that the allocator is implemented."""
    s.require(trial['auxiliary_mock']=={
        'callee':'_checked_malloc','genuine_trap_required':True,'request':8520,
        'returned_buffer_role':'mock-return','old_buffer_role':'old-snapshot',
        'owned_request':8584,'guard_bytes':32,'old_guard':0xa7,'new_guard':0xd3,
        'mocked_cleanup_callback':'_initialize_frontend_01','mocked_cleanup_slots':32,
        'helper_unmasked':False,'helper_acceptance_credit':False},'undeclared auxiliary mock roles')
    s.require(not any(row['owner']=='_checked_malloc' for row in trial['ranges']),
              'auxiliary allocator gained unmask/acceptance credit')
    s.require((original/'masked-text.bin').read_bytes()[0x233ba-0x1000]==0xcc,
              'auxiliary allocator was not an actual masked trap')
    call=json.loads((native/'aux-call.json').read_text());returned=json.loads((native/'aux-return.json').read_text())
    mock=call['aux_mock_call']
    s.require(call['symbol']=='checked_malloc' and call['args']=={'bytes':8520} and
              mock['callee']=='_checked_malloc' and mock['request']==8520 and mock['actual_opcode']=='cc' and
              mock['genuine_int3_pc']==call['pc']-1 and
              returned=={'eax':mock['result'],'pc':mock['caller_return_pc'],
                         'role':'mock-return','helper_accepted':False},'native mock did not follow the real allocator trap')
    # Read the archived ELF byte, rather than accepting the callback's claimed opcode.
    s.require(elf_byte(native,mock['genuine_int3_pc'])==0xcc,
              'archived allocator does not contain the trapped INT3')
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'native-executable.elf')],text=True)
    callee=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] checked_malloc$',symbols,re.M)
    caller=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_compiler_pass$',symbols,re.M)
    s.require(callee is not None and caller is not None,'missing actual auxiliary call edge symbols')
    first,width=[int(word,16) for word in caller.groups()]
    pc=mock['caller_return_pc'];instruction=bytes(elf_byte(native,pc-5+i) for i in range(5))
    s.require(first<pc<first+width and instruction[0]==0xe8 and
              pc+struct.unpack_from('<i',instruction,1)[0]==int(callee[1],16),
              'mock return does not follow the actual initializer-to-allocator call')
    original_mock=re.findall(r'^aux-stub-return role=mock-return rva=000233ba target-byte=cc request=8520 result=([0-9a-f]+) caller=10438595 cdecl-pop=4$',text,re.M)
    s.require(len(original_mock)==1,'missing raw original allocator-return experiment')
    owned=re.findall(r'^aux-owned role=(old-snapshot|mock-return) base=([0-9a-f]+) payload=([0-9a-f]+) request=8584 payload-bytes=8520 guards=32 pattern=(a7|d3) first=(23|303)$',text,re.M)
    s.require(len(owned)==2 and {row[0] for row in owned}=={'old-snapshot','mock-return'},'missing owned original allocations')
    buffers={role:(int(base,16),int(payload,16),int(pattern,16),int(first)) for role,base,payload,pattern,first in owned}
    s.require(buffers['old-snapshot'][2:]==(0xa7,23) and buffers['mock-return'][2:]==(0xd3,303) and
              buffers['old-snapshot'][1]!=buffers['mock-return'][1] and
              int(original_mock[0],16)==buffers['mock-return'][1], 'original mock returned the wrong owned buffer')
    for base,payload,_,_ in buffers.values():s.require(payload==base+32,'original guard/payload layout differs')
    for suffix,snapshot,new_first in (('006',pre,303),('007',call,303),('phase',phase,17),('008',state,17)):
        raw=(original/('aux-owned-fields-'+suffix+'.bin')).read_bytes()
        s.require(len(raw)==16 and struct.unpack('<4I',raw)==
                  (buffers['old-snapshot'][1],buffers['mock-return'][1],23,new_first),
                  'original owned field/copy order differs')
        for old_role,new_role in (('old-snapshot','old'),('mock-return','new')):
            guard=buffers[old_role][2]
            s.require((original/('aux-'+new_role+'-guards-'+suffix+'.bin')).read_bytes()==bytes([guard])*64,
                      'original owned allocation guard changed')
            item=snapshot['aux_owned'][new_role]
            s.require(item['payload']==item['base']+32 and item['owned_request']==8584 and item['payload_bytes']==8520 and
                      item['prefix']==item['suffix']==('%02x'%guard)*32 and
                      item['known_first']==(23 if new_role=='old' else new_first),
                      'native owned allocation/guard/copy order differs')
            s.require(item['payload']==pre['aux_owned'][new_role]['payload'],'native owned buffer identity changed')
    s.require(mock['result']==pre['aux_owned']['new']['payload'] and
              pre['frontend']['saved_state']==pre['aux_owned']['old']['payload'], 'wrong native buffer roles')
    before=original_fields(original,'006')
    s.require(before['first_invocation']==17 and before['saved_state']==buffers['old-snapshot'][1] and
              before['cleanup_count']==32 and before['restore_mode']==9 and
              pre['frontend']['saved_first']==23,'wrong nonzero first-copy input')
    for suffix,snapshot,wanted_first in (('phase',phase,17),('008',state,0)):
        value=original_fields(original,suffix);new=snapshot['frontend']
        s.require(value['first_invocation']==new['first_invocation']==wanted_first and
                  value['saved_state']==buffers['mock-return'][1] and new['saved_state']==mock['result'] and
                  value['cleanup_count']==new['cleanup_count']==32 and value['restore_mode']==new['restore_mode']==0 and
                  value['cleanup_callbacks']==[0x10438610]*32 and new['cleanup_callbacks']==[slots[1]]*32 and
                  new['saved_first']==17,'first-copy pointer/order or BSS preservation differs')
        raw=(original/('frontend-snapshot-'+suffix+'.bin')).read_bytes()
        s.require(len(raw)==8520 and struct.unpack_from('<I',raw,0x17e0)[0]==17,'missing complete original copied snapshot')
    s.require((original/'frontend-data-phase.bin').read_bytes()==(original/'frontend-snapshot-phase.bin').read_bytes(),
              'original opaque first-copy transfer differs')
    return 8520


def check(original,native,require_audit=True,audit_path=None):
    o,n,trial,text,state=s.pair(original,native)
    real_allocator=trial['trial']=='_checked_malloc'
    s.require(trial['trial'] in ('_initialize_compiler_pass','_checked_malloc') and trial['case']=='minimal' and o['exit']==125,
              'wrong initializer trial/frontier')
    s.require(s.sha(native/'native-executable.elf')==n['native_sha256'],'wrong captured ELF')
    model=json.loads((native/'state-model.json').read_text())
    pre=json.loads((native/'pre.json').read_text())
    field_map(model,state['frontend']);field_map(model,pre['frontend'])
    slots=callback_table(model,original,native,state)
    contract=json.loads((native/'contract.json').read_text())
    _,old_argv=b.vector(original/'invoke-entry-argv.bin');native_pointer,new_argv=b.vector(native/'native-argv.bin')
    s.require(old_argv==new_argv==contract['original_argv'] and state['invocation']=={
        'argc':26,'argv':new_argv,'argv_address':native_pointer,'mode':1}, 'real Invoke input differs')
    defaults=original_fields(original,'000')
    s.require(defaults=={'first_invocation':1,'saved_state':0,'cleanup_count':0,'restore_mode':0,'cleanup_callbacks':[0]*32},
              'actual original defaults differ from modeled initializers')
    raw=(original/'frontend-data-000.bin').read_bytes()
    s.require(len(raw)==8520 and struct.unpack_from('<I',raw,0x17e0)[0]==1,'missing complete original initialized state')
    global0=(original/'state-000.bin').read_bytes()
    s.require(struct.unpack_from('<I',global0,8)[0]==0,'compiler-mode original default differs')
    seed=trial.get('test_seed')
    traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    wanted=[('0','0002e3c6'),('1','0002e46a'),('2','000384ce'),('3','0002e52a'),
            ('5','000384d4'),('6','00038550')]
    if seed in (None,'first-copy-aux'):wanted.append(('7','000233ba'))
    if seed or real_allocator:wanted.append(('8' if seed=='first-copy-aux' or real_allocator else '7','00038610'))
    s.require([row[:2] for row in traps]==wanted,'raw initializer execution path differs')
    s.require(set(re.findall(r'^last-error=(\d+)$',text,re.M))=={'203'} and
              {int(word,16)&0xffff for word in re.findall(r'^control-word=(\w+)$',text,re.M)}=={0x27f},
              'original caller LastError/x87 differs')
    pointer,_=b.vector(original/'invoke-entry-argv.bin')
    s.require([int(word,16) for word in traps[4][2].split(',')][1:4]==[26,pointer,1],
              'original raw Invoke ABI differs')
    if seed in (None,'first-copy-aux'):
        s.require(int(traps[6][2].split(',')[1],16)==8520,'original raw allocator argument differs')
    context=(original/'invoke-entry-context.bin').read_bytes()
    s.require(len(context)==204 and struct.unpack_from('<I',context,184)[0]==0x104384d5 and
              struct.unpack_from('<I',context,196)[0]==int(re.search(r'^trap seq=5 .*esp=(\w+)',text,re.M)[1],16),
              'original Invoke entry CONTEXT differs')
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'native-executable.elf')],text=True)
    match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+state['symbol']+'$',symbols,re.M)
    s.require(match is not None,'missing named ELF frontier')
    start,width=[int(word,16) for word in match.groups()]
    s.require(start<state['pc']<=start+width and elf_byte(native,state['pc']-1)==0xcc,'not a real named native INT3 frontier')
    trace=['DllMain','InvokeCompilerPass','initialize_compiler_pass']
    if seed in (None,'first-copy-aux'):trace.append('checked_malloc')
    if seed or real_allocator:trace.append('initialize_frontend_01')
    s.require((native/'trace.txt').read_text()==''.join(name+'\n' for name in trace),'native logical entry trace differs')
    before=original_fields(original,'006');after=original_fields(original,'008' if seed=='first-copy-aux' or real_allocator else '007')
    s.require(pre['symbol']=='initialize_compiler_pass','missing actual native initializer prestate')
    keys=('first_invocation','cleanup_count','restore_mode','cleanup_callbacks')
    for key in keys:s.require(before[key]==pre['frontend'][key] if key!='cleanup_callbacks' or not seed else
                             before[key]==[0x10438610]*32 and pre['frontend'][key]==[slots[1]]*32,
                             'typed prestate differs '+key)
    s.require((before['saved_state']==0)==(pre['frontend']['saved_state']==0),'snapshot pointer nullness differs')
    if real_allocator:
        s.require(not seed and 'auxiliary_mock' not in trial and state.get('aux_owned') is None and
                  'aux-stub-return' not in text and not (native/'aux-return.json').exists(),
                  'real allocator proof contains an auxiliary mock')
        s.require(before==defaults and pre['frontend']['saved_first'] is None and
                  pre['target_state']['compiler_mode']==state['target_state']['compiler_mode']==0,
                  'real allocation did not begin from actual defaults')
        allocation=json.loads((native/'allocation-entry.json').read_text())
        s.require(allocation['symbol']=='checked_malloc' and allocation['args']=={'bytes':8520} and
                  {key:allocation['frontend'][key] for key in before}==before,
                  'actual native malloc call request/prestate differs')
        providers=re.findall(r'^module name=MSVCRT.dll actual=([0-9a-f]+) preferred=([0-9a-f]+) entry=00001000$',text,re.M)
        resolutions=re.findall(r'^provider iat=000c90a8 resolved=([0-9a-f]+)$',text,re.M)
        s.require(len(providers)==len(resolutions)==1 and int(resolutions[0],16)==int(providers[0][0],16)+0x1498,
                  'real malloc is not the canonical CRT import')
        phase=original_fields(original,'phase');phase_native=json.loads((native/'phase.json').read_text())
        for old,new,first in ((phase,phase_native['frontend'],1),(after,state['frontend'],0)):
            s.require(old['first_invocation']==new['first_invocation']==first and
                      old['saved_state']!=0 and new['saved_state']!=0 and
                      old['cleanup_count']==new['cleanup_count']==old['restore_mode']==new['restore_mode']==0 and
                      old['cleanup_callbacks']==new['cleanup_callbacks']==[0]*32 and new['saved_first']==1,
                      'genuine first-copy result/order differs')
        s.require(phase['saved_state']==after['saved_state'] and
                  phase_native['frontend']['saved_state']==state['frontend']['saved_state'] and
                  state['frontend']['saved_state']!=state['target_state']['onexit_begin'],
                  'real snapshot allocation identity changed/aliased CRT heap')
        for suffix in ('phase','008'):
            raw=(original/('frontend-snapshot-'+suffix+'.bin')).read_bytes()
            s.require(len(raw)==8520 and struct.unpack_from('<I',raw,0x17e0)[0]==1,'missing genuine copied snapshot')
        s.require((original/'frontend-data-phase.bin').read_bytes()==(original/'frontend-snapshot-phase.bin').read_bytes(),
                  'genuine original first-copy transfer differs')
        check_observer(trial,text,state)
        frontier='_initialize_frontend_01';opaque=8520
    elif not seed:
        s.require(before==defaults and after==defaults and state['frontend']['first_invocation']==1 and
                  state['frontend']['saved_state']==0 and state['frontend']['cleanup_count']==0 and
                  state['frontend']['cleanup_callbacks']==[0]*32 and state['frontend']['restore_mode']==0,
                  'fresh path changed fields before allocation')
        s.require(pre['target_state']['compiler_mode']==state['target_state']['compiler_mode']==0,
                  'normal initializer changed the caller mode before returning')
        s.require(state['symbol']=='checked_malloc' and state['args']=={'bytes':8520} and
                  re.findall(r'^frontier rva=(\w+)$',text,re.M)==['000233ba'],'fresh allocation request/frontier differs')
        last=re.findall(r'^trap seq=7 .*stack=([0-9a-f,]+)$',text,re.M)
        s.require(len(last)==1 and int(last[0].split(',')[1],16)==8520,'raw original allocation size differs')
        frontier='_checked_malloc';opaque=0
    else:
        s.require(seed in ('restore-nonzero','restore-zero','first-copy-aux'),'unknown seeded case')
        phase=original_fields(original,'phase');phase_native=json.loads((native/'phase.json').read_text())
        if seed=='first-copy-aux':
            opaque=first_copy_aux(original,native,trial,text,pre,phase_native,state,slots)
        else:
            check_restore(original,native,seed,before,after,pre,phase,phase_native,state)
            opaque=8520
        check_observer(trial,text,state)
        mode=0 if seed=='restore-zero' else 3
        s.require(pre['target_state']['compiler_mode']==phase_native['target_state']['compiler_mode']==
                  state['target_state']['compiler_mode']==mode,'compiler mode changed during initializer')
        frontier='_initialize_frontend_01'
    for snapshot in (pre,state):
        s.require(snapshot['target_state']['process_attached']==1 and snapshot['target_state']['last_error']==203 and
                  snapshot['control_word']==0x27f,'lifecycle/LastError/x87 changed')
    s.require('Program received signal SIGTRAP' in (native/'gdb.txt').read_text(),'no real native frontier')
    if require_audit:s.audit(trial,audit_path)
    return {'trial':trial['trial'],'seed':seed,'frontier':frontier,'malloc_bytes':8520,
            'mapped_snapshot_fields':['first_invocation'],'original_unmodeled_snapshot_bytes':8516,
            'native_snapshot_byte_equivalence_claimed':False,'original_opaque_transfer_bytes_checked':opaque,
            'callback_slots_verified':43,'typed_bss_bytes':136,'whole_mask_bytes_checked':751216,
            'original_seconds':o['seconds'],'native_seconds':n['seconds']}


def check_observer(trial,text,state):
    s.require(state['symbol']=='initialize_frontend_01' and state['args']=={} and
              re.findall(r'^frontier rva=(\w+)$',text,re.M)==['00038610'] and
              'restore-phase rva=000385be\n' in text,'unknown consumer did not hard trap after copy/restore')
    observer=trial['validation_observers']
    s.require(len(observer)==1 and {key:observer[0][key] for key in ('owner','original_rva','original_byte')}=={
          'owner':'_initialize_compiler_pass','original_rva':0x385be,'original_byte':0x89} and
          observer[0]['purpose'] in ('observe typed restore/BSS clear before first-flag reset',
              'observe typed copy/restore before first-flag reset'),'undeclared copy/restore validation observer')


def check_restore(original,native,seed,before,after,pre,phase,phase_native,state):
        wanted=1 if seed=='restore-zero' else 0
        s.require(before['first_invocation']==0 and before['cleanup_count']==32 and before['restore_mode']==9 and
                  before['saved_state']!=0,'not the declared nonzero seeded prestate')
        s.require(phase['first_invocation']==phase_native['frontend']['first_invocation']==17 and
                  after['first_invocation']==state['frontend']['first_invocation']==0,'snapshot restore/reset order differs')
        for old,new in ((phase,phase_native['frontend']),(after,state['frontend'])):
            s.require(old['saved_state']==before['saved_state'] and new['saved_state']==pre['frontend']['saved_state'] and
                      old['cleanup_count']==new['cleanup_count']==0 and
                      old['cleanup_callbacks']==new['cleanup_callbacks']==[0]*32 and
                      old['restore_mode']==new['restore_mode']==wanted and new['saved_first']==17,
                      'typed BSS clear/retained pointer/restore-mode differs')
        for suffix in ('seed','006','phase','007'):
            snapshot=(original/('frontend-snapshot-'+suffix+'.bin')).read_bytes()
            s.require(len(snapshot)==8520 and struct.unpack_from('<I',snapshot,0x17e0)[0]==17,
                      'missing complete original opaque snapshot/mapped flag')
        s.require((original/'frontend-data-phase.bin').read_bytes()==(original/'frontend-snapshot-phase.bin').read_bytes(),
                  'original opaque restore itself differs')


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--packet',type=Path,default=PROJECT/'cases/minimal/initializer-trials.json')
    args=parser.parse_args();packet=json.loads(args.packet.read_text());results=[]
    spec=importlib.util.spec_from_file_location('history',Path(__file__).with_name('check-history.py'))
    history=importlib.util.module_from_spec(spec);spec.loader.exec_module(history)
    import sys
    decoder=sys.modules[__name__]
    for row in packet['trials']:
        original,native=Path(row['original']),Path(row['native'])
        active=decoder
        if (json.loads((native/'trial.json').read_text())['trial'] or '').startswith(('_initialize_frontend_','_initialize_output_buffer_','_initialize_pdb_context_')) or json.loads((native/'trial.json').read_text())['trial'] in ('_register_cleanup','_initialize_deferred_work'):
            active=history.module('callback_packet','check-callback.py')
        audit=history.configure(active,original,native,saved_record=row)
        results.append(active.check(original,native,audit_path=audit))
    print(json.dumps({'result':'PASS','projection_trials':results},indent=2))


if __name__=='__main__':main()
