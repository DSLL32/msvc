"""Two actual leaf calls, public fullpath and real resolver return."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 code=r'''normalizer_entries=[]
normalizer_returns=[]
fullpath_entries=[]
fullpath_returns=[]
resolver_returns=[]
def path_context(row,phase):
    row['locale']=native_locale_snapshot(phase)
    row['logical_cwd']=__import__('os').environ['RECON_CANONICAL_CWD']
    row['private_S_root']=__import__('os').environ['RECON_DRIVE_S']
    row['physical_cwd']=__import__('os').readlink('/proc/%d/cwd'%gdb.selected_inferior().pid)
    return row
class NormalizerReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(NormalizerReturn,self).__init__('*0x%x'%address);self.index=index
    def stop(self):
        row=path_context(vector_snapshot(),'normalizer-%u-return'%self.index);entry=normalizer_entries[self.index]
        row.update(index=self.index,path_bytes=vector_c_string(entry['args']['path']),argument_slot_after=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4).tobytes())[0])
        normalizer_returns.append(row);self.enabled=False;return False
class NormalizerEntry(gdb.Breakpoint):
    def stop(self):
        index=len(normalizer_entries);assert index<2
        row=path_context(vector_abi(vector_snapshot(),2),'normalizer-%u-entry'%index)
        row.update(index=index,args={'path':row['words'][1]},path_bytes=vector_c_string(row['words'][1]))
        caller=gdb.newest_frame().older();assert caller.name()=='resolve_option_path'
        row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('output','input','size')}
        block=caller.block()
        while block and not block.function:block=block.superblock
        row['caller']['formal_inventory']=[dict(name=s.name,type=str(s.type))for s in block if s.is_argument]
        local=caller.read_var('path');row['resolver_local_buffer']=dict(address=int(local.address.cast(gdb.lookup_type('unsigned int'))),bytes=local.type.sizeof)
        row['resolver_path_bytes']=row['path_bytes'];row['resolver_locale']=row['locale']
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        normalizer_entries.append(row);NormalizerReturn(row['words'][0],index);return False
class FullpathReturn(gdb.Breakpoint):
    def __init__(self,address):super(FullpathReturn,self).__init__('*0x%x'%address)
    def stop(self):
        row=path_context(vector_snapshot(),'fullpath-return');entry=fullpath_entries[0]
        row['argument_slots_after']=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12).tobytes()))
        row['output_bytes']=vector_c_string(entry['args']['out'])
        open(NORMALIZER_OUTPUT+'/fullpath-native-return-buffer.bin','wb').write(gdb.selected_inferior().read_memory(entry['args']['out'],520).tobytes())
        fullpath_returns.append(row);self.enabled=False;return False
class FullpathEntry(gdb.Breakpoint):
    def stop(self):
        assert not fullpath_entries
        row=path_context(vector_abi(vector_snapshot(),4),'fullpath-entry');row['args']=dict(zip(('out','name','capacity'),row['words'][1:]))
        row['input_bytes']=vector_c_string(row['args']['name']);assert row['args']['capacity']==520
        caller=gdb.newest_frame().older();assert caller.name()=='resolve_option_path'
        row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('output','input','size')}
        open(NORMALIZER_OUTPUT+'/fullpath-native-entry-buffer.bin','wb').write(gdb.selected_inferior().read_memory(row['args']['out'],520).tobytes())
        fullpath_entries.append(row);FullpathReturn(row['words'][0]);return False
class ResolverReturn(gdb.Breakpoint):
    def __init__(self,address):super(ResolverReturn,self).__init__('*0x%x'%address)
    def stop(self):
        row=path_context(vector_snapshot(),'resolver-return');row['output_bytes']=vector_c_string(resolver_entries[0]['args']['output'])
        resolver_returns.append(row);self.enabled=False;return False
class ObserveResolverReturn(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));ret=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        ResolverReturn(ret);return False
NormalizerEntry('*0xNORMALIZE')
FullpathEntry('*0xFULLPATH')
ObserveResolverReturn('*0xRESOLVER')
'''
 return code.replace('NORMALIZER_OUTPUT',repr(str(output))).replace('NORMALIZE','%x'%names['normalize_path'][0]).replace('FULLPATH','%x'%names['recon_fullpath'][0]).replace('RESOLVER','%x'%names['resolve_option_path'][0])

def final_observation(output):
 return '\n'.join("open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')"%(str(output/file),value)for file,value in [('normalizer-entries.json','normalizer_entries'),('normalizer-returns.json','normalizer_returns'),('fullpath-entries.json','fullpath_entries'),('fullpath-returns.json','fullpath_returns'),('resolver-returns.json','resolver_returns')])+'\n'
