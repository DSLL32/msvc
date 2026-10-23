"""Four-field POD and raw ECX/stack entry and return ABI observations."""
import copy
import json
import re
import struct

FIELDS=('cursor','remaining','minimum_pages','arena')


def check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps):
    if 'CompilerPool::BindArena' in trial['accepted']:
        # Constructor-prefix proof ends at a real earlier checkpoint, before
        # later binding/allocation consumers change these existing fields.
        state=json.loads((native/'arena-size-entry.json').read_text())
        final='%03d'%int(next(row[0]for row in traps if row[1]=='00038ed2'))
    model=json.loads((native/'state-model.json').read_text());count=model['pool_projection']['count']
    member=trial['trial']=='CompilerPool::Initialize';initialized='CompilerPool::Initialize' in trial['accepted']
    def raw(suffix,number=1):
        blob=(original/('pool-%02d-'%number+suffix+'.bin')).read_bytes()
        s.require(len(blob)==16,'missing raw pool object')
        return dict(zip(FIELDS,struct.unpack('<4I',blob)))
    def seeded(pointer):return dict(zip(FIELDS,[pointer+4,23,37,pointer+8]))
    zero=dict.fromkeys(FIELDS,0);result=dict(zero,minimum_pages=1)
    old_saved=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0]
    new_saved=pre['frontend']['saved_state']
    for number in range(1,count+1):
        s.require(raw('000',number)==zero,'original pool defaults differ')
        s.require(raw('006',number)==(seeded(old_saved) if seed else zero),'original nonzero pool input differs')
        s.require(pre['frontend']['pools'][number-1]==(seeded(new_saved) if seed else zero),'native nonzero pool input differs')
        s.require(raw('phase',number)==zero and phase['frontend']['pools'][number-1]==zero,'pool escaped original/native BSS clearing')
        s.require(raw(final,number)==(result if initialized else zero) and state['frontend']['pools'][number-1]==(result if initialized else zero),'pool member result or wrapper input differs')
    rows=[r for r in traps if r[1]=='000389d5'];s.require(len(rows)==count,'missing/extra actual original pool calls')
    row=next(row for row in trial['driver'] if row['name']=='CompilerPool::Initialize')
    s.require(row['symbol']=='?Initialize@CompilerPool@@QAEPAU1@I@Z' and row['native_symbol']=='_ZN12CompilerPool10InitializeEj','unbound member symbol projection')
    if model.get('pool_entry_order_observed'):
        s.require((native/'pool-entry-order.txt').read_text()==''.join('%d\n'%i for i in range(1,count+1)),'native actual pool-call count/order differs')
    entries=[]
    for number,trap in enumerate(rows,1):
        filename='pool-entry-%02d.json'%number if model['schema']>=20 else 'pool-entry.json'
        entry=json.loads((native/filename).read_text()) if member or model['schema']>=20 else state
        entries.append(entry)
        wanted_this=entry['frontend']['storage']['pool_%02d.cursor'%number][0]
        s.require(entry['symbol']=='CompilerPool::Initialize' and entry['args']=={'this':wanted_this,'minimum_pages':1},'actual native member arguments differ')
        suffix='%03d'%int(trap[0]);line=next(line for line in text.splitlines() if line.startswith('trap seq='+trap[0]+' '))
        s.require('ecx=%08x '%model['pool_projection']['original_bases'][number-1] in line and int(trap[2].split(',')[1],16)==1,'original thiscall ECX/stack argument differs')
        caller=next(row for row in trial['driver'] if row['name']=='_initialize_frontend_%02d'%(23+number))
        s.require(int(trap[2].split(',')[0],16)==caller['start']+12,'original member caller-return provenance differs')
        if member or model['schema']>=20:
            wrapper=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_frontend_%02d$'%(23+number),symbols,re.M)
            s.require(wrapper is not None and int(wrapper[1],16)<entry['raw_entry_abi']['return_pc']<int(wrapper[1],16)+int(wrapper[2],16),'native member caller-return provenance differs')
            native_entry=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] _ZN12CompilerPool10InitializeEj$',symbols,re.M)[1],16)
            s.require(entry['pc']==native_entry and entry['raw_entry_abi']=={'ecx':wanted_this,'esp':entry['esp'],'return_pc':entry['raw_entry_abi']['return_pc'],'stack_argument':1},'native member input not observed at raw entry')
        constructor_seed=bool(trial.get('test_pool_seed')) and number==1
        old_owned=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0]
        for other in range(1,count+1):
            old_expected=result if other<number else seeded(old_owned) if constructor_seed and other==number else zero
            new_expected=result if other<number else seeded(entry['frontend']['saved_state']) if constructor_seed and other==number else zero
            s.require(raw(suffix,other)==old_expected and entry['frontend']['pools'][other-1]==new_expected,'pool initialization order/receiver/prestate differs')
        if constructor_seed:
            s.require(member and trial['auxiliary_pool_data_seed']=={'owner':'CompilerPool::Initialize','point':'actual member entry before body','cursor':'owned snapshot+4','remaining':23,'minimum_pages':37,'arena':'owned snapshot+8; never dereferenced','actual_this_and_argument_unchanged':True,'code_or_allocator_mock':False},'undeclared pool constructor seed')
            s.require(re.search(r'^pool-constructor-seed allocation=%08x argument=1 receiver-unchanged=1$'%old_owned,text,re.M),'missing original pool constructor seed')
    if not trial.get('test_pool_seed'):s.require('auxiliary_pool_data_seed' not in trial and 'pool-constructor-seed ' not in text,'hidden pool constructor seed')
    entry=entries[0];trap=rows[0];line=next(line for line in text.splitlines() if line.startswith('trap seq='+trap[0]+' '))
    if not member:return
    ret=json.loads((native/'pool-return.json').read_text());abi=entry['raw_entry_abi']
    native_entry=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] _ZN12CompilerPool10InitializeEj$',symbols,re.M)[1],16)
    s.require(entry['pc']==native_entry and abi['ecx']==entry['args']['this'] and abi['stack_argument']==1 and abi['esp']==entry['esp'],'native member entry not bound to actual registers/stack')
    wrapper=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_frontend_24$',symbols,re.M);start,size=int(wrapper[1],16),int(wrapper[2],16)
    s.require(ret['symbol']=='initialize_frontend_24' and ret['pc']==abi['return_pc'] and start<ret['pc']<start+size and ret['eax']==abi['ecx'] and ret['esp']==abi['esp']+8,'native pool return-this or RET4 ABI differs')
    returns=re.findall(r'^pool-return rva=000389d4 eax=(\w+) esp=(\w+)$',text,re.M)
    original_esp=int(re.search(r' esp=(\w+)',line)[1],16)
    s.require(returns==[('104c0ac8','%08x'%(original_esp+8))] and int(trap[2].split(',')[0],16)==0x104389d4,'original pool return-this or RET4 ABI differs')
    s.require(raw('pool-return')==result and ret['frontend']['pools']==[result],'pool result absent at actual caller return')
    for field in ('first_invocation','saved_state','cleanup_count','restore_mode','cleanup_callbacks','context_stack','saved_context_stack','nested_context_stacks','source_position','output_buffers','deferred_work','token_value','sentinel_token','source_signature','input_pdb_context','output_pdb_context'):
        s.require(entry['frontend'][field]==ret['frontend'][field],'pool member changed neighboring state '+field)
    for frame in (entry,ret):
        s.require(frame['target_state']['last_error']==203 and frame['control_word']==0x27f,'pool member altered caller state')
