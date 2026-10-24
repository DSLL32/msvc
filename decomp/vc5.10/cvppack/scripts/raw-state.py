"""Pure original-event decoder and typed application-state comparison."""
import json
import re
import struct
import hashlib


def decode(events):
    lines = events.splitlines()
    frontiers = [(index,int(line.split()[1],16)) for index,line in enumerate(lines)
                 if re.fullmatch(r'frontier [0-9a-f]{8}',line)]
    assert len(frontiers) == 1, 'expected one raw original frontier'
    index,address = frontiers[0]
    memory,strings = {},{}
    file_calls=[];all_strings={};last_error=None;handles={};mapping_calls=[];mapped_views={};file_reads=[];directory_allocation={};subsection_allocations=[];image_allocations=[];checked_allocations=[];module_allocations=[];cleared_allocations=[]
    for line in lines:
        if line.startswith('module-allocation-call '):
            _,entry,size=line.split();module_allocations.append(dict(entry=int(entry,16),size=int(size)))
        elif line.startswith('module-allocation-return '):
            assert module_allocations and 'address' not in module_allocations[-1]
            module_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('cleared-allocation-call '):
            cleared_allocations.append(dict(size=int(line.split()[1])))
        elif line.startswith('cleared-allocation-return '):
            assert cleared_allocations and 'address' not in cleared_allocations[-1]
            cleared_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('checked-allocation-call '):
            checked_allocations.append(dict(size=int(line.split()[1])))
        elif line.startswith('checked-allocation-return '):
            assert checked_allocations and 'address' not in checked_allocations[-1]
            checked_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('image-allocation-call '):
            _,site,size=line.split();image_allocations.append(dict(site=int(site,16),size=int(size)))
        elif line.startswith('image-allocation-return '):
            assert image_allocations and 'address' not in image_allocations[-1]
            image_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('subsection-allocation-call '):
            _,entry,size=line.split();subsection_allocations.append(dict(entry=int(entry,16),size=int(size)))
        elif line.startswith('subsection-allocation-return '):
            assert subsection_allocations and 'address' not in subsection_allocations[-1]
            subsection_allocations[-1]['address']=int(line.split()[1],16)
        elif line.startswith('directory-allocation-call '):
            assert not directory_allocation,'directory allocation generation needs an explicit model'
            directory_allocation['size']=int(line.split()[1])
        elif line.startswith('directory-allocation-return '):
            directory_allocation['address']=int(line.split()[1],16)
        elif line.startswith(('blob-observation-failed','public-node-observation-failed')):raise AssertionError('incomplete initialized allocation observation')
        elif line.startswith('file-read-call '):
            _,file,buffer,size,pc,position=line.split()
            file_reads.append(dict(file=int(file),buffer=int(buffer,16),size=int(size),pc=int(pc,16),entry_position=int(position),entry_last_error=last_error))
        elif line.startswith('file-read-return '):
            assert file_reads and 'returned' not in file_reads[-1]
            _,returned,position=line.split()
            file_reads[-1].update(returned=int(returned),exit_position=int(position),exit_last_error=last_error)
        elif line.startswith('file-read-bytes '):
            assert file_reads and 'returned' in file_reads[-1]
            file_reads[-1]['bytes']=line.split(' ',1)[1]
            assert len(bytes.fromhex(file_reads[-1]['bytes']))==file_reads[-1]['returned']
        elif line in ('file-read-failed','file-read-overlap'):raise AssertionError('incomplete actual read observation')
        elif line.startswith(('mapping-create-arguments ','view-map-arguments ')):
            fields=line.split();args=[int(value,16) for value in fields[1:]];assert len(args)==6
            mapping_calls.append(dict(kind='create' if fields[0]=='mapping-create-arguments' else 'map',args=args))
        elif line.startswith('mapping-close-argument '):
            mapping_calls.append(dict(kind='close',args=[int(line.split()[1],16)]))
        elif line.startswith(('mapping-create-result ','view-map-result ','mapping-close-result ')):
            assert mapping_calls and 'result' not in mapping_calls[-1]
            mapping_calls[-1].update(result=int(line.split()[1],16),last_error=last_error)
        elif line.startswith('file-open-arguments '):
            args=[int(value,16) for value in line.split()[1:]];assert len(args)==7
            file_calls.append(dict(pointer=args[0],**dict(zip(('access','share','security','disposition','attributes','template'),args[1:]))))
        elif line.startswith('file-open-result '):
            assert file_calls and 'handle' not in file_calls[-1]
            file_calls[-1].update(handle=int(line.split()[1],16),last_error=last_error)
        elif line.startswith('last-error '):last_error=int(line.split()[1])
        elif line.startswith('string '):
            _,location,raw=line.split()
            assert raw not in ('FAILED','UNBOUNDED')
            all_strings[int(location,16)]=raw
    for call in file_calls:call['pathname']=all_strings[call.pop('pointer')]

    widen_factory_calls=[];widen_query_calls=[];widen_emit_calls=[];widen_release_calls=[];widen_sequence=[];main_free_calls=[];probe_observed={};widen_read_count=0;module_bases={};error=None;probe_regs=None;probe_stack=None
    for ordinal,line in enumerate(lines):
        if line.startswith('file-read-call '):widen_read_count+=1
        if line.startswith('module '):
            parts=line.split()
            if len(parts)==3:module_bases[parts[2].lower()]=int(parts[1],16)
        elif line.startswith('last-error '):error=int(line.split()[1])
        elif line.startswith('registers '):probe_regs=[int(value,16) for value in line.split()[1:]];probe_stack=None;probe_observed={}
        elif line.startswith('memory ') and probe_regs:
            _,probe_address,contents=line.split()
            probe_observed[int(probe_address,16)]=bytes.fromhex(contents)
            if int(probe_address,16)==probe_regs[-1]:probe_stack=bytes.fromhex(contents)
        elif re.fullmatch(r'main-free-call [0-9a-f]{8} [0-9a-f]{8} [0-9]+ [0-9a-f]{8}',line):
            _,site,location,size,provider=line.split();location=int(location,16);size=int(size)
            assert probe_stack is not None,'main free actual stack missing'
            before=b''
            if location:
                following=lines[ordinal+1].split()
                assert following[:2]==['memory','%08x'%location],'main free pre-free snapshot missing'
                before=bytes.fromhex(following[2])
            main_free_calls.append(dict(site=int(site,16),address=location,size=size,provider=int(provider,16),before=before,
                entry_esp=probe_regs[-1],stack_argument=struct.unpack('<I',probe_stack[:4])[0],globals=dict(probe_observed),entry_last_error=error))
        elif re.fullmatch(r'main-free-return [0-9a-f]{8} [0-9a-f]{8}',line):
            _,site,location=line.split()
            assert main_free_calls and main_free_calls[-1]['site']==int(site,16) and main_free_calls[-1]['address']==int(location,16),'main free return sequence differs'
            main_free_calls[-1].update(exit_esp=probe_regs[-1],exit_last_error=error)
        elif line.startswith(('main-free-observation-failed','released-memory-range-failed')):raise AssertionError('incomplete main free lifetime observation')
        elif line.startswith('widen create-call '):
            _,_,output,capacity,flags=line.split()
            widen_sequence.append('factory-call')
            widen_factory_calls.append(dict(output_argument=int(output,16),capacity=int(capacity),flags=int(flags),entry_last_error=error))
        elif line.startswith('widen create-return '):
            assert widen_factory_calls and 'result' not in widen_factory_calls[-1],'unexpected factory return'
            _,_,result,object,table=line.split();observed={}
            for following in lines[ordinal+1:ordinal+5]:
                if not following.startswith('memory '):break
                _,observed_address,contents=following.split();observed[int(observed_address,16)]=bytes.fromhex(contents)
            widen_sequence.append('factory-return')
            widen_factory_calls[-1].update(result=int(result),object=int(object,16),table=int(table,16),memory=observed,exit_last_error=error)
        elif line.startswith(('widen method-call 32 ','widen method-return 32 ')):
            parts=line.split();observed={}
            for following in lines[ordinal+1:ordinal+4]:
                if not following.startswith('memory '):break
                _,observed_address,contents=following.split();observed[int(observed_address,16)]=bytes.fromhex(contents)
            if parts[1]=='method-call':
                assert probe_stack is not None and len(probe_stack)>=16,'query actual ABI stack observation missing'
                widen_sequence.append('query-call')
                widen_query_calls.append(dict(read_count=widen_read_count,object=int(parts[3],16),description=int(parts[4],16),input_address=int(parts[5],16),size=int(parts[6],16),flags=int(parts[7],16),before=observed,entry_last_error=error,entry_esp=probe_regs[-1],entry_ecx=probe_regs[2],entry_stack_words=struct.unpack('<4I',probe_stack[:16])))
            else:
                assert widen_query_calls and widen_query_calls[-1]['object']==int(parts[3],16),'query return object/order differs'
                widen_sequence.append('query-return')
                widen_query_calls[-1].update(result=int(parts[4]),after=observed,exit_last_error=error,exit_esp=probe_regs[-1],exit_eax=probe_regs[0])
        elif line.startswith(('widen method-call 36 ','widen method-return 36 ')):
            parts=line.split();observed={}
            for following in lines[ordinal+1:ordinal+5]:
                if not following.startswith('memory '):break
                _,observed_address,contents=following.split();observed[int(observed_address,16)]=bytes.fromhex(contents)
            if parts[1]=='method-call':
                assert probe_stack is not None and len(probe_stack)>=16,'emit actual ABI stack observation missing'
                widen_sequence.append('emit-call')
                widen_emit_calls.append(dict(object=int(parts[3],16),description=int(parts[4],16),input_address=int(parts[5],16),size=int(parts[6],16),flags=int(parts[7],16),before=observed,entry_last_error=error,entry_esp=probe_regs[-1],entry_ecx=probe_regs[2],entry_stack_words=struct.unpack('<4I',probe_stack[:16])))
            else:
                assert widen_emit_calls and widen_emit_calls[-1]['object']==int(parts[3],16),'emit return object/order differs'
                widen_sequence.append('emit-return')
                widen_emit_calls[-1].update(result=int(parts[4]),after=observed,exit_last_error=error,exit_esp=probe_regs[-1],exit_eax=probe_regs[0])
        elif line.startswith('widen destroy '):
            assert probe_stack is not None and len(probe_stack)>=8,'Release actual ABI stack missing'
            observed={}
            for following in lines[ordinal+1:ordinal+4]:
                if not following.startswith('memory '):break
                _,location,contents=following.split();observed[int(location,16)]=bytes.fromhex(contents)
            widen_sequence.append('release-call')
            widen_release_calls.append(dict(object=int(line.split()[2],16),entry_ecx=probe_regs[2],table=probe_regs[0],entry_esp=probe_regs[-1],entry_stack_words=struct.unpack('<2I',probe_stack[:8]),before=observed,entry_last_error=error))
        elif line.startswith('widen copied '):
            assert widen_release_calls and 'copied' not in widen_release_calls[-1]
            _,_,location,size=line.split();following=lines[ordinal+1].split()
            assert following[:2]==['memory',location],'Release copied-byte observation missing'
            raw=bytes.fromhex(following[2]);assert len(raw)==int(size),'Release copied-byte extent differs'
            widen_release_calls[-1].update(copied_address=int(location,16),copied=raw.hex(),copied_size=int(size))
        elif line.startswith('widen destroy-return '):
            assert widen_release_calls and widen_release_calls[-1]['object']==int(line.split()[2],16),'Release return object/order differs'
            widen_sequence.append('release-return')
            widen_release_calls[-1].update(exit_esp=probe_regs[-1],exit_stack_word=struct.unpack('<I',probe_stack[:4])[0],exit_last_error=error)
    type_compare_calls=[]
    for line in lines:
        if line.startswith('type-compare-failed'):raise AssertionError('incomplete recursive comparison observation')
        if line.startswith('type-compare-call '):
            _,record,offset,size,left,right=line.split()
            size=int(size)
            assert left!='FAILED' and right!='FAILED','unobserved recursive comparison bytes'
            assert len(bytes.fromhex(left))==len(bytes.fromhex(right))==size,'truncated recursive comparison bytes'
            type_compare_calls.append(dict(record=int(record,16),offset=int(offset),size=size,left=left,right=right))
        elif line.startswith('type-compare-return '):
            assert type_compare_calls and 'different' not in type_compare_calls[-1],'recursive comparison return inventory differs'
            type_compare_calls[-1]['different']=bool(int(line.split()[1]))
    lookup_calls=[];lookup_stack=[]
    for line in lines:
        if line.startswith('lookup-type-failed'):raise AssertionError('incomplete global-type lookup observation')
        if line.startswith('lookup-type-entry '):
            _,depth,argument,output,stack=line.split();depth=int(depth);output=int(output,16);stack=int(stack,16)
            assert depth==len(lookup_stack),'lookup entry nesting differs'
            row=dict(depth=depth,index=int(argument),parent_argument_alias=bool(lookup_stack and output==lookup_stack[-1]['stack']+8),stack=stack)
            if depth:assert row['parent_argument_alias'],'original recursive lookup output does not alias parent argument'
            lookup_calls.append(row);lookup_stack.append(row)
        elif line.startswith('lookup-type-return '):
            _,depth,result,output=line.split();depth=int(depth)
            assert lookup_stack and depth==len(lookup_stack)-1,'lookup return nesting differs'
            lookup_stack.pop().update(result=int(result,16),output=int(output,16))

    registers = None
    for line in lines[index+1:]:
        if line.startswith('registers '):
            registers = [int(value,16) for value in line.split()[1:]]
        elif line.startswith('memory '):
            _,location,raw = line.split()
            memory[int(location,16)] = bytes.fromhex(raw)
        elif line.startswith('mapped-view '):
            _,view,length,base,region,protect,committed,kind,ordinal=line.split()
            mapped_views[int(view,16)]=dict(length=int(length),base=int(base,16),region=int(region),
                protection=int(protect),state=int(committed),type=int(kind),file='events-view-%d.bin'%int(ordinal))
        elif line.startswith('mapped-view-failed ') or line.startswith('file-handle-failed '):
            raise AssertionError('incomplete original typed resource observation')
        elif line.startswith('file-handle '):
            _,handle,kind,size,high,position=line.split()
            handles[int(handle,16)]=dict(type=int(kind),size=int(size)+(int(high)<<32),position=int(position))
        elif line.startswith('string '):
            _,location,raw = line.split()
            assert raw not in ('FAILED','UNBOUNDED'), 'unreadable string argument'
            value = bytes.fromhex(raw)
            assert value[-1:] == b'\0' and b'\0' not in value[:-1]
            strings[int(location,16)] = value
    assert registers and len(registers) == 8
    stack = memory[registers[-1]]
    assert len(stack) == 128
    return dict(address=address,memory=memory,strings=strings,widen_factory_calls=widen_factory_calls,widen_query_calls=widen_query_calls,widen_emit_calls=widen_emit_calls,widen_release_calls=widen_release_calls,widen_sequence=widen_sequence,main_free_calls=main_free_calls,module_bases=module_bases,lookup_calls=lookup_calls,lookup_active_depth=len(lookup_stack),type_compare_calls=type_compare_calls,
                registers=dict(zip(('eax','ebx','ecx','edx','esi','edi','ebp','esp'),registers)),esp=registers[-1],stack=struct.unpack('<32I',stack),file_calls=file_calls,file_handles=handles,last_error=last_error,mapping_calls=mapping_calls,mapped_views=mapped_views,file_reads=file_reads,directory_allocation=directory_allocation,subsection_allocations=subsection_allocations,image_allocations=image_allocations,checked_allocations=checked_allocations,module_allocations=module_allocations,cleared_allocations=cleared_allocations)


def compare(config,data,events,native,artifacts=None,views=None):
    original = decode(events)
    record = next(r for r in config['functions'] if r['original']==original['address'])
    assert native['function'] == record['native_symbol']
    factories=original['widen_factory_calls']
    if config.get('observe_widen_factory'):
        assert len(factories)==len(native.get('widen_factory_calls',[]))<=21,'factory lifetime inventory differs'
        policy=config['observe_widen_factory'];roles=['destroy','unknown','unknown','record','unknown','unknown','unknown','unknown','query','emit']
        for before,after in zip(factories,native['widen_factory_calls']):
            assert before['capacity']==after['capacity']==256 and before['flags']==after['flags']==0 and before['result']==after['result']==1,'factory actual arguments/result differ'
            assert before['entry_last_error']==before['exit_last_error']==after['entry_last_error']==after['exit_last_error']==0,'factory actual LastError differs'
            base=original['module_bases']['mspdb50.dll']
            assert before['table']==base+policy['table_rva'],'factory actual provider vtable identity differs'
            observed=before['memory']
            assert observed.get(before['output_argument'])==before['object'].to_bytes(4,'little'),'factory actual output pointer write differs'
            assert observed[before['object']]==before['table'].to_bytes(4,'little'),'factory actual opaque object vptr differs'
            assert len(observed[before['table']])==40 and [value-base for value in struct.unpack('<10I',observed[before['table']])]==policy['slots'],'factory original method identities differ'
            assert after['vtable_roles']==roles and after['vtable_slots']==[after['provider_functions'][name] for name in roles],'factory native typed method identities differ'
            assert after['owner']=='widen_subsection' and after['local']=='object','factory native output owner differs'
    if config.get('observe_widen_query'):
        calls=original['widen_query_calls']
        assert len(calls)==len(native.get('widen_query_calls',[]))<=21,'query lifetime inventory differs'
        for instance,(before,after) in enumerate(zip(calls,native['widen_query_calls'])):
            assert instance<len(factories) and before['object']==factories[instance]['object'] and after['object']==native['widen_factory_calls'][instance]['object'],'query factory/object alias differs'
            assert before['description']==factories[instance]['output_argument']-24,'query original descriptor caller-local alias differs'
            assert before['entry_ecx']==before['object'] and before['entry_stack_words']==(before['description'],before['input_address'],before['size'],before['flags']),'query actual ECX/stack ABI differs'
            assert before['exit_esp']-before['entry_esp']==16 and before['exit_eax']==before['result'],'query actual callee cleanup/result ABI differs'
            assert before['flags']==after['flags']==0 and before['result']==after['result']==1,'query actual flags/result differ'
            assert before['entry_last_error']==before['exit_last_error']==after['entry_last_error']==after['exit_last_error']==0,'query actual LastError differs'
            source=before['input_address'];descriptor=before['description'];size=before['size']
            roots=[]
            for name in ('symbol_data','type_data','module_data'):
                row=next(row for row in data if row['name']==name)
                input_base=int.from_bytes(original['memory'][row['original']],'little')
                actual_base=int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
                if source==input_base+4 and after['input_address']==actual_base+4:roots.append((name,input_base))
            assert len(roots)==1,'query owned input buffer/offset alias differs'
            name,input_base=roots[0]
            if 'input_root' in after:assert after['input_root']==name and after['read_count']==before['read_count'],'query actual call-time read/root differs'
            inputs=[row for row in original['file_reads'][:before['read_count']] if row['buffer']==input_base]
            assert inputs and inputs[-1]['returned']==size+4 and bytes.fromhex(inputs[-1]['bytes'])[4:]==before['before'][source],'query input actual read initialization differs'
            assert size==after['size'] and before['before'][source].hex()==after['input'] and len(bytes.fromhex(after['input']))==size,'query complete initialized input differs'
            assert before['after'][source]==before['before'][source] and after['input_after']==after['input'],'query changed initialized input'
            assert before['before'][descriptor]==bytes(16) and after['description_before']=='00'*16,'query initial descriptor differs'
            assert before['after'][descriptor].hex()==after['description_after'] and len(before['after'][descriptor])==16,'query full output descriptor differs'
            assert before['after'][descriptor][8:]==bytes(8),'query unexpectedly initialized output pointers'
    if config.get('observe_widen_emit'):
        calls=original['widen_emit_calls']
        assert len(calls)==len(native.get('widen_emit_calls',[]))<=21,'emit lifetime inventory differs'
        def observed_global(name):
            row=next(row for row in data if row['name']==name)
            return int.from_bytes(original['memory'][row['original']],'little'),int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
        for instance,(before,after) in enumerate(zip(calls,native['widen_emit_calls'])):
            assert instance<len(original['widen_query_calls']) and instance<len(native['widen_query_calls']),'emit missing queried object state'
            query=original['widen_query_calls'][instance];actual_query=native['widen_query_calls'][instance]
            assert before['object']==query['object'] and after['object']==actual_query['object'],'emit queried object alias differs'
            assert before['description']==query['description'] and after['description']==actual_query['description'],'emit query descriptor alias differs'
            assert before['input_address']==query['input_address'] and after['input_address']==actual_query['input_address'],'emit query input alias differs'
            assert before['size']==after['size']==query['size']==actual_query['size'],'emit complete input extent differs'
            assert before['entry_ecx']==before['object'] and before['entry_stack_words']==(before['description'],before['input_address'],before['size'],before['flags']),'emit actual ECX/stack ABI differs'
            assert before['exit_esp']-before['entry_esp']==16 and before['exit_eax']==before['result'],'emit actual callee cleanup/result ABI differs'
            assert before['flags']==after['flags']==0 and before['result']==after['result']==1,'emit actual flags/result differ'
            assert before['entry_last_error']==before['exit_last_error']==after['entry_last_error']==after['exit_last_error']==0,'emit actual LastError differs'
            description=before['description'];source=before['input_address']
            initial=struct.unpack('<4I',before['before'][description]);actual_initial=struct.unpack('<4I',bytes.fromhex(after['description_before']))
            final=struct.unpack('<4I',before['after'][description]);actual_final=struct.unpack('<4I',bytes.fromhex(after['description_after']))
            assert initial[:2]==actual_initial[:2] and initial[:2]==struct.unpack('<4I',query['after'][query['description']])[:2],'emit queried capacities differ'
            assert (initial[2],actual_initial[2])==observed_global('pack_buffer') and (initial[3],actual_initial[3])==observed_global('pack_fixups'),'emit output allocation aliases differ'
            assert final[2:]==initial[2:] and actual_final[2:]==actual_initial[2:],'emit changed output allocation pointers'
            assert final[:2]==actual_final[:2] and final[0]<=initial[0] and final[1]<=initial[1],'emit returned initialized counts differ'
            assert before['before'][source]==query['before'][query['input_address']] and before['before'][source].hex()==after['input'],'emit complete initialized input differs'
            assert before['after'][source]==before['before'][source] and after['input_after']==after['input'],'emit changed initialized input'
            for name,index,scale in [('output',0,1),('fixups',1,8)]:
                old_pointer=initial[index+2];new_pointer=actual_initial[index+2];capacity=initial[index]*scale;written=final[index]*scale
                old_before=before['before'][old_pointer];old_after=before['after'][old_pointer]
                new_before=bytes.fromhex(after[name+'_before']);new_after=bytes.fromhex(after[name+'_after'])
                assert len(old_before)==len(old_after)==len(new_before)==len(new_after)==capacity,'emit full capacity observation differs'
                assert old_after[:written]==new_after[:written],'emit initialized output prefix differs'
                assert old_before[written:]==old_after[written:] and new_before[written:]==new_after[written:],'emit changed own unwritten capacity tail'
                if instance==len(calls)-1:
                    ended={call['address'] for call in original.get('main_free_calls',[]) if 'exit_esp' in call}
                    if old_pointer in ended:
                        assert config.get('observe_widen_frees') and any(call['address']==new_pointer and call.get('returned') for call in native.get('main_free_calls',[])),'emitted scratch lifetime lacks actual free return'
                    else:
                        assert original['memory'][old_pointer]==old_after and bytes.fromhex(native['memory'][str(new_pointer)])==new_after,'live emitted scratch buffer state differs'
            if not config.get('observe_widen_release'):
                old_input,new_input=observed_global('type_data')
                assert original['memory'][old_input][4:4+final[0]]==before['after'][initial[2]][:final[0]] and bytes.fromhex(native['memory'][str(new_input)])[4:4+final[0]]==bytes.fromhex(after['output_after'])[:final[0]],'caller copied emitted initialized bytes differ'
    if config.get('observe_widen_release'):
        calls=original['widen_release_calls'];actual=native.get('widen_release_calls',[])
        assert len(calls)==len(actual)==len(factories)==len(original['widen_query_calls'])==len(original['widen_emit_calls']),'Release complete lifetime inventory differs'
        cycle=['factory-call','factory-return','query-call','query-return','emit-call','emit-return','release-call','release-return']
        assert original['widen_sequence']==native.get('widen_sequence')==cycle*len(calls),'Widen actual lifetime sequence differs'
        for instance,(before,after) in enumerate(zip(calls,actual)):
            factory=factories[instance];emit=original['widen_emit_calls'][instance];actual_emit=native['widen_emit_calls'][instance]
            assert before['object']==factory['object'] and after['object']==native['widen_factory_calls'][instance]['object'],'Release opaque instance/object differs'
            assert before['entry_ecx']==before['object'] and before['entry_stack_words']==(0x4053ad,before['object']),'Release actual ECX/stack/caller identity differs'
            assert before['before'][before['object']]==before['table'].to_bytes(4,'little') and before['table']==factory['table'],'Release opaque vptr differs'
            assert int.from_bytes(before['before'][before['table']],'little')==original['module_bases']['mspdb50.dll']+config['observe_widen_factory']['slots'][0],'Release actual slot0 method differs'
            assert before['exit_esp']==before['entry_esp']+4 and before['exit_stack_word']==before['object'],'Release actual RET0/caller cleanup evidence differs'
            assert after['instance']==after['entry_cursor']==instance and after['entry_stage']==3 and after['exit_cursor']==instance+1 and after['exit_stage']==0,'Release native lifetime transition differs'
            assert after['entry_ecx']==after['object'],'Release native actual ECX differs'
            assert after['exit_esp']==after['entry_esp']+4,'Release native RET0 differs'
            assert before['entry_last_error']==before['exit_last_error']==after['entry_last_error']==after['exit_last_error']==0,'Release actual LastError differs'
            size=int.from_bytes(emit['after'][emit['description']][:4],'little')
            output=int.from_bytes(emit['after'][emit['description']][8:12],'little')
            assert before['copied_size']==after['copied_size']==size and before['copied']==emit['after'][output][:size].hex()==after['copied']==actual_emit['output_after'][:size*2],'Release caller copied output differs'
            assert before['copied_address']==emit['input_address'] and after['copied_address']==actual_emit['input_address'],'Release copied output source-buffer alias differs'
    parameters = {};object_parameters=[];arena_parameters=[]
    assert 'parameters' in record, 'frontier lacks explicit ABI/parameter schema'
    if config.get('require_native_argument_inventory'):
        actual=native['observed_argument_names'];expected=[item['name'] for item in record['parameters']]
        assert len(actual)==len(expected) and set(actual)==set(expected), 'actual frontier formal-argument inventory differs'
        types=native['observed_argument_types']
        assert len(types)==len(expected) and {item['name'] for item in types}==set(expected), 'actual frontier formal-type inventory differs'
        for item in record['parameters']:
            actual=next(row for row in types if row['name']==item['name'])
            integer=item['kind']=='integer'
            assert actual['kind']==('integer' if integer else 'pointer'), 'actual frontier formal type differs'
            assert actual['width']==(item.get('native_width',4) if integer else 4), 'actual frontier formal width differs'
            required=item.get('width') if item['kind']=='caller-local-pointer' else item.get('element_size') if item['kind']=='blob-element-pointer' else 4 if item['kind']=='integer-pointer' else item.get('native_pointee_width')
            if required is not None:assert actual.get('pointee_width')==required, 'actual frontier pointee extent differs'

    for item in record['parameters']:
        value = original['registers'][item['register']] if 'register' in item else original['stack'][item['slot']]
        if item['kind'] == 'integer':
            width=item.get('native_width',4)
            assert width in (1,2,4),'unsupported integer ABI width'
            parameters[item['name']] = value & ((1 << (width*8))-1)
        elif item['kind'] == 'string':
            parameters[item['name']] = dict(null=value==0,bytes=original['strings'][value].hex() if value else None)
        elif item['kind'] == 'argv':
            count = original['stack'][item['slot']-1]
            assert count < 64
            array = original['memory'][value]
            pointers = struct.unpack('<%dI'%(count+1),array)
            assert pointers[-1] == 0
            parameters[item['name']] = [original['strings'][pointer].hex() for pointer in pointers[:-1]]
        elif item['kind'] == 'integer-pointer':
            parameters[item['name']]=dict(null=value==0,bytes=original['memory'][value].hex() if value else None)
            assert not value or len(original['memory'][value])==4
        elif item['kind'] == 'global-pointer':
            if not value:
                assert item.get('nullable');parameters[item['name']]=dict(null=True)
            else:
                names=[row['name'] for row in data if row['name'] in item['allowed_globals'] and (int.from_bytes(original['memory'][row['original']],'little') if item.get('global_value') else row['original'])==value]
                assert len(names)==1,'argument is not the specified original global address'
                parameters[item['name']]=dict(null=False,global_name=names[0])
                if 'buffer_count_slot' in item:
                    size=original['stack'][item['buffer_count_slot']];row=next(row for row in data if row['name']==item['capacity_global'])
                    capacity=int.from_bytes(original['memory'][row['original']],'little');pointer=int.from_bytes(original['memory'][value],'little')
                    assert 0<=size<=capacity and len(original['memory'][pointer])==size
                    parameters[item['name']].update(size=size,capacity=capacity,bytes=original['memory'][pointer].hex())
        elif item['kind']=='global-buffer':
            names=[row['name'] for row in data if row['name'] in item['allowed_globals'] and int.from_bytes(original['memory'][row['original']],'little')+item.get('offset',0)==value]
            assert len(names)==1,'buffer argument does not alias specified original global'
            if 'count_slot' in item:size=original['stack'][item['count_slot']]
            else:
                row=next(row for row in data if row['name']=='directory_entries');root=int.from_bytes(original['memory'][row['original']],'little');entry=original['stack'][item['entry_slot']]
                assert entry>=root and (entry-root)%12==0
                payload=original['memory'][root][entry-root:entry-root+12];assert len(payload)==12
                size=int.from_bytes(payload[item['count_offset']:item['count_offset']+4],'little')
            assert 0<=size<=65536
            assert len(original['memory'][value])==size
            parameters[item['name']]=dict(global_name=names[0],size=size,bytes=original['memory'][value].hex())
            if item.get('offset'):parameters[item['name']]['offset']=item['offset']
        elif item['kind']=='blob-element-pointer':
            row=next(row for row in data if row['name']==item['root']);root=int.from_bytes(original['memory'][row['original']],'little')
            row=next(row for row in data if row['name']==item['count']);count=int.from_bytes(original['memory'][row['original']],'little')
            assert root<=value<root+count*item['element_size'] and (value-root)%item['element_size']==0,'argument is not an original directory element'
            offset=value-root;payload=original['memory'][root][offset:offset+item['element_size']]
            assert len(payload)==item['element_size']
            parameters[item['name']]=dict(root=item['root'],index=offset//item['element_size'],bytes=payload.hex())
        elif item['kind']=='type-record-pointer':
            row=next(row for row in data if row['name']==item['root']);root=int.from_bytes(original['memory'][row['original']],'little')
            row=next(row for row in data if row['name']==item['count']);count=int.from_bytes(original['memory'][row['original']],'little')
            assert root<=value<root+count*32 and (value-root)%32==0,'argument is not an owned original local type record'
            parameters[item['name']]=dict(root=item['root'],index=(value-root)//32)
        elif item['kind']=='current-symbol-record':
            def observed(name):
                row=next(row for row in data if row['name']==name)
                return int.from_bytes(original['memory'][row['original']],'little')
            base=observed(item['root']);capacity=observed(item['capacity']);offset=value-base
            assert value==observed(item['cursor']) and 0<=offset<=capacity-4,'symbol record does not alias active owned input cursor'
            buffer=original['memory'][base];length=int.from_bytes(buffer[offset:offset+2],'little')+2
            assert len(buffer)==capacity and 4<=length<=capacity-offset,'symbol record exceeds observed input capacity'
            parameters[item['name']]=dict(root=item['root'],offset=offset,bytes=buffer[offset:offset+length].hex())
        elif item['kind']=='type-record-name':
            def observed(name):
                row=next(row for row in data if row['name']==name)
                return int.from_bytes(original['memory'][row['original']],'little')
            base=int.from_bytes(original['memory'][observed('type_segments')][:4],'little');segment=original['memory'][base]
            count=observed('type_record_count');records=original['memory'][observed('type_records')]
            assert 0<=count<=32768 and len(records)>=count*32,'invalid type-name owner inventory'
            indices=[]
            for i in range(count):
                pointer=int.from_bytes(records[i*32:i*32+4],'little');offset=pointer-base
                if not 0<=offset<=len(segment)-4:continue
                kind=int.from_bytes(segment[offset+2:offset+4],'little')
                if kind not in (0x1004,0x1005,0x1006,0x1007):continue
                name_offset=16 if kind==0x1007 else 12 if kind==0x1006 else 20
                if kind!=0x1007:
                    if offset+name_offset+2>len(segment):continue
                    numeric=int.from_bytes(segment[offset+name_offset:offset+name_offset+2],'little')
                    if numeric>=0x8000:continue # Needs an encountered numeric-leaf contract.
                    name_offset+=2
                if pointer+name_offset!=value:continue
                length=segment[value-base]+1;record_size=int.from_bytes(segment[offset:offset+2],'little')+2
                assert name_offset+length<=record_size and offset+record_size<=len(segment),'type name exceeds its initialized owner record'
                indices.append(i)
            assert indices,'type name has no observed immediate-size local-record owner'
            length=segment[value-base]+1
            parameters[item['name']]=dict(segment=0,offset=value-base,record_indices=indices,bytes=segment[value-base:value-base+length].hex())
        elif item['kind']=='arena-type-record':
            arena_parameters.append((item,value))
        elif item['kind']=='type-record-data':
            def observed(name):
                row=next(row for row in data if row['name']==name)
                return int.from_bytes(original['memory'][row['original']],'little')
            base=int.from_bytes(original['memory'][observed('type_segments')][:4],'little')
            segment=original['memory'][base];root=observed('type_records');count=observed('type_record_count')
            records=original['memory'][root]
            indices=[i for i in range(count) if int.from_bytes(records[i*32:i*32+4],'little')==value]
            assert indices and base<=value<=base+len(segment)-4,'original type payload has no observed local-record/segment owner'
            offset=value-base;length=int.from_bytes(segment[offset:offset+2],'little')+2
            assert 4<=length<=len(segment)-offset,'original type payload exceeds segment capacity'
            parameters[item['name']]=dict(segment=0,offset=offset,record_indices=indices,bytes=segment[offset:offset+length].hex())
        elif item['kind']=='output-buffer':
            assert original['stack'][item['owner_return_slot']]==item['owner_return'],'output buffer original caller site differs'
            assert value==original['esp']+item['original_delta'],'output buffer original stack-local identity differs'
            size=original['stack'][item['count_slot']];assert 0<=size<=item['capacity']
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],capacity=item['capacity'],size=size)
        elif item['kind']=='local-record':
            assert original['stack'][0]==item['owner_return'] and value==original['esp']+item['original_delta'],'original record owner/caller-local identity differs'
            raw=original['memory'][value];size=int.from_bytes(raw[:2],'little')+2
            assert len(raw)==size and 2<=size<=item['capacity']
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],capacity=item['capacity'],bytes=raw.hex())
        elif item['kind']=='widen-object':
            assert len(factories)==1 and factories[0]['object']==value,'opaque original object was not returned by observed factory'
            assert factories[0]['output_argument']==original['esp']+item['original_output_delta'],'opaque original output argument caller alias differs'
            after=native['widen_factory_calls'][0];argument=native['widen_object_arguments'][item['name']]
            assert after['object']==argument['address'] and after['output_argument']==argument['output_slot'],'opaque native factory output/argument alias differs'
            parameters[item['name']]=dict(provider='MSPDB50.DLL',instance=1,state=item.get('state','created'),owner=item['owner'],local=item['local'])
        elif item['kind']=='caller-local-pointer':
            assert original['stack'][0]==item['owner_return'] and value==original['esp']+item['original_delta'],'original output argument caller-local identity differs'
            parameters[item['name']]=dict(owner=item['owner'],local=item['local'],width=item['width'])
            if item.get('initialized'):
                assert len(original['memory'][value])==item['width']
                raw=original['memory'][value]
                if item.get('widen_output'):
                    assert item['width']==16
                    words=struct.unpack('<4I',raw)
                    def root(name):return int.from_bytes(original['memory'][next(row['original'] for row in data if row['name']==name)],'little')
                    assert words[2]==root('pack_buffer') and words[3]==root('pack_fixups'),'widen original output allocation aliases differ'
                    assert 0<=words[0]<=root('pack_buffer_size') and 0<=words[1]<=root('pack_fixup_count'),'widen original output capacities exceed owned allocations'
                    parameters[item['name']]['description']=dict(bytes=words[0],fixups=words[1],output='pack_buffer',records='pack_fixups')
                else:parameters[item['name']]['bytes']=raw.hex()
        elif item['kind']=='function-pointer':
            assert value==item['original'],'original function-pointer argument differs'
            parameters[item['name']]=dict(function=item['function'])
        elif item['kind'] == 'object-pointer':
            object_parameters.append((item,value))
        elif item['kind']=='allocation-output':
            owned=[row for row in original['image_allocations'] if row['site']==item['allocation_site']]
            assert len(owned)==1,'symbol output allocation lifetime differs'
            allocation=owned[0]
            row=next(row for row in data if row['name']==item['cursor'])
            cursor=int.from_bytes(original['memory'][row['original']],'little')
            offset=value-allocation['address']
            if item.get('record_argument'):
                expected_offset=original['stack'][item['offset_slot']]
                assert offset==expected_offset and 0<=offset<=cursor-allocation['address']-4,'original reference record offset/owner differs'
                raw=original['memory'][allocation['address']];length=int.from_bytes(raw[offset:offset+2],'little')+2
                assert 4<=length<=cursor-value and len(raw)>=offset+length,'original reference record exceeds initialized output'
                parameters[item['name']]=dict(owner=item['owner'],offset=offset,capacity=allocation['size'],bytes=raw[offset:offset+length].hex())
            elif item.get('base_argument'):
                size=original['stack'][item['size_slot']]
                assert offset==0 and 0<=size<=allocation['size'] and value+size==cursor,'original initialized symbol buffer ownership/extent differs'
                assert len(original['memory'][value])>=size,'incomplete initialized symbol buffer'
                parameters[item['name']]=dict(owner=item['owner'],offset=0,capacity=allocation['size'],size=size,bytes=original['memory'][value][:size].hex())
            else:
                assert value==cursor,'symbol output argument cursor alias differs'
                assert 0<=offset<=allocation['size']-item['write_size'],'symbol output argument extent differs'
                parameters[item['name']]=dict(owner=item['owner'],offset=offset,capacity=allocation['size'],write_size=item['write_size'])
        else:
            raise AssertionError('unimplemented parameter kind: '+item['kind'])
    caller_state={}
    for item in record.get('caller_state',[]):
        assert item['width'] in (1,2,4)
        value=original['registers'][item['original_register']] & ((1 << (item['width']*8))-1)
        caller_state[item['name']]=dict(owner=item['owner'],local=item['local'],width=item['width'],native_type=item['native_type'],value=value)
    assert caller_state==native.get('caller_state',{}),'typed caller state differs'
    reads=[]
    for captured in original['file_reads']:
        row=dict(captured)
        pc=row.pop('pc');sites=[site for site in config.get('read_sites',[]) if site['return_pc']==pc]
        assert len(sites)==1,'unknown actual original read caller'
        site=sites[0];row.pop('buffer')
        if site.get('segment_input'):
            item=next(item for item in data if item['name']=='type_segments')
            table=int.from_bytes(original['memory'][item['original']],'little')
            base=int.from_bytes(original['memory'][table][:4],'little')
            item=next(item for item in data if item['name']=='type_segment_bytes')
            capacity=min(int.from_bytes(original['memory'][item['original']],'little'),0xffe8)
            assert any(allocation['address']==base and allocation['size']==capacity for allocation in original['image_allocations']), 'type read has no owned segment capacity'
            assert base<=captured['buffer'] and captured['buffer']+row['size']<=base+capacity, 'type read exceeds first segment'
            row.update(segment_index=0,segment_offset=captured['buffer']-base)
        elif site.get('allocated_global'):
            item=next(item for item in data if item['name']==site['allocated_global']);address=int.from_bytes(original['memory'][item['original']],'little')
            item=next(item for item in data if item['name']==site['capacity_global']);capacity=int.from_bytes(original['memory'][item['original']],'little')
            if 'capacity_max' in site:capacity=min(capacity,site['capacity_max'])
            if 'count_literal' in site:assert row['size']==site['count_literal']
            assert captured['buffer']==address and any(row['address']==address and row['size']==capacity for row in original['image_allocations']),'read buffer allocation/global ownership differs'
        elif site.get('count_parameter'):
            allocations=[allocation for allocation in original['module_allocations' if site.get('allocation_kind')=='module' else 'subsection_allocations'] if allocation['address']==captured['buffer']]
            assert len(allocations)==1,'subsection allocation ownership missing/ambiguous'
            allocation=allocations[0];capacity=allocation['size']
            item=next(item for item in data if item['name']=='directory_entries');root=int.from_bytes(original['memory'][item['original']],'little')
            entry=allocation['entry'];assert entry>=root and (entry-root)%12==0
            payload=original['memory'][root][entry-root:entry-root+12];assert len(payload)==12
            assert int.from_bytes(payload[8:12],'little')==capacity==row['size'],'actual subsection allocation size differs'
        elif site.get('heap_buffer'):
            item=next(item for item in data if item['name']==site['count'])
            count=int.from_bytes(original['memory'][item['original']],'little')
            capacity=(count+site['spare_elements'])*site['element_size']
            assert row['size']==count*site['element_size']
            assert original['directory_allocation']==dict(address=captured['buffer'],size=capacity),'original directory allocation capacity/ownership differs'
        else:capacity=site['capacity']
        row.update(owner=site['owner'],local=site['local'],capacity=capacity)
        assert 0<=row['size']<=capacity
        if 'returned' in row:assert row['returned']==row['size'],'partial read needs a defined-tail contract'
        reads.append(row)
    assert reads==native.get('file_reads',[]),'actual read caller/extent/position/result/initialized output differs'
    if original['directory_allocation']:
        row=next(row for row in data if row['name']=='directory_entries')
        address=int.from_bytes(original['memory'][row['original']],'little');other=int.from_bytes(bytes.fromhex(native['globals'][row['name']]),'little')
        assert original['directory_allocation']['address']==address and native['directory_allocation']['address']==other
        assert original['directory_allocation']['size']==native['directory_allocation']['size'],'actual directory allocation capacity differs'
    allocations=original['subsection_allocations'];actual_allocations=native.get('subsection_allocations',[]);allocation_aliases={}
    assert len(allocations)==len(actual_allocations),'subsection allocation inventory differs'
    if allocations:
        item=next(item for item in data if item['name']=='directory_entries');root=int.from_bytes(original['memory'][item['original']],'little')
        for left,right in zip(allocations,actual_allocations):
            assert left['entry']>=root and (left['entry']-root)%12==0
            assert left['size']==right['size'] and (left['entry']-root)//12==right['entry_index'],'subsection allocation source/size differs'
            assert left['address'] and right['address'] and left['address'] not in allocation_aliases and right['address'] not in allocation_aliases.values(),'live subsection allocation identity reused'
            allocation_aliases[left['address']]=right['address']
    module_aliases={};module_sizes={}
    assert len(original['module_allocations'])==len(native.get('module_allocations',[])),'module allocation inventory differs'
    if original['module_allocations']:
        row=next(row for row in data if row['name']=='directory_entries');root=int.from_bytes(original['memory'][row['original']],'little')
        for left,right in zip(original['module_allocations'],native['module_allocations']):
            assert left['entry']>=root and (left['entry']-root)%12==0
            assert left['size']==right['size'] and (left['entry']-root)//12==right['entry_index'],'module allocation source/size differs'
            assert left['address'] and right['address'] and left['address'] not in module_aliases and right['address'] not in module_aliases.values(),'live module buffer identity reused'
            module_aliases[left['address']]=right['address'];module_sizes[left['address']]=left['size']
    # Call-time operands remain meaningful after pack_module resets its globals
    # for a later module and reuses the source buffer. These bytes were watched
    # by the original allocation probe, independently of the final snapshot.
    rewrite_contexts=[];watched={};read_count=0;watching=False
    addresses={row['name']:row['original'] for row in data}
    for line in events.splitlines():
        if line.startswith('file-read-call '):read_count+=1
        if line.startswith('image-allocation-entry 0040a7d1 '):watching=True;watched={}
        elif watching and line.startswith('memory '):
            _,address,contents=line.split();watched[int(address,16)]=bytes.fromhex(contents)
        elif watching and line.startswith('image-allocation-call 0040a7d1 '):
            def captured(name):return int.from_bytes(watched[addresses[name]],'little')
            rewrite_contexts.append(dict(entry_index=captured('line_entry_index'),info=captured('current_module_info'),
                symbols=captured('module_symbol_count'),references=captured('module_reference_count'),
                input=captured('type_data'),read_count=read_count))
            watching=False
    image_aliases={};image_sizes={};rewrite_owners={};rewrite_index=0
    assert len(original['image_allocations'])==len(native.get('image_allocations',[])),'image allocation inventory differs'
    for left,right in zip(original['image_allocations'],native.get('image_allocations',[])):
        sites={0x4062b0:('debug_directory','module_capacity'),0x4062d3:('symbol_data','symbol_capacity'),
               0x4062f7:('type_data','line_capacity'),0x40631b:('module_data','type_capacity'),0x40639d:None,0x40a7d1:None}
        assert left['site'] in sites,'unknown actual image allocator caller'
        if sites[left['site']] is not None:
            name,size_name=sites[left['site']]
            row=next(row for row in data if row['name']==name)
            assert int.from_bytes(original['memory'][row['original']],'little')==left['address'],'actual allocation not assigned to its evidenced output'
            row=next(row for row in data if row['name']==size_name)
            assert int.from_bytes(original['memory'][row['original']],'little')==left['size'],'actual allocation request differs from capacity source'
        if left['site']==0x40a7d1:
            assert rewrite_index<len(rewrite_contexts),'symbol rewrite call-time context lifetime differs'
            context=rewrite_contexts[rewrite_index];rewrite_index+=1
            # The directory entry size is replaced with the packed size after
            # rewriting; retain its input extent from the actual earlier read.
            symbol_reads=[row for row in original['file_reads'][:context['read_count']] if row['pc']==0x4050b8 and row['buffer']==context['input']]
            assert symbol_reads,'symbol rewrite input read lifetime differs'
            size=symbol_reads[-1]['returned'];assert size==symbol_reads[-1]['size'],'symbol rewrite input read is incomplete'
            directory_reads=[row for row in original['file_reads'] if row['pc']==0x40588b]
            assert len(directory_reads)==1,'symbol directory input read lifetime differs'
            entries=bytes.fromhex(directory_reads[0]['bytes']);index=context['entry_index']
            directory=int.from_bytes(original['memory'][addresses['directory_entries']],'little')
            identity=original['memory'][directory][index*12:index*12+4]
            matches=[entries[i:i+12] for i in range(0,len(entries),12) if entries[i:i+4]==identity]
            assert len(matches)==1 and int.from_bytes(matches[0][8:12],'little')==size,'symbol rewrite original directory/read extent differs'
            assert size+context['symbols']-context['references']==left['size'],'symbol rewrite allocation expression differs'
            assert right.get('owner')=='rewrite_module_symbols','symbol output allocation owner differs'
            module=int.from_bytes(original['memory'][context['info']][:2],'little')
            source=dict(entry_index=index,module=module,input_size=size,symbols=context['symbols'],references=context['references'])
            # Historical pairs ending before module rollover already validated
            # these same operands against then-live globals; new observers also
            # retain independent actual DWARF call-time values.
            if 'source' in right:assert right['source']==source,'symbol rewrite actual call-time operands differ'
            else:
                assert context['info']==int.from_bytes(original['memory'][addresses['current_module_info']],'little'),'historical symbol rewrite proof ends before module rollover'
                assert context['symbols']==int.from_bytes(original['memory'][addresses['module_symbol_count']],'little') and context['references']==int.from_bytes(original['memory'][addresses['module_reference_count']],'little'),'historical symbol rewrite call-time operand limit'
            rewrite_owners[left['address']]=dict(context,**{'module':module})
        assert left['size']==right['size'] and left['address'] and right['address'],'actual image allocation request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(),'live image allocation identity reused'
        image_aliases[left['address']]=right['address'];image_sizes[left['address']]=left['size']
    assert len(original['checked_allocations'])==len(native.get('checked_allocations',[])),'checked allocator request inventory differs'
    for left,right in zip(original['checked_allocations'],native.get('checked_allocations',[])):
        assert left['size']==right['size'] and left['address'] and right['address'],'checked allocator request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(),'live checked allocation identity reused'
        image_aliases[left['address']]=right['address'];image_sizes[left['address']]=left['size']
    assert len(original['cleared_allocations'])==len(native.get('cleared_allocations',[])), 'cleared allocator inventory differs'
    for left,right in zip(original['cleared_allocations'],native.get('cleared_allocations',[])):
        assert left['size']==right['size'] and left['address'] and right['address'], 'cleared allocation request/result differs'
        assert left['address'] not in image_aliases and right['address'] not in image_aliases.values(), 'live cleared allocation identity reused'
        image_aliases[left['address']]=right['address'];image_sizes[left['address']]=left['size']
    retired={};free_lifetimes=dict(calls=0,retired_allocations=0)
    if config.get('observe_main_frees'):
        import importlib.util
        from pathlib import Path
        spec=importlib.util.spec_from_file_location('main_free_state',Path(__file__).with_name('main-free-state.py'))
        free_state=importlib.util.module_from_spec(spec);spec.loader.exec_module(free_state)
        retired,free_lifetimes=free_state.validate(config,data,original,native,image_aliases,image_sizes,reads)
    def historical_segment(base):
        # Only former-input initialization/extent proof uses the saved PRE-free
        # observation. It is not current payload state and is never reinstalled
        # into either raw memory namespace.
        if base in retired:
            assert retired[base]['name']=='line_data','unknown historical segment lifetime role'
            return retired[base]['before']
        return original['memory'][base]
    used_uninitialized=set()
    aliases,reverse={},{}
    handle_aliases,handle_reverse={},{}
    original_calls=original['file_calls'];native_calls=native.get('file_calls',[])
    assert len(original_calls)==len(native_calls),'actual file-open call inventory differs'
    for left,right in zip(original_calls,native_calls):
        assert {key:value for key,value in left.items() if key!='handle'}=={key:value for key,value in right.items() if key!='handle'},'actual file-open ABI/input/LastError differs'
        assert bool(left['handle'])==bool(right['handle'])
        assert left['handle'] not in handle_aliases and right['handle'] not in handle_reverse,'file HANDLE lifetime needs explicit model'
        handle_aliases[left['handle']]=right['handle'];handle_reverse[right['handle']]=left['handle']
    def file_handle(left,right):
        assert bool(left)==bool(right),'file HANDLE NULL state differs'
        if not left:return
        assert handle_aliases[left]==right and handle_reverse[right]==left,'file HANDLE ownership/alias differs'
        expected=original['file_handles'][left];actual=native['file_handles'][str(right)]
        assert expected=={key:actual[key] for key in ('type','size','position')},'file HANDLE type/size/position differs'
        call=next(row for row in original_calls if row['handle']==left)
        pathname=bytes.fromhex(call['pathname']).rstrip(b'\0').decode('latin1')
        assert actual['pathname']==pathname,'file HANDLE frozen pathname differs'
        access=call['access'];mode=2 if access&0x40000000 and access&0x80000000 else 1 if access&0x40000000 else 0
        assert actual['access_mode']==mode,'file HANDLE descriptor access differs'
        assert artifacts is not None and pathname in artifacts,'missing actual original file contents'
        content=artifacts[pathname]
        assert len(content)==expected['size'] and hashlib.sha256(content).hexdigest()==actual['sha256'],'file HANDLE complete contents differ'

    mapping_aliases={};view_roles={};live_mappings=set()
    assert len(original['mapping_calls'])==len(native.get('mapping_calls',[])),'actual mapping call inventory differs'
    for index,(left,right) in enumerate(zip(original['mapping_calls'],native.get('mapping_calls',[]))):
        assert left['kind']==right['kind'] and left['last_error']==right['last_error'],'mapping call kind/LastError differs'
        kind=left['kind'];source=left['args'];actual=right['args']
        if kind=='create':
            assert handle_aliases[source[0]]==actual[0] and source[1:]==actual[1:],'mapping source/ABI/input differs'
            assert bool(left['result'])==bool(right['result'])
            if left['result']:
                assert left['result'] not in mapping_aliases,'mapping HANDLE generation needs explicit reuse model'
                mapping_aliases[left['result']]=right['result'];live_mappings.add(left['result'])
                assert right['registry']==dict(length=source[4] or original['file_handles'][source[0]]['size'],writable=int(source[2]==4),source_owned=True),'typed mapping registry state differs'
        elif kind=='map':
            assert source[0] in live_mappings and mapping_aliases[source[0]]==actual[0] and source[1:]==actual[1:],'view source/ABI/input differs'
            assert not source[5],'fixed mapping base needs explicit address-role proof'
            assert bool(left['result'])==bool(right['result'])
            if left['result']:
                assert left['result'] not in view_roles,'mapped view generation needs explicit reuse model'
                creation=next(row for row in original['mapping_calls'] if row['kind']=='create' and row['result']==source[0])
                view_roles[left['result']]=dict(address=right['result'],index=index,creation=creation,arguments=source)
        elif kind=='close':
            assert source[0] in live_mappings and mapping_aliases[source[0]]==actual[0] and left['result']==right['result']==1,'mapping ownership/close result differs'
            live_mappings.remove(source[0])
        else:raise AssertionError('unknown mapping call kind')
    def mapped_view(address,other,length):
        assert bool(address)==bool(other),'mapped view NULL state differs'
        if not address:return
        role=view_roles[address];assert role['address']==other,'mapped view alias/generation differs'
        left=original['mapped_views'][address];right=native['mapped_views'][str(other)]
        assert left['length']==right['length']==length and left['base']==address and left['region']>=length,'mapped view extent/base differs'
        assert left['state']==0x1000 and left['type']==0x40000,'original view is not committed mapped memory'
        creation=role['creation'];args=role['arguments']
        assert left['protection']==creation['args'][2],'original actual view protection differs'
        assert right['permissions']==('rw-s' if left['protection']==4 else 'r--s'),'native actual view protection/sharing differs'
        offset=(args[2]<<32)+args[3]
        assert right['offset']==offset and right['call_index']==role['index'],'mapped view offset/generation differs'
        owner=next(row for row in original_calls if row['handle']==creation['args'][0])
        pathname=bytes.fromhex(owner['pathname']).rstrip(b'\0').decode('latin1')
        assert right['pathname']==pathname,'mapped view backing pathname differs'
        assert views is not None,'missing complete mapped-memory observations'
        before=views['original'][left['file']];actual=views['native'][right['file']]
        assert len(before)==len(actual)==length and before==actual,'complete mapped bytes differ'
        assert hashlib.sha256(actual).hexdigest()==right['sha256'],'native mapped bytes do not bind raw observation'
        assert artifacts is not None and before==artifacts[pathname][offset:offset+length],'view contents differ from observed backing file'
    def pair(address,other):
        assert bool(address)==bool(other),'pointer NULL state differs'
        if not address:return False
        if address in aliases:
            assert aliases[address]==other and reverse[other]==address,'object alias differs'
            return False
        assert other not in reverse,'native object aliases distinct original objects'
        aliases[address]=other;reverse[other]=address
        return True
    def allocation(address,other,spec):
        assert bool(address)==bool(other),'uninitialized allocation NULL state differs'
        if not address:return
        item=next(item for item in data if item['name']==spec['size_global'])
        size=int.from_bytes(original['memory'][item['original']],'little')*spec.get('element_size',1)
        if 'maximum' in spec:size=min(size,spec['maximum'])
        assert image_aliases[address]==other and image_sizes[address]==size,'uninitialized allocation ownership/capacity differs'
        used_uninitialized.add(address)
        pair(address,other)
    def module_buffer(address,other,length=None):
        assert bool(address)==bool(other),'module buffer NULL state differs'
        if not address:return
        assert module_aliases[address]==other,'module payload allocation alias differs'
        if length is None:length=module_sizes[address]
        assert module_sizes[address]==length,'module payload capacity differs'
        left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
        assert len(left)==len(right)==length and left==right,'complete initialized module payload differs'
        pair(address,other)
    def linked(address,other,shape):
        count=0
        while pair(address,other):
            count+=1
            assert count<=128,'unbounded linked object chain'
            left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
            assert len(left)==len(right)==shape['size']
            offset=shape['link'];assert 0<=offset<=shape['size']-4
            excluded=set(range(offset,offset+4))
            if 'string' in shape:
                text=shape['string'];assert 0<=text<=shape['size']-4
                excluded.update(range(text,text+4))
                source=int.from_bytes(left[text:text+4],'little');destination=int.from_bytes(right[text:text+4],'little')
                pair(source,destination)
                if source:assert original['strings'][source].hex()==native['strings'][str(destination)],'target object string differs'
            if 'handle' in shape:
                handle=shape['handle'];excluded.update(range(handle,handle+4))
                file_handle(int.from_bytes(left[handle:handle+4],'little'),int.from_bytes(right[handle:handle+4],'little'))
            if 'view' in shape:
                spec=shape['view'];offset_view=spec['offset'];excluded.update(range(offset_view,offset_view+4))
                mapped_view(int.from_bytes(left[offset_view:offset_view+4],'little'),int.from_bytes(right[offset_view:offset_view+4],'little'),int.from_bytes(left[spec['length_offset']:spec['length_offset']+4],'little'))
            if 'payload' in shape:
                spec=shape['payload'];payload_offset=spec['offset'];excluded.update(range(payload_offset,payload_offset+4))
                module_buffer(int.from_bytes(left[payload_offset:payload_offset+4],'little'),int.from_bytes(right[payload_offset:payload_offset+4],'little'),int.from_bytes(left[spec['length_offset']:spec['length_offset']+4],'little'))
            # Actual ModuleInfo layout: rewritten symbols at20, length24. The
            # original module payload remains separately owned at12/16.
            if shape['size']==48 and shape['link']==44 and shape.get('payload')==dict(offset=12,length_offset=16):
                excluded.update(range(20,24))
                symbols=int.from_bytes(left[20:24],'little');actual_symbols=int.from_bytes(right[20:24],'little')
                assert bool(symbols)==bool(actual_symbols),'module rewritten symbols NULL state differs'
                if symbols:
                    allocations=[row for row in original['image_allocations'] if row['site']==0x40a7d1 and row['address']==symbols]
                    assert len(allocations)==1 and allocations[0]['address']==symbols,'module rewritten symbols allocation lifetime differs'
                    allocation=allocations[0];size=int.from_bytes(left[24:28],'little')
                    assert image_aliases[symbols]==actual_symbols and 0<=size<=allocation['size'],'module rewritten symbols allocation/size differs'
                    assert rewrite_owners[symbols]['info']==address,'module rewritten symbols actual owner differs'
                    if address==global_integer('current_module_info'):
                        assert symbols+size==global_integer('symbol_output'),'module rewritten symbols initialized extent differs'
                    assert original['memory'][symbols][:size]==bytes.fromhex(native['memory'][str(actual_symbols)])[:size],'module rewritten symbol bytes differ'
                    pair(symbols,actual_symbols);used_uninitialized.add(symbols)
                excluded.update(range(28,32))
                lines=int.from_bytes(left[28:32],'little');actual_lines=int.from_bytes(right[28:32],'little')
                assert bool(lines)==bool(actual_lines),'module source lines NULL state differs'
                if lines:
                    assert allocation_aliases.get(lines)==actual_lines,'module source lines allocation alias differs'
                    size=int.from_bytes(left[32:36],'little')
                    assert len(original['memory'][lines])==len(bytes.fromhex(native['memory'][str(actual_lines)]))==size,'module source lines allocation extent differs'
                    assert original['memory'][lines]==bytes.fromhex(native['memory'][str(actual_lines)]),'module source lines initialized bytes differ'
                    source=global_integer('module_data')
                    historical=retired[source]['before'] if source in retired else original['memory'][source]
                    assert original['memory'][lines]==historical[:size],'module source lines copied input differs'
                    pair(lines,actual_lines)
            assert all(a==b for i,(a,b) in enumerate(zip(left,right)) if i not in excluded),'target object bytes differ'
            address=int.from_bytes(left[offset:offset+4],'little')
            other=int.from_bytes(right[offset:offset+4],'little')
    def global_integer(name):
        item=next(item for item in data if item['name']==name)
        return int.from_bytes(original['memory'][item['original']],'little')
    # Arena capacities are captured in full, but only each block's used prefix
    # is initialized. Preserve allocation and chain identity independently.
    arena_payloads={};arena_nodes=set()
    for arena_item in data:
        if not arena_item.get('type_arena'):continue
        a=original['memory'][arena_item['original']];b=bytes.fromhex(native['globals'][arena_item['name']])
        assert len(a)==len(b)==24,'incomplete arena header'
        av=struct.unpack('<6I',a);bv=struct.unpack('<6I',b)
        assert av[0:2]==bv[0:2] and av[4]==bv[4],'arena capacity/count/remaining differs'
        assert av[0] in (0x1000,0x10000) and av[1]<=1024,'invalid arena capacity/block count'
        source=av[5];destination=bv[5];visited=[]
        while source or destination:
            assert source and destination and source not in arena_nodes,'arena chain NULL/identity/cycle differs'
            arena_nodes.add(source);visited.append(source);pair(source,destination)
            assert image_aliases[source]==destination and image_sizes[source]==12,'arena block header allocation differs'
            assert any(row['address']==source for row in original['cleared_allocations']),'arena header was not cleared'
            left=original['memory'][source];right=bytes.fromhex(native['memory'][str(destination)])
            assert len(left)==len(right)==12,'incomplete arena block header'
            x=struct.unpack('<3I',left);y=struct.unpack('<3I',right)
            assert x[0]==y[0] and 0<=x[0]<=av[0],'arena initialized extent differs'
            assert x[1] in image_aliases and image_aliases[x[1]]==y[1] and image_sizes[x[1]]==av[0],'arena payload allocation differs'
            before=original['memory'][x[1]];after=bytes.fromhex(native['memory'][str(y[1])])
            assert len(before)==len(after)==av[0],'incomplete arena payload capacity'
            assert before[:x[0]]==after[:y[0]],'initialized arena payload bytes differ'
            assert x[1] not in arena_payloads,'arena payload identity reused'
            arena_payloads[x[1]]=dict(native=y[1],used=x[0],capacity=av[0],bytes=before,
                arena=arena_item['name'],block_index=len(visited)-1)
            used_uninitialized.add(x[1]);pair(x[1],y[1])
            source,destination=x[2],y[2]
        assert len(visited)==av[1],'arena chain inventory differs'
        if visited:
            last=visited[-1];actual_last=aliases[last];last_fields=struct.unpack('<3I',original['memory'][last])
            assert av[2]==last and bv[2]==actual_last,'arena last-block alias differs'
            assert av[3]==last_fields[1]+last_fields[0] and bv[3]==image_aliases[last_fields[1]]+last_fields[0],'arena cursor alias/offset differs'
            assert av[4]==av[0]-last_fields[0],'arena remaining/used extent differs'
        else:assert not any(av[2:]) and not any(bv[2:]),'empty arena has live state'
    def arena_payload(source,destination,length=None):
        owners=[(base,row) for base,row in arena_payloads.items() if base<=source<base+row['used']]
        assert len(owners)==1,'type payload has no initialized arena owner'
        base,row=owners[0];offset=source-base
        assert destination==row['native']+offset,'type payload arena/offset alias differs'
        if length is None:
            assert offset+2<=row['used'],'incomplete arena record length'
            length=int.from_bytes(row['bytes'][offset:offset+2],'little')+2
            assert length>=4,'invalid arena type record length'
        assert 0<=length and offset+length<=row['used'],'type payload exceeds initialized arena prefix'
        return row['bytes'][offset:offset+length]
    for spec,address in arena_parameters:
        owners=[(base,row) for base,row in arena_payloads.items() if row['arena']==spec['arena'] and base<=address<base+row['used']]
        assert len(owners)==1,'arena argument has no initialized owned block'
        base,row=owners[0];offset=address-base
        assert offset+2<=row['used'],'arena argument lacks initialized length'
        length=int.from_bytes(row['bytes'][offset:offset+2],'little')+2
        assert 4<=length<=row['used']-offset,'arena argument exceeds initialized record extent'
        index=parameters[spec['index_parameter']]
        assert 0x1000<=index<global_integer('next_global_type'),'arena argument index out of range'
        records=original['memory'][global_integer('type_records')];count=global_integer('type_record_count')
        indices=[i for i in range(count) if int.from_bytes(records[i*32:i*32+4],'little')==address and int.from_bytes(records[i*32+4:i*32+8],'little')==index]
        assert indices,'arena argument has no mapped local-record owner'
        root=next(item['original'] for item in data if item['name']=='global_type_blocks')
        assert global_integer('type_table_used')==512 and index-0x1000<512,'arena argument block inventory needs explicit contract'
        block=int.from_bytes(original['memory'][root][:4],'little');slot=(index&511)*4
        assert int.from_bytes(original['memory'][block][slot:slot+4],'little')==address,'arena argument/global index slot alias differs'
        parameters[spec['name']]=dict(arena=spec['arena'],block_index=row['block_index'],offset=offset,
            record_indices=indices,index=index,bytes=row['bytes'][offset:offset+length].hex())
    typed_records_checked=0
    extended_references_checked=0
    recursive_type_nodes_checked=0
    reference_symbol_nodes_checked=0
    interned_type_nodes_checked=0
    pooled_interned_nodes_checked=0
    cached_type_names_checked=0
    globals_checked = []
    for item in data:
        raw = original['memory'][item['original']]
        assert len(raw)==item['size']
        actual = bytes.fromhex(native['globals'][item['name']])
        assert len(actual)==item['size']
        if item['kind']=='pointer-array':
            shape=item['shape'];count=shape['count_literal']
            assert len(raw)==len(actual)==count*4
            for index in range(count):
                linked(int.from_bytes(raw[index*4:index*4+4],'little'),int.from_bytes(actual[index*4:index*4+4],'little'),shape)
        elif item['kind'] == 'pointer':
            if item.get('recursive_types'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert image_aliases[address]==other and image_sizes[address]==16384,'recursive-type bucket allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==16384,'incomplete recursive-type bucket observation'
                    counts=original['memory'][global_integer('type_table_primary_counts')]
                    assert len(counts)==8192,'incomplete recursive-type bucket counts'
                    records=original['memory'][global_integer('type_records')];visited=set()
                    for offset in range(0,16384,4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little');count=0
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<4096,'recursive-type node NULL/identity/chain differs'
                            visited.add(source);pair(source,destination);count+=1
                            owners=[base for base,size in image_sizes.items() if size==0x3000 and base<=source and source+12<=base+size and (source-base)%12==0]
                            assert len(owners)==1,'recursive-type node has no actual pool ownership'
                            owner=owners[0]
                            assert destination==image_aliases[owner]+source-owner,'recursive-type pool/offset alias differs'
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert len(a)==len(b)==12,'incomplete initialized recursive-type node'
                            index=int.from_bytes(a[4:8],'little')
                            assert a[4:8]==b[4:8] and 0x1000<=index<global_integer('next_global_type'),'recursive-type index differs'
                            payload=int.from_bytes(a[:4],'little');actual_payload=int.from_bytes(b[:4],'little')
                            arena_payload(payload,actual_payload)
                            matches=[records[i:i+32] for i in range(0,global_integer('type_record_count')*32,32) if int.from_bytes(records[i:i+4],'little')==payload and int.from_bytes(records[i+4:i+8],'little')==index]
                            assert matches and any(int.from_bytes(row[14:16],'little')==offset//4 for row in matches),'recursive-type record/hash bucket differs'
                            source=int.from_bytes(a[8:12],'little');destination=int.from_bytes(b[8:12],'little')
                            used_uninitialized.add(owner);recursive_type_nodes_checked+=1
                        assert count==int.from_bytes(counts[offset//2:offset//2+2],'little'),'recursive-type bucket count differs'
            elif item.get('interned_types'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert image_aliases[address]==other and image_sizes[address]==16384,'interned-type bucket allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==16384,'incomplete interned-type bucket observation'
                    visited=set();counts_address=global_integer('type_table_secondary_counts')
                    counts=original['memory'][counts_address]
                    for offset in range(0,16384,4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little');count=0
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<4096,'interned-type node NULL/identity/chain differs'
                            visited.add(source);count+=1;pair(source,destination)
                            if image_sizes.get(source)==12:
                                assert image_aliases[source]==destination,'interned-type node allocation differs'
                            else:
                                assert any(row['name']=='interned_node_cursor' for row in data),'interned-type pool needs an observed contract'
                                owners=[base for base,size in image_sizes.items() if size==0x3000 and base<=source and source+12<=base+size and (source-base)%12==0]
                                assert len(owners)==1,'interned-type node has no actual pool ownership'
                                owner=owners[0]
                                remaining=global_integer('interned_nodes_remaining')
                                cursor=global_integer('interned_node_cursor')
                                assert 0<=remaining<=1024 and cursor-owner==(1024-remaining)*12,'interned-type active pool/cursor ownership differs'
                                assert source+12<=cursor,'interned-type node exceeds initialized pool prefix'
                                assert destination==image_aliases[owner]+source-owner,'interned-type pool/offset alias differs'
                                used_uninitialized.add(owner);pooled_interned_nodes_checked+=1
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert len(a)==len(b)==12 and a[:4]==b[:4],'interned-type index/extent differs'
                            index=int.from_bytes(a[:4],'little');assert 0x1000<=index<global_integer('next_global_type'),'interned-type index out of range'
                            payload=int.from_bytes(a[4:8],'little');actual_payload=int.from_bytes(b[4:8],'little')
                            arena_payload(payload,actual_payload)
                            block_root=next(row['original'] for row in data if row['name']=='global_type_blocks')
                            assert index-0x1000<512,'interned-type block inventory needs an explicit contract'
                            block=int.from_bytes(original['memory'][block_root][:4],'little')
                            assert int.from_bytes(original['memory'][block][(index&511)*4:(index&511)*4+4],'little')==payload,'interned-type/global slot alias differs'
                            source=int.from_bytes(a[8:12],'little');destination=int.from_bytes(b[8:12],'little')
                            interned_type_nodes_checked+=1
                        assert count==int.from_bytes(counts[offset//2:offset//2+2],'little'),'interned-type bucket count differs'
            elif item.get('type_name_cache'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert image_aliases[address]==other and image_sizes[address]==2048,'type-name bucket allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==2048,'incomplete type-name bucket observation'
                    visited=set();cleared={row['address'] for row in original['cleared_allocations']}
                    checked={row['address'] for row in original['checked_allocations']}
                    table=global_integer('type_segments');base=int.from_bytes(original['memory'][table][:4],'little')
                    for offset in range(0,2048,4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<512,'type-name node NULL/identity/chain differs'
                            visited.add(source);pair(source,destination)
                            assert source in cleared and image_aliases[source]==destination and image_sizes[source]==20,'type-name node allocation differs'
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert len(a)==len(b)==20,'incomplete type-name node observation'
                            assert a[8:16]==b[8:16],'type-name index/reserved fields differ'
                            name=int.from_bytes(a[:4],'little');actual_name=int.from_bytes(b[:4],'little')
                            pair(name,actual_name)
                            if name:
                                text=original['memory'][name];actual_text=bytes.fromhex(native['memory'][str(actual_name)])
                                assert len(text)==len(actual_text)==text[0]+1==actual_text[0]+1,'incomplete type-name counted string observation'
                                if item['type_name_cache']=='global':
                                    assert arena_payload(name,actual_name,len(text))==text,'global counted name bytes/owner differ'
                                else:assert name in checked and image_aliases[name]==actual_name and image_sizes[name]==len(text),'type-name counted string allocation differs'
                                assert text==actual_text,'type-name counted string bytes differ'
                                assert sum(text)&511==offset//4,'type-name bucket/hash differs'
                            payload=int.from_bytes(a[4:8],'little');actual_payload=int.from_bytes(b[4:8],'little')
                            assert bool(payload)==bool(actual_payload),'type-name record NULL state differs'
                            if payload:
                                if item['type_name_cache']=='global':arena_payload(payload,actual_payload)
                                else:
                                    displacement=payload-base;segment=historical_segment(base)
                                    assert 0<=displacement<=len(segment)-4,'type-name record has no observed segment owner'
                                    length=int.from_bytes(segment[displacement:displacement+2],'little')+2
                                    assert length>=4 and any(row.get('segment_index')==0 and row['segment_offset']<=displacement and displacement+length<=row['segment_offset']+row['size'] for row in reads),'type-name record exceeds initialized segment'
                                    assert actual_payload==image_aliases[base]+displacement,'type-name record segment alias differs'
                            source=int.from_bytes(a[16:20],'little');destination=int.from_bytes(b[16:20],'little')
                            cached_type_names_checked+=1
            elif item.get('type_record_table'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    assert global_integer('type_module_count')==1, 'reused type-table lifetime needs an explicit state contract'
                    capacity=global_integer('type_record_capacity');count=global_integer('type_record_count')
                    assert 0<=count<=capacity<=32768, 'type record count/capacity differs'
                    assert image_aliases[address]==other and image_sizes[address]==capacity*32, 'type record table allocation differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==capacity*32, 'incomplete initialized type record table'
                    table=global_integer('type_segments');base=int.from_bytes(original['memory'][table][:4],'little')
                    segment=historical_segment(base);destination=image_aliases[base]
                    initialized=[(row['segment_offset'],row['segment_offset']+row['size']) for row in reads if row.get('segment_index')==0]
                    for index in range(capacity):
                        a=left[index*32:index*32+32];b=right[index*32:index*32+32]
                        if index>=count:
                            assert not any(a) and not any(b), 'unused type record capacity is not cleared'
                            continue
                        source=int.from_bytes(a[:4],'little');target=int.from_bytes(b[:4],'little')
                        assert bool(source)==bool(target), 'type record payload NULL state differs'
                        if source:
                            offset=source-base
                            if 0<=offset<=len(segment)-4:
                                length=int.from_bytes(segment[offset:offset+2],'little')+2
                                assert length>=4 and any(first<=offset and offset+length<=last for first,last in initialized), 'type record exceeds initialized segment input'
                                assert target==destination+offset, 'type record segment/payload offset alias differs'
                            else:arena_payload(source,target)
                        assert a[4:16]==b[4:16] and a[20:24]==b[20:24] and a[28:]==b[28:], 'initialized type record fields differ'
                        if a[29]&1:
                            references=int.from_bytes(a[12:14],'little')
                            cleared={row['address'] for row in original['cleared_allocations']}
                            for field,minimum in [(16,references*2),(24,(references+7)//8)]:
                                child=int.from_bytes(a[field:field+4],'little');actual_child=int.from_bytes(b[field:field+4],'little')
                                assert child in cleared and child in image_aliases and image_aliases[child]==actual_child,'extended type reference allocation/alias differs'
                                size=image_sizes[child]
                                assert minimum<=size<=65536,'extended type reference allocation capacity differs'
                                before=original['memory'][child];after=bytes.fromhex(native['memory'][str(actual_child)])
                                assert len(before)==len(after)==size,'incomplete extended type reference allocation'
                                assert before==after,'initialized extended type reference bytes differ'
                                assert not any(before[minimum:]) and not any(after[minimum:]),'unused cleared type reference capacity differs'
                                pair(child,actual_child)
                            extended_references_checked+=1
                        else:
                            assert a[16:28]==b[16:28], 'initialized inline type reference bytes differ'
                    typed_records_checked=count
            elif item.get('segment_buffer'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                assert bool(address)==bool(other),'type segment buffer NULL state differs'
                if address:
                    row=next(row for row in data if row['name']=='type_segments');table=int.from_bytes(original['memory'][row['original']],'little')
                    base=int.from_bytes(original['memory'][table][:4],'little')
                    assert address==base and image_aliases[base]==other,'type segment buffer/allocator alias differs'
                    if address in retired:
                        pair(address,other);globals_checked.append(item['name']);continue
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    if item.get('segment_contents'):
                        capacity=min(global_integer('type_segment_bytes'),0xffe8)
                        assert len(left)==len(right)==capacity, 'incomplete type segment capacity observation'
                        defined=[(0,4)]+[(row['segment_offset'],row['segment_offset']+row['size']) for row in reads if row.get('segment_index')==0]
                        for first,last in defined:
                            assert 0<=first<=last<=capacity and left[first:last]==right[first:last], 'initialized type segment bytes differ'
                    else:
                        assert len(left)==len(right)==item['blob']['size'] and left==right,'initialized type segment prefix differs'
                    pair(address,other)
            elif item.get('public_buckets'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if pair(address,other):
                    # Earlier public-only captures predate the complete cleared-allocator inventory.
                    if any(row['name']=='global_symbol_count' for row in data):
                        assert image_aliases[address]==other and image_sizes[address]==16384,'symbol bucket allocation ownership/capacity differs'
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==4096*4,'incomplete symbol bucket observation'
                    visited=set()
                    for offset in range(0,len(left),4):
                        source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                        while source or destination:
                            assert source and destination and source not in visited and len(visited)<512,'symbol node NULL/identity/chain differs'
                            visited.add(source);pair(source,destination)
                            owners=[base for base,size in image_sizes.items() if size==0x1fa4 and base<=source<base+size]
                            assert len(owners)==1,'public node has no actual arena ownership'
                            owner=owners[0];assert destination==image_aliases[owner]+source-owner,'public node arena/offset differs'
                            a=original['memory'][source];b=bytes.fromhex(native['memory'][str(destination)])
                            assert 20<=len(a)==len(b)==18+int.from_bytes(a[16:18],'little')<=1042 and source+len(a)<=owner+image_sizes[owner],'incomplete or unowned initialized symbol node extent'
                            kind=int.from_bytes(a[18:20],'little')
                            if item.get('reference_records') and kind in (0x400,0x401):
                                assert len(a)==32 and a[16:18]==b'\x0e\x00','reference node extent/type differs'
                                assert a[4:30]==b[4:30],'initialized reference node fields differ'
                                assert a[20:24]==a[4:8],'reference checksum/header hash alias differs'
                                identifier=int.from_bytes(a[28:30],'little')
                                allocations=[row for row in original['image_allocations'] if row['site']==0x40a7d1 and rewrite_owners[row['address']]['module']==identifier]
                                assert len(allocations)==1,'reference source allocation lifetime differs'
                                source_allocation=allocations[0];base=source_allocation['address'];position=int.from_bytes(a[24:28],'little')
                                assert int.from_bytes(a[28:30],'little')==rewrite_owners[base]['module'],'reference module owner differs'
                                owner_info=original['memory'][rewrite_owners[base]['info']]
                                size=int.from_bytes(owner_info[24:28],'little')
                                assert 4<=position<=size-4,'reference source offset exceeds initialized output'
                                before=original['memory'][base];after=bytes.fromhex(native['memory'][str(image_aliases[base])])
                                length=int.from_bytes(before[position:position+2],'little')+2
                                assert 4<=length<=size-position,'reference source record exceeds initialized output'
                                assert before[position:position+length]==after[position:position+length],'reference source record bytes differ'
                                name=int.from_bytes(a[12:14],'little')
                                assert 4<=name<length and name+1+before[position+name]<=length,'reference borrowed counted name exceeds source record'
                                source_kind=int.from_bytes(before[position+2:position+4],'little')
                                assert (kind==0x400 and source_kind in (0x104,0x105,0x100a,0x100b,0x1010,0x1011)) or (kind==0x401 and source_kind in (0x101,0x102,0x1007,0x1008,0x100e,0x100f)),'reference kind/source record differs'
                                reference_symbol_nodes_checked+=1
                            else:
                                assert a[4:]==b[4:],'initialized public node header/record/padding differs'
                                name=16+int.from_bytes(a[12:14],'little')
                                assert 20<=name<len(a) and name+1+a[name]<=len(a),'symbol node counted name exceeds initialized record'
                            assert int.from_bytes(a[4:8],'little')%4096==offset//4,'symbol node hash/bucket differs'
                            source=int.from_bytes(a[:4],'little');destination=int.from_bytes(b[:4],'little')
            elif item.get('written_allocation'):
                spec=item['written_allocation'];address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                owned=[row for row in original['image_allocations'] if row['site']==spec['site'] and row['address']<=address<=row['address']+row['size']]
                assert len(owned)==1,'symbol output allocation lifetime differs'
                owner=owned[0]
                offset=address-owner['address'];capacity=owner['size'];destination=image_aliases[owner['address']]
                assert 0<=offset<=capacity and other==destination+offset,'symbol output cursor allocation/offset differs'
                left=original['memory'][owner['address']];right=bytes.fromhex(native['memory'][str(destination)])
                assert len(left)==len(right)==capacity,'incomplete symbol output capacity observation'
                assert left[:offset]==right[:offset],'initialized symbol output prefix differs'
                used_uninitialized.add(owner['address']);pair(owner['address'],destination)
            elif item.get('buffer_cursor'):
                spec=item['buffer_cursor'];base=global_integer(spec['root']);capacity=global_integer(spec['capacity'])
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                destination=int.from_bytes(bytes.fromhex(native['globals'][spec['root']]),'little')
                assert 0<=address-base<=capacity and other==destination+address-base,'symbol input cursor buffer/offset differs'
            elif item.get('interior_allocation'):
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little');spec=item['interior_allocation']
                assert bool(address)==bool(other),'arena cursor NULL state differs'
                if address:
                    owners=[base for base,size in image_sizes.items() if size==spec['size'] and base<=address<=base+size]
                    assert len(owners)==1,'arena cursor ownership differs'
                    owner=owners[0];row=next(row for row in data if row['name']==spec['remaining'])
                    remaining=int.from_bytes(original['memory'][row['original']],'little')
                    assert address-owner+remaining*spec.get('remaining_element_size',1)==spec['size'] and other==image_aliases[owner]+address-owner,'arena cursor/capacity/offset differs'
                    used_uninitialized.add(owner)
            elif item.get('module_buffer'):
                module_buffer(int.from_bytes(raw,'little'),int.from_bytes(actual,'little'))
            elif 'buffer_alias' in item:
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                alias=item['buffer_alias'];source=global_integer(alias)
                target=int.from_bytes(bytes.fromhex(native['globals'][alias]),'little')
                assert address==source and other==target, 'retained subsection buffer alias differs'
                pair(address,other)
            elif 'allocation' in item:
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                allocation(address,other,item['allocation'])
                if address and item.get('read_contents') and address not in retired:
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==image_sizes[address], 'incomplete read-buffer capacity observation'
                    for read in original['file_reads']:
                        if address<=read['buffer']<address+image_sizes[address]:
                            offset=read['buffer']-address;end=offset+read['returned']
                            assert end<=image_sizes[address] and left[offset:end]==right[offset:end], 'initialized subsection buffer bytes differ'
            elif 'blob' in item:
                address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if address in allocation_aliases:assert allocation_aliases[address]==other,'subsection output/global allocation alias differs'
                if pair(address,other):
                    spec=item['blob'];left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    size=spec.get('size')
                    if size is None:
                        count_item=next(row for row in data if row['name']==spec['count'])
                        size=int.from_bytes(original['memory'][count_item['original']],'little')*spec['element_size']
                    assert len(left)==len(right)==size,'allocated initialized byte extent differs'
                    excluded=set()
                    for pointer in spec.get('pointers',[]):
                        for offset in range(pointer['offset'],size,pointer.get('stride',size+1)):
                            excluded.update(range(offset,offset+4))
                            source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                            if 'allocation' in pointer:allocation(source,destination,pointer['allocation'])
                            elif 'shape' in pointer:linked(source,destination,pointer['shape'])
                            elif pair(source,destination):
                                if source in image_aliases:
                                    assert image_aliases[source]==destination and image_sizes[source]==pointer['size'],'initialized checked allocation ownership/capacity differs'
                                child=original['memory'][source];actual_child=bytes.fromhex(native['memory'][str(destination)])
                                assert len(child)==len(actual_child)==pointer['size'] and child==actual_child,'nested initialized allocation bytes differ'
                    for relocation in spec.get('relocations',[]):
                        for offset in range(relocation['offset'],size,relocation['stride']):
                            source=int.from_bytes(left[offset:offset+4],'little');destination=int.from_bytes(right[offset:offset+4],'little')
                            if source in module_aliases or destination in module_aliases.values():
                                excluded.update(range(offset,offset+4));module_buffer(source,destination)
                            elif relocation.get('copied_source_lines') and (source in allocation_aliases or destination in allocation_aliases.values()):
                                assert allocation_aliases.get(source)==destination,'directory source lines allocation alias differs'
                                owners=[row for row in original['subsection_allocations'] if row['address']==source]
                                assert len(owners)==1 and owners[0]['entry']==address+offset-4,'directory source lines entry/allocation identity differs'
                                assert int.from_bytes(left[offset-4:offset-2],'little')==0x127,'directory source lines subsection differs'
                                modules=original['memory'][global_integer('module_index')]
                                identifier=int.from_bytes(left[offset-2:offset],'little')
                                candidates=[int.from_bytes(modules[i+8:i+12],'little') for i in range(0,len(modules),12) if int.from_bytes(modules[i:i+2],'little')==identifier]
                                assert len(candidates)==1,'directory source lines module identity differs'
                                module=original['memory'][candidates[0]]
                                assert int.from_bytes(module[28:32],'little')==source and module[32:36]==left[offset+4:offset+8],'directory/module source lines identity differs'
                                assert owners[0]['size']==len(original['memory'][source])==len(bytes.fromhex(native['memory'][str(destination)])),'directory source lines extent differs'
                                assert original['memory'][source]==bytes.fromhex(native['memory'][str(destination)]),'directory source lines contents differ'
                                excluded.update(range(offset,offset+4));pair(source,destination)
                            elif relocation.get('rewritten_symbols_site') and (source in image_aliases or destination in image_aliases.values()):
                                owners=[row for row in original['image_allocations'] if row['site']==relocation['rewritten_symbols_site'] and row['address']==source]
                                assert len(owners)==1 and owners[0]['address']==source,'directory rewritten symbol allocation lifetime differs'
                                assert image_aliases[source]==destination,'directory rewritten symbol allocation alias differs'
                                assert offset==rewrite_owners[source]['entry_index']*12+4,'directory rewritten symbol entry owner differs'
                                assert int.from_bytes(left[offset-4:offset-2],'little')==0x124,'directory rewritten symbol subsection differs'
                                length=int.from_bytes(left[offset+4:offset+8],'little')
                                module=original['memory'][rewrite_owners[source]['info']]
                                assert module[20:24]==source.to_bytes(4,'little') and int.from_bytes(module[24:28],'little')==length,'directory/module rewritten symbol identity differs'
                                assert 0<=length<=owners[0]['size'],'directory rewritten symbol initialized extent differs'
                                if rewrite_owners[source]['info']==global_integer('current_module_info'):
                                    assert source+length==global_integer('symbol_output'),'current directory rewritten symbol cursor differs'
                                assert original['memory'][source][:length]==bytes.fromhex(native['memory'][str(destination)])[:length],'directory rewritten symbol contents differ'
                                excluded.update(range(offset,offset+4));pair(source,destination);used_uninitialized.add(source)
                    assert all(a==b for i,(a,b) in enumerate(zip(left,right)) if i not in excluded),('allocated initialized bytes differ',item['name'],[(i,a,b) for i,(a,b) in enumerate(zip(left,right)) if i not in excluded and a!=b][:16])
            elif 'shape' not in item:
                assert not any(raw) and not any(actual), ('non-NULL pointer needs an evidenced role',item['name'])
            else:
                shape=item['shape'];address=int.from_bytes(raw,'little');other=int.from_bytes(actual,'little')
                if 'count' not in shape:linked(address,other,shape)
                elif pair(address,other):
                    count_item=next(row for row in data if row['name']==shape['count'])
                    count=int.from_bytes(original['memory'][count_item['original']],'little');assert count<=128
                    left=original['memory'][address];right=bytes.fromhex(native['memory'][str(other)])
                    assert len(left)==len(right)==count*4
                    for index in range(count):
                        linked(int.from_bytes(left[index*4:index*4+4],'little'),int.from_bytes(right[index*4:index*4+4],'little'),shape)
        elif item.get('global_type_blocks'):
            capacity=global_integer('type_table_used');used=global_integer('next_global_type')-0x1000
            assert capacity in (0,512) and 0<=used<=capacity,'global type block contents need an observed initialized-prefix contract'
            for offset in range(0,item['size'],4):
                source=int.from_bytes(raw[offset:offset+4],'little');destination=int.from_bytes(actual[offset:offset+4],'little')
                if offset==0 and capacity:
                    assert source in image_aliases and image_aliases[source]==destination and image_sizes[source]==2048,'global type block allocation/alias/capacity differs'
                    used_uninitialized.add(source);pair(source,destination)
                    if isinstance(item['global_type_blocks'],dict):
                        left=original['memory'][source];right=bytes.fromhex(native['memory'][str(destination)])
                        assert len(left)==len(right)==2048,'incomplete global type pointer-block capacity'
                        for index in range(used):
                            arena_payload(int.from_bytes(left[index*4:index*4+4],'little'),int.from_bytes(right[index*4:index*4+4],'little'))
                    else:assert used==0,'historical global type block has no initialized-prefix observation'
                else:assert not source and not destination,'unused global type block slot is not zero'
        elif item.get('type_arena'):pass # Proven above, including pointer roles.
        elif item.get('function_table'):
            spec=item['function_table'] if isinstance(item['function_table'],dict) else {}
            stride=spec.get('stride',8);pointer=spec.get('pointer_offset',4)
            assert stride>=4 and 0<=pointer<=stride-4 and item['size']%stride==0
            assert len(raw)==len(actual)==item['size'],'symbol callback table extent differs'
            for offset in range(0,item['size'],stride):
                a=raw[offset:offset+stride];b=actual[offset:offset+stride]
                assert a[:pointer]==b[:pointer] and a[pointer+4:]==b[pointer+4:],'symbol callback kind/reserved differs'
                address=int.from_bytes(a[pointer:pointer+4],'little');other=int.from_bytes(b[pointer:pointer+4],'little')
                if address:
                    function=next(row for row in config['functions'] if row['original']==address)
                    assert other==native['function_addresses'][function['native_symbol']],'symbol callback logical identity differs'
                else:assert other==0,'symbol callback NULL identity differs'
        else:
            assert raw == actual, ('application global differs',item['name'],raw.hex(),actual.hex())
        globals_checked.append(item['name'])
    native_parameters=dict(native['parameters'])
    for item,value in object_parameters:
        actual=native_parameters[item['name']]
        assert actual['null']==(value==0)
        linked(value,actual['address'],item['shape'])
        if value:
            identities=list(aliases)
            parameters[item['name']]=dict(null=False,object_id=identities.index(value)+1)
            native_parameters[item['name']]=dict(null=False,object_id=identities.index(reverse[actual['address']])+1)
        else:
            parameters[item['name']]=native_parameters[item['name']]=dict(null=True)
    assert parameters == native_parameters, ('typed ABI arguments differ',parameters,native_parameters)
    if original_calls:assert original['last_error']==native['last_error'],'frontier LastError differs'
    type_compare_calls_checked=0
    if config.get('observe_recursive_type_compare'):
        before=original['type_compare_calls'];after=native['type_compare_calls']
        assert before and len(before)==len(after),'recursive comparison call inventory differs'
        root=global_integer('type_records');count=global_integer('type_record_count')
        for left,right in zip(before,after):
            offset=left['record']-root
            assert 0<=offset<count*32 and offset%32==0,'recursive comparison record ownership differs'
            expected={key:value for key,value in left.items() if key!='record'}
            expected['record_index']=offset//32
            assert 'different' in expected and expected==right,'recursive comparison operands/zero-result differ'
            assert expected['different']==(expected['left']!=expected['right']),'recursive comparison result contradicts complete inputs'
            type_compare_calls_checked+=1
    lookup_calls_checked=0
    if config.get('observe_global_type_lookup'):
        assert original['lookup_active_depth']==native['lookup_type_active_depth']==0,'unfinished global-type lookup observation'
        before=original['lookup_calls'];after=native['lookup_type_calls']
        assert before and len(before)==len(after),'global-type lookup call inventory differs'
        root=global_integer('type_records');count=global_integer('type_record_count')
        other=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little')
        sentinel=global_integer('empty_type_record')
        actual_sentinel=int.from_bytes(bytes.fromhex(native['globals']['empty_type_record']),'little')
        for left,right in zip(before,after):
            assert all(left[key]==right[key] for key in ('depth','index','parent_argument_alias','output')),'global-type lookup ABI/output sequence differs'
            result=left['result'];actual_result=right['result']
            if result==sentinel:assert actual_result==actual_sentinel,'global-type lookup sentinel return differs'
            else:
                offset=result-root
                assert 0<=offset<count*32 and offset%32==0 and actual_result==other+offset,'global-type lookup returned record identity differs'
            lookup_calls_checked+=1
    return dict(parameters=parameters,caller_state=caller_state,main_free_lifetimes=free_lifetimes,globals=globals_checked,lookup_type_calls_checked=lookup_calls_checked,type_compare_calls_checked=type_compare_calls_checked,aliased_objects_checked=len(aliases),file_handles_checked=len(handle_aliases),mapped_views_checked=len(view_roles),reads_checked=len(reads),uninitialized_allocations_checked=len(used_uninitialized),allocator_requests_checked=len(image_aliases),initialized_module_buffers_checked=len(module_aliases),typed_record_entries_checked=typed_records_checked,extended_type_references_checked=extended_references_checked,recursive_type_nodes_checked=recursive_type_nodes_checked,interned_type_nodes_checked=interned_type_nodes_checked,reference_symbol_nodes_checked=reference_symbol_nodes_checked,pooled_interned_nodes_checked=pooled_interned_nodes_checked,cleared_allocator_requests_checked=len(original['cleared_allocations']),cached_type_names_checked=cached_type_names_checked)
