"""Actual native numeric calls and libc public entries, with raw return masks."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 code=r'''integer_entries=[]
integer_returns=[]
integer_api_entries=[]
integer_api_returns=[]
integer_native_providers={}
class IntegerReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(IntegerReturn,self).__init__('*0x%x'%address);self.index=index
    def stop(self):
        row=vector_snapshot();row['index']=self.index
        integer_returns.append(row);self.enabled=False;return False
class IntegerApiReturn(gdb.Breakpoint):
    def __init__(self,address,kind,index):
        super(IntegerApiReturn,self).__init__('*0x%x'%address);self.kind,self.index=kind,index
    def stop(self):
        row=vector_snapshot();row.update(kind=self.kind,index=self.index,argument_slot_after=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4).tobytes())[0])
        row['locale']=native_locale_snapshot('integer-%u-api-%s-return'%(self.index,self.kind))
        integer_api_returns.append(row);self.enabled=False;return False
class IntegerApiEntry(gdb.Breakpoint):
    def __init__(self,address,kind):
        super(IntegerApiEntry,self).__init__('*0x%x'%address);self.kind=kind
    def stop(self):
        caller=gdb.newest_frame().older()
        if caller.name()!='parse_option_integer':return False
        row=vector_snapshot();esp=row['esp'];words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()));index=len(integer_entries)-1
        row.update(kind=self.kind,index=index,words=words,caller=dict(symbol=caller.name(),pc=int(caller.pc()),args={'value':int(caller.read_var('value').cast(gdb.lookup_type('unsigned int')))}),formal_inventory=None,formal_observation='public libc has no target DWARF formals; actual one-slot cdecl ABI observed directly')
        raw=gdb.selected_inferior().read_memory(words[0]-5,5).tobytes();assert raw[0]==0xe8
        plt=words[0]+struct.unpack('<i',raw[1:])[0];jump=gdb.selected_inferior().read_memory(plt,6).tobytes();assert jump[:2]==b'\xff\x25'
        got=struct.unpack('<I',jump[2:])[0];resolved=struct.unpack('<I',gdb.selected_inferior().read_memory(got,4).tobytes())[0];assert resolved==row['pc']
        row['physical_import']=dict(call_bytes=raw.hex(),plt=plt,plt_bytes=jump.hex(),got=got,resolved=resolved)
        if self.kind=='atol':row['input']=vector_c_string(words[1])
        row['locale']=native_locale_snapshot('integer-%u-api-%s-entry'%(index,self.kind))
        integer_api_entries.append(row);IntegerApiReturn(words[0],self.kind,index);return False
class IntegerEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_snapshot(),2);caller=gdb.newest_frame().older();row['args']={'value':row['words'][1]};row['input']=vector_c_string(row['words'][1]);row['index']=len(integer_entries)
        row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('rule','value','copy')}
        identity=gdb.parse_and_eval('*c1_host_identity');row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        if not integer_entries:
            maps=open('/proc/%d/maps'%gdb.selected_inferior().pid).read()
            for kind in ('isspace','isdigit','atol'):
                address=int(gdb.parse_and_eval('(unsigned int)&'+kind));regions=[]
                for line in maps.splitlines():
                    pieces=line.split();low,high=[int(v,16)for v in pieces[0].split('-')]
                    if low<=address<high:regions.append(pieces)
                assert len(regions)==1 and regions[0][-1].endswith('/libc.so.6')and 'x'in regions[0][1]
                path=regions[0][-1];integer_native_providers[kind]=dict(address=address,region=regions[0],provider_path=path,provider_sha256=__import__('hashlib').sha256(open(path,'rb').read()).hexdigest(),entry16=gdb.selected_inferior().read_memory(address,16).tobytes().hex())
                IntegerApiEntry(address,kind)
        integer_entries.append(row);IntegerReturn(row['words'][0],row['index']);return False
IntegerEntry('*0x%x'%INTEGER_ENTRY)
'''
 return code.replace('INTEGER_ENTRY',str(names['parse_option_integer'][0]))

def final_observation(output):
 return '\n'.join("open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')"%(str(output/file),value)for file,value in [('integer-entries.json','integer_entries'),('integer-returns.json','integer_returns'),('integer-api-entries.json','integer_api_entries'),('integer-api-returns.json','integer_api_returns'),('integer-native-providers.json','integer_native_providers')])+'\n'
