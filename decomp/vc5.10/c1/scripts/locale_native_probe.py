"""Physical Linux-only public-API and scanner entry/return observations."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 code=r'''locale_native_records=[]
scanner_init_entries=[]
scanner_init_returns=[]
locale_old_pctype=0
locale_result_pointer=0
def native_locale_snapshot(phase):
    global locale_old_pctype
    def number(expression):return int(gdb.parse_and_eval(expression))
    pctype=number('(unsigned int)recon_pctype')
    if not locale_old_pctype:locale_old_pctype=pctype
    addresses={'ctype':pctype-2,'old-ctype':locale_old_pctype-2,'mbctype':number('(unsigned int)recon_mbctype'),'result':locale_result_pointer}
    raw={name:gdb.selected_inferior().read_memory(addresses[name],size).tobytes() for name,size in [('ctype',514),('old-ctype',514),('mbctype',257)]}
    raw['result']=gdb.Value(locale_result_pointer).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1').encode('latin1')+b'\0'if locale_result_pointer else b''
    for name,data in raw.items():open(LOCALE_OUTPUT+'/locale-native-'+phase+'-'+name+'.bin','wb').write(data)
    crt=observed_crt_vectors()
    for name in ('argv','environment'):
        data=open(LOCALE_OUTPUT+'/native-crt-'+name+'.bin','rb').read()
        open(LOCALE_OUTPUT+'/locale-native-'+phase+'-CRT-'+name+'.bin','wb').write(data)
    invocation=capture_invocation();values=invocation['argv'];pointers=invocation['argv_pointers']
    packed=struct.pack('<II',invocation['args']['argv'],len(values))
    for pointer,text in zip(pointers,values):
        data=text.encode('ascii')+b'\0';packed+=struct.pack('<II',pointer,len(data))+data
    open(LOCALE_OUTPUT+'/locale-native-'+phase+'-Invoke-argv.bin','wb').write(packed)
    return {'phase':phase,'addresses':addresses,'ctype_static':number('(unsigned int)character_flags'),
      'mb_cur_max':number('recon_mb_cur_max'),'lc_codepage':number('recon_lc_codepage'),
      'lc_handles':[number('recon_lc_handle[%d]'%i)for i in range(6)],
      'mb_gate':number('recon_mb_codepage_gate'),'mb_codepage':number('recon_mb_codepage'),
      'errors':[number('(int)errno'),number('recon_doserrno'),number('last_error')],
      'fp':capture_x87(),'compiler_state':capture_modeled(),'invocation':invocation,'crt_vectors':crt,
      'crt_state':capture_command_line_crt_state(),
      'ctype_selection':{'bound':number('ctype_selection.bound'),'calls':number('ctype_selection.calls'),'mb_calls':number('ctype_selection.mb_calls'),
        'reference':gdb.parse_and_eval('ctype_selection.reference_sha256').string(),'result':number('(unsigned int)ctype_selection.result')}}
class NativeLocaleReturn(gdb.Breakpoint):
    def __init__(self,name,address,esp):
        super(NativeLocaleReturn,self).__init__('*0x%x'%address);self.name=name;self.esp=esp
    def stop(self):
        global locale_result_pointer
        eax=int(gdb.parse_and_eval('(unsigned int)$eax'))
        if self.name=='setlocale':locale_result_pointer=eax
        phase='after'if self.name=='setlocale'else'mb-after'
        record=native_locale_snapshot(phase)
        record.update(kind='return',name=self.name,pc=int(gdb.parse_and_eval('(unsigned int)$pc')),
          esp=int(gdb.parse_and_eval('(unsigned int)$esp')),entry_esp=self.esp,eax=eax,
          preserved=pool_cycle_preserved())
        locale_native_records.append(record);self.enabled=False;return False
class NativeLocaleEntry(gdb.Breakpoint):
    def __init__(self,name,address,count):
        super(NativeLocaleEntry,self).__init__('*0x%x'%address);self.name=name;self.count=count
    def stop(self):
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<%dI'%(self.count+1),gdb.selected_inferior().read_memory(esp,(self.count+1)*4).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        require=bool(block and block.function)
        assert require
        formals=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument]
        phase='before'if self.name=='setlocale'else'mb-before'
        record=native_locale_snapshot(phase)
        frame=gdb.newest_frame();caller=frame.older()
        record.update(kind='entry',name=self.name,pc=pc,esp=esp,words=words,formal_inventory=formals,
          caller={'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}if self.name=='setlocale'else{}},
          preserved=pool_cycle_preserved())
        if self.name=='setlocale':record['locale_bytes']=list(gdb.selected_inferior().read_memory(words[2],1).tobytes())
        if self.name=='setlocale':
            identity=gdb.parse_and_eval('*c1_host_identity')
            state=dict(symbol=frame.name(),pc=pc,args={'category':words[1],'locale':words[2]},formal_inventory=formals,
              invocation=record['invocation'],caller=record['caller'],compiler_state=record['compiler_state'],
              command_line_fp=record['fp'],pass_mode=record['compiler_state']['pass_mode'],
              module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),
              module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')),
              public_entry={'esp':esp,'stack':words,'return':words[0],'locale_bytes':record['locale_bytes'],'preserved':record['preserved']})
            state.update(record['crt_state'])
            open(LOCALE_OUTPUT+'/locale-native-setlocale-entry-state.json','w').write(json.dumps(state,indent=2)+'\n')
        locale_native_records.append(record);NativeLocaleReturn(self.name,words[0],esp);return False
NativeLocaleEntry('setlocale',SETLOCALE_PC,2)
NativeLocaleEntry('_setmbcp',SETMBCP_PC,1)
class ScannerInitReturn(gdb.Breakpoint):
    def stop(self):
        scanner_init_returns.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
          'preserved':pool_cycle_preserved(),'fp':capture_x87(),'compiler_state':capture_modeled()})
        self.enabled=False;return False
class ScannerInitEntry(gdb.Breakpoint):
    def stop(self):
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        returned=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        frame=gdb.newest_frame();caller=frame.older()
        scanner_init_entries.append({'pc':pc,'esp':esp,'return':returned,'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}},
          'preserved':pool_cycle_preserved(),'fp':capture_x87(),'compiler_state':capture_modeled()})
        ScannerInitReturn('*0x%x'%returned);return False
ScannerInitEntry('*0x%x'%SCANNER_PC)
'''
 code=code.replace('SETLOCALE_PC',str(names['recon_setlocale'][0])).replace('SETMBCP_PC',str(names['recon_setmbcp'][0])).replace('SCANNER_PC',str(names['initialize_multibyte_scanner'][0]))
 return 'LOCALE_OUTPUT=%r\n'%str(output)+code

def final_observation(output,following_stage=False):
 prefix=''if following_stage else "locale_native_records.append(dict(native_locale_snapshot('next'),kind='next',name='target-stub'))\n"
 return prefix+'''
open(%r,'w').write(json.dumps(locale_native_records,indent=2)+'\\n')
open(%r,'w').write(json.dumps(scanner_init_entries,indent=2)+'\\n')
open(%r,'w').write(json.dumps(scanner_init_returns,indent=2)+'\\n')
open(%r,'w').write(json.dumps(checksum_entries,indent=2)+'\\n')
open(%r,'w').write(json.dumps(checksum_returns,indent=2)+'\\n')
'''%tuple(str(output/name)for name in ('locale-native-records.json','scanner-init-entries.json','scanner-init-returns.json','checksum-entries.json','checksum-returns.json'))
