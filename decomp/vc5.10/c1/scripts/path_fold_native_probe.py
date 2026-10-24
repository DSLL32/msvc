"""Physical native initializer and public _mbctolower entry/return records."""
from allocation_state import symbols

def definitions(binary,output,restore):
 names=symbols(binary)
 text=r'''path_fold_entries=[]
path_fold_returns=[]
path_fold_calls=[]
def path_fold_api_snapshot():
    number=lambda expression:int(gdb.parse_and_eval(expression))
    inferior=gdb.selected_inferior()
    address=lambda expression:number('(unsigned int)'+expression)
    read=lambda pointer,count:list(inferior.read_memory(pointer,count).tobytes())
    ctype=address('recon_pctype')-2;old=address('character_flags')
    result=gdb.Value(locale_result_pointer).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1').encode('latin1')+b'\0'
    return {'errors':[number('(int)errno'),number('recon_doserrno'),number('last_error')],
      'fp':capture_x87(),'mb_cur_max':number('recon_mb_cur_max'),'lc_codepage':number('recon_lc_codepage'),
      'lc_handles':[number('recon_lc_handle[%d]'%i)for i in range(6)],
      'mb_gate':number('recon_mb_codepage_gate'),'mb_codepage':number('recon_mb_codepage'),
      'addresses':{'ctype':ctype,'old_ctype':old,'result':locale_result_pointer,'mb':address('recon_mbctype')},
      'mb':read(address('recon_mbctype'),257),'ctype':read(ctype,514),'old_ctype':read(old,514),
      'result':list(result),'fold':capture_path_fold_state(),
      'classes':read(address('&c1_data.scanner.classes'),256),
      'selection':{'bound':number('ctype_selection.bound'),'calls':number('ctype_selection.calls'),
        'mb_calls':number('ctype_selection.mb_calls'),'reference':gdb.parse_and_eval('ctype_selection.reference_sha256').string(),
        'result':address('ctype_selection.result')}}
class PathFoldApiReturn(gdb.Breakpoint):
    def __init__(self,index,address,esp):
        super(PathFoldApiReturn,self).__init__('*0x%x'%address);self.index=index;self.entry_esp=esp
    def stop(self):
        row=path_fold_api_snapshot();row.update(kind='return',pc=int(gdb.parse_and_eval('(unsigned int)$pc')),
          esp=int(gdb.parse_and_eval('(unsigned int)$esp')),entry_esp=self.entry_esp,
          eax=int(gdb.parse_and_eval('(unsigned int)$eax')),preserved=pool_cycle_preserved())
        path_fold_calls[self.index]['return']=row;self.enabled=False;return False
class PathFoldApiEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older();pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        row=path_fold_api_snapshot();row.update(kind='entry',pc=pc,esp=esp,words=words,
          formal_inventory=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          caller={'symbol':caller.name(),'pc':int(caller.pc())},preserved=pool_cycle_preserved())
        path_fold_calls.append({'entry':row});PathFoldApiReturn(len(path_fold_calls)-1,words[0],esp);return False
PathFoldApiEntry('*0x%x'%PATH_API_PC)
class PathFoldReturn(gdb.Breakpoint):
    def stop(self):
        path_fold_returns.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'preserved':pool_cycle_preserved(),
          'compiler_state':capture_modeled(),'fp':capture_x87(),'crt_state':capture_command_line_crt_state()})
        self.enabled=False;return False
class PathFoldEntry(gdb.Breakpoint):
    def stop(self):
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        returned=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        frame=gdb.newest_frame();caller=frame.older();block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        record={'pc':pc,'esp':esp,'return':returned,'symbol':frame.name(),
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}},
          'preserved':pool_cycle_preserved(),'fp':capture_x87(),'crt_state':capture_command_line_crt_state(),
          'pre_seed':capture_modeled(),'invocation':capture_invocation()}
        locale_native_records.append(dict(native_locale_snapshot('next'),kind='next',name='path-fold-entry-before-auxiliary'))
        if PATH_RESTORE:
            from path_fold_fields import seed
            address=int(gdb.parse_and_eval('(unsigned int)&c1_bss.path_fold'))
            gdb.selected_inferior().write_memory(address,struct.pack('<256H',*seed('helper')))
        record['compiler_state']=capture_modeled();path_fold_entries.append(record)
        PathFoldReturn('*0x%x'%returned);return False
PathFoldEntry('*0x%x'%PATH_INIT_PC)
'''
 return text.replace('PATH_API_PC',str(names['recon_mbctolower'][0])).replace('PATH_INIT_PC',str(names['initialize_path_fold_table'][0])).replace('PATH_RESTORE',str(bool(restore)))

def final_observation(output):
 return '\n'.join("open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')"%(str(output/file),value)for file,value in [('path-fold-entries.json','path_fold_entries'),('path-fold-returns.json','path_fold_returns'),('path-fold-calls.json','path_fold_calls')])+'\n'
