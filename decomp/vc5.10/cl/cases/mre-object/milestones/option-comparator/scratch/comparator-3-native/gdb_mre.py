"""Read-only native MRE ABI/CRT observations at actual calls and named trap."""
import json,os,stat,struct
from pathlib import Path
import gdb


def observe(directory,phase,arguments,call=None):
    directory=Path(directory);inferior=gdb.selected_inferior();frame=gdb.newest_frame();ranges=[]
    value=lambda expression:int(gdb.parse_and_eval(expression))&0xffffffff
    def read(address,length):
        raw=bytes(inferior.read_memory(address,length));ranges.append({'address':address,'bytes':raw.hex()});return raw
    def global_address(name):
        symbol=gdb.lookup_global_symbol(name)or gdb.lookup_static_symbol(name)
        assert symbol is not None,name
        return int(symbol.value().address)&0xffffffff
    def scalar(name):return struct.unpack('<I',bytes(inferior.read_memory(global_address(name),4)))[0]
    symbols={name:global_address(name)for name in(
        'recon_doserrno','last_error','recon_fmode','recon_commode','recon_mb_cur_max','recon_mb_codepage',
        'recon_lc_codepage','recon_lc_handle','recon_pctype','recon_crt_environment','recon_crt_initenv',
        'recon_crt_os_environment','recon_crt_os_environment_units','recon_crt_environment_updates',
        'driver','use_standard_include','minimal_rebuild','reproducible','first_dependency_scan','interrupted','pass_running','pdb_name','mre_call_count')}
    symbols['errno']=value('(unsigned)&errno')
    for name,address in symbols.items():read(address,24 if name=='recon_lc_handle' else 4)
    read(global_address('recon_mbctype'),257);read(scalar('recon_pctype')-2,514)
    read(global_address('recon_iob'),96);read(global_address('recon_osfile'),256)
    read(global_address('recon_osfhnd'),1024);read(global_address('standard_handles'),12)
    def text(pointer):
        if not pointer:return
        for offset in range(32768):
            if read(pointer+offset,1)==b'\0':return
        raise AssertionError('unbounded native MRE string')
    def vector(pointer):
        assert pointer
        for i in range(4096):
            address=struct.unpack('<I',read(pointer+4*i,4))[0]
            if not address:return
            text(address)
        raise AssertionError('unbounded native MRE vector')
    vector(scalar('recon_crt_environment'));vector(scalar('recon_crt_initenv'))
    read(scalar('recon_crt_os_environment'),2*scalar('recon_crt_os_environment_units'))
    driver=scalar('driver');words=struct.unpack('<11I',read(driver,44))
    read(global_address('option_buffer'),1024)
    text(words[9])
    options=words[0]
    for count in range(4096):
        if not options:break
        row=struct.unpack('<6I',read(options,24))
        for pointer in row[2:5]:text(pointer)
        options=row[0]
    else:raise AssertionError('native MRE option list cycle')
    text(scalar('pdb_name'))
    if call==3:
        read(arguments['stack'],8)
        text(arguments['values'][0])
        signature=frame.older()if phase=='lower-before'else frame
        assert signature.name()=='option_signature'
        ebp=int(signature.read_register('ebp'))&0xffffffff
        read(ebp,12);slot=int(signature.read_var('state').address)&0xffffffff
        read(slot,4);assert int(signature.read_var('state'))&0xffffffff==driver
        arguments={**arguments,'signature_ebp':ebp,'state_slot':slot,'state':driver}
    elif call==6:
        sp=value('$esp');raw=struct.unpack('<3I',read(sp,12))
        qsort=frame.older();assert qsort.name()=='recon_qsort'
        qsort_ebp=int(qsort.read_register('ebp'))&0xffffffff;read(qsort_ebp,8)
        comparator=int(qsort.read_var('compare'))&0xffffffff
        compare_slot=int(qsort.read_var('compare').address)&0xffffffff;read(compare_slot,4)
        signature=qsort.older();assert signature.name()=='option_signature'
        signature_ebp=int(signature.read_register('ebp'))&0xffffffff;read(signature_ebp,12)
        driver_state=int(signature.read_var('state'))&0xffffffff
        slot=int(signature.read_var('state').address)&0xffffffff;read(slot,4);assert driver_state==driver
        items=int(signature.read_var('items'))&0xffffffff;count=int(signature.read_var('count'))
        items_slot=int(signature.read_var('items').address)&0xffffffff;count_slot=int(signature.read_var('count').address)&0xffffffff
        read(items_slot,4);read(count_slot,4);assert 2<=count<=64
        pointers=struct.unpack('<%dI'%count,read(items,count*4))
        for pointer in pointers:text(pointer)
        left,right=raw[1:];read(left,4);read(right,4)
        caller=signature.older();assert caller.name()=='prepare_jobs'
        pending=int(caller.read_var('pending'))&0xffffffff;read(pending,20)
        arguments={'stack':sp,'return':raw[0],'state':driver_state,'slot':slot,'caller':'recon_qsort',
            'signature_ebp':signature_ebp,'qsort_ebp':qsort_ebp,'comparator':comparator,'compare_slot':compare_slot,
            'items':items,'count':count,'items_slot':items_slot,'count_slot':count_slot,
            'left':left,'right':right,'left_slot':sp+4,'right_slot':sp+8,'pending':pending}
    elif call==5:
        # First callback normal return: frame is now qsort, while its caller's
        # owned array/strings remain live. Read the saved actual callback slots.
        for name in('left_slot','right_slot','items_slot','count_slot','slot','compare_slot'):
            read(arguments[name],4)
        read(arguments['left'],4);read(arguments['right'],4)
        text(struct.unpack('<I',read(arguments['left'],4))[0])
        text(struct.unpack('<I',read(arguments['right'],4))[0])
        read(arguments['qsort_ebp'],8);read(arguments['signature_ebp'],12)
        pointers=struct.unpack('<%dI'%arguments['count'],read(arguments['items'],4*arguments['count']))
        for pointer in pointers:text(pointer)
        read(arguments['pending'],20)
    elif call is not None:
        size=(6,2,1,0,2)[call];read(arguments['stack'],4*(size+1))
        args=arguments['values']
        if call==0:
            text(args[1]);read(args[3],1024)
            if phase.endswith('after'):read(args[0],4);read(args[2],4)
        elif call==1 and phase.endswith('after'):read(args[1],4)
        elif call==4:read(args[1],20)
    else:
        # Actual saved caller words are included in every new observation.
        # Initial-prefix evidence predating this remains explicitly qualified.
        frame_ebp=value('$ebp');read(frame_ebp,8)
        if frame.name()=='compare_option_strings':
            qsort=frame.older();assert qsort.name()=='recon_qsort'
            qsort_ebp=int(qsort.read_register('ebp'))&0xffffffff;read(qsort_ebp,8)
            comparator=int(qsort.read_var('compare'))&0xffffffff
            compare_slot=int(qsort.read_var('compare').address)&0xffffffff;read(compare_slot,4)
            signature=qsort.older();assert signature.name()=='option_signature'
            signature_ebp=int(signature.read_register('ebp'))&0xffffffff;read(signature_ebp,12)
            state=int(signature.read_var('state'))&0xffffffff;slot=int(signature.read_var('state').address)&0xffffffff
            read(slot,4);assert state==driver
            items=int(signature.read_var('items'))&0xffffffff;count=int(signature.read_var('count'))
            items_slot=int(signature.read_var('items').address)&0xffffffff;count_slot=int(signature.read_var('count').address)&0xffffffff
            read(items_slot,4);read(count_slot,4);assert 2<=count<=64
            pointers=struct.unpack('<%dI'%count,read(items,4*count))
            for pointer in pointers:text(pointer)
            left=int(frame.read_var('left'))&0xffffffff;right=int(frame.read_var('right'))&0xffffffff
            left_slot=int(frame.read_var('left').address)&0xffffffff;right_slot=int(frame.read_var('right').address)&0xffffffff
            read(left_slot,4);read(right_slot,4);read(left,4);read(right,4)
            caller=signature.older();assert caller.name()=='prepare_jobs'
            arguments={'state':state,'slot':slot,'caller':'recon_qsort','signature_ebp':signature_ebp,'qsort_ebp':qsort_ebp,
                'comparator':comparator,'compare_slot':compare_slot,'items':items,'count':count,'items_slot':items_slot,'count_slot':count_slot,
                'left':left,'right':right,'left_slot':left_slot,'right_slot':right_slot}
        elif frame.name()=='jobs_from_deferred':
            inputs=int(frame.read_var('inputs'))&0xffffffff
            slot=int(frame.read_var('inputs').address)&0xffffffff;read(slot,4)
            caller=frame.older();assert caller.name()=='prepare_jobs'
            arguments={'inputs':inputs,'slot':slot,'caller':'prepare_jobs'}
        elif frame.name()=='option_matching_selector':
            option=int(frame.read_var('option'))&0xffffffff
            selectors=int(frame.read_var('selectors'))&0xffffffff
            option_slot=int(frame.read_var('option').address)&0xffffffff
            selectors_slot=int(frame.read_var('selectors').address)&0xffffffff
            read(option_slot,4);read(selectors_slot,4);read(option,24);text(selectors)
            signature=frame.older();assert signature.name()=='option_signature'
            signature_ebp=int(signature.read_register('ebp'))&0xffffffff
            read(signature_ebp,12)
            state=int(signature.read_var('state'))&0xffffffff
            slot=int(signature.read_var('state').address)&0xffffffff
            read(slot,4);assert state==driver
            caller=signature.older();assert caller.name()=='prepare_jobs'
            arguments={'state':state,'slot':slot,'caller':'option_signature','signature_ebp':signature_ebp,
                'option':option,'selectors':selectors,'option_slot':option_slot,'selectors_slot':selectors_slot}
        else:
            assert frame.name()=='option_signature'
            state=int(frame.read_var('state'))&0xffffffff
            slot=int(frame.read_var('state').address)&0xffffffff
            read(slot,4);assert state==driver
            caller=frame.older();assert caller.name()=='prepare_jobs'
            arguments={'state':state,'slot':slot,'caller':'prepare_jobs'}
        pending=int(caller.read_var('pending'))&0xffffffff
        read(pending,20);arguments['pending']=pending
    registers={name:frame.read_register(name).bytes.hex()for name in
        ['fctrl','fstat','ftag','fiseg','fioff','foseg','fooff','fop']+['st%d'%i for i in range(8)]}
    general={name:value('$'+name)for name in('eip','esp','eax','ebx','esi','edi','ebp')}
    descriptor=None
    if value('recon_osfile[3]')&1:
        handle=value('recon_osfhnd[3]');fd=handle-1
        info=os.stat('/proc/%d/fd/%d'%(inferior.pid,fd))
        details=dict(row.split(':',1)for row in Path('/proc/%d/fdinfo/%d'%(inferior.pid,fd)).read_text().splitlines())
        assert stat.S_ISREG(info.st_mode)
        descriptor={'handle':handle,'device':info.st_dev,'inode':info.st_ino,'size':info.st_size,
            'position':int(details['pos']),'flags':int(details['flags'],8)}
    record={'phase':phase,'pid':inferior.pid,'symbols':symbols,
        'data_addresses':{name:global_address(name)for name in('recon_mbctype','recon_iob','recon_osfile','recon_osfhnd','standard_handles','option_buffer')},
        'arguments':arguments,'general':general,'x87':registers,'ranges':ranges,'descriptor3':descriptor}
    path=directory/('native-'+phase+'-gdb.json');assert not path.exists()
    path.write_text(json.dumps(record,indent=2)+'\n')
    from gdb_streams import capture
    capture(directory,phase)


def install(directory):
    calls=[]
    names=('MREFOpenByName','MREQueryMreDrv','MREDrvOneTimeInit','MREDrvFFilesOutOfDate')
    class Return(gdb.Breakpoint):
        def __init__(self,address,index,arguments):
            super().__init__('*%#x'%address,temporary=True,internal=True);self.index=index;self.arguments=arguments
        def stop(self):observe(directory,'mre-%d-after'%(self.index+1),self.arguments,4 if self.index==3 else self.index);return False
    class Entry(gdb.Breakpoint):
        def __init__(self,index):super().__init__('*'+names[index],internal=True);self.index=index
        def stop(self):
            assert self.index==len(calls),'extra/unordered native MRE entry'
            frame=gdb.newest_frame();assert frame.name()==names[self.index]and frame.older().name()=='prepare_jobs'
            sp=int(gdb.parse_and_eval('$esp'))&0xffffffff
            words=struct.unpack('<%dI'%((6,2,1,2)[self.index]+1),bytes(gdb.selected_inferior().read_memory(sp,4*((6,2,1,2)[self.index]+1))))
            arguments={'stack':sp,'return':words[0],'values':list(words[1:])};calls.append(self.index)
            observe(directory,'mre-%d-before'%(self.index+1),arguments,4 if self.index==3 else self.index)
            Return(words[0],self.index,arguments);return False
    for i in range(3):Entry(i)
    class LowerReturn(gdb.Breakpoint):
        def __init__(self,address,arguments):
            super().__init__('*%#x'%address,temporary=True,internal=True);self.arguments=arguments
        def stop(self):observe(directory,'lower-after',self.arguments,3);return False
    class LowerEntry(gdb.Breakpoint):
        def __init__(self):super().__init__('*recon_mbslwr',internal=True);self.calls=0
        def stop(self):
            assert self.calls==0 and gdb.newest_frame().name()=='recon_mbslwr'
            assert gdb.newest_frame().older().name()=='option_signature'
            self.calls+=1;sp=int(gdb.parse_and_eval('$esp'))&0xffffffff
            words=struct.unpack('<2I',bytes(gdb.selected_inferior().read_memory(sp,8)))
            arguments={'stack':sp,'return':words[0],'values':[words[1]]}
            observe(directory,'lower-before',arguments,3);LowerReturn(words[0],arguments);return False
    config=json.loads((Path(directory)/'config.json').read_bytes())
    if '_option_matching_selector'in config['accepted']:LowerEntry()
    if '_compare_option_strings'in config['accepted'] and '_jobs_from_deferred'not in config['accepted']:
        Entry(3)
        class CompareReturn(gdb.Breakpoint):
            def __init__(self,address,arguments):
                super().__init__('*%#x'%address,temporary=False,internal=True);self.arguments=arguments
            def stop(self):
                observe(directory,'compare-after',self.arguments,5)
                self.enabled=False
                expected=json.loads((Path(directory)/'mre-response.json').read_bytes())['comparator_call']['result']
                # Stop before any later external call if the actual CRT result
                # differs; retain the native failure as a public ABI mismatch.
                return (int(gdb.parse_and_eval('$eax'))&0xffffffff)!=expected
        class CompareEntry(gdb.Breakpoint):
            def __init__(self):super().__init__('*compare_option_strings',internal=True);self.calls=0
            def stop(self):
                if self.calls:return False
                self.calls+=1
                observe(directory,'compare-before',{},6)
                record=json.loads((Path(directory)/'native-compare-before-gdb.json').read_bytes())
                ret=record['arguments']['return']
                CompareReturn(ret,record['arguments']);return False
        CompareEntry()
