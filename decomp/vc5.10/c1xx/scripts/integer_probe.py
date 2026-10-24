"""Actual native numeric and Linux public CRT calls; no return-mask remapping."""
from pathlib import Path


def setup(output, binary, collector):
    (output / 'integer-probe-source.py').write_bytes(Path(__file__).read_bytes())
    checkpoint = output / 'collect-integer-entry.gdb'
    checkpoint.write_text(collector.read_text().replace(
        repr(str(output / 'state.json')), repr(str(output / 'integer-entry-state.json'))
    ).replace(repr(str(output / 'native-argv.bin')),
              repr(str(output / 'integer-entry-argv.bin'))))
    code = r'''python
integer_entries=[]
integer_returns=[]
integer_api_entries=[]
integer_api_returns=[]
integer_suffix_entries=[]
integer_native_providers={}
class IntegerReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(IntegerReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('integer-0-return');row.update(entry_esp=self.entry['esp'],value=action_string(self.entry['args'][0]))
        integer_returns.append(row);self.enabled=False;return False
class IntegerPublicReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(IntegerPublicReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('integer-0-api-'+self.entry['kind']+'-return')
        row.update(entry_esp=self.entry['esp'],kind=self.entry['kind'],argument_after=struct.unpack('<I',gdb.selected_inferior().read_memory(row['esp'],4))[0])
        if self.entry['kind']=='atol':row['input']=action_string(self.entry['args'][0])
        integer_api_returns.append(row);self.enabled=False;return False
class IntegerPublic(gdb.Breakpoint):
    def __init__(self,kind,address):
        super(IntegerPublic,self).__init__('*0x%x'%address,internal=True);self.kind=kind
    def stop(self):
        if len(integer_entries)!=1 or integer_returns:raise RuntimeError('public numeric observer escaped active target invocation')
        row=vector_row('integer-0-api-'+self.kind+'-entry')
        words=struct.unpack('<2I',gdb.selected_inferior().read_memory(row['esp'],8))
        row.update(return_pc=words[0],args=[words[1]],formal_inventory=None,caller=gdb.newest_frame().older().name(),eflags=int(gdb.parse_and_eval('(unsigned int)$eflags')))
        provider=gdb.solib_name(row['pc'])
        if not provider or not provider.endswith('/libc.so.6'):raise RuntimeError('numeric public observer is not at actual Linux libc entry')
        row.update(provider=provider,provider_entry_bytes=gdb.selected_inferior().read_memory(row['pc'],16).tobytes().hex())
        row.update(kind=self.kind,formal_observation='one actual raw cdecl argument; external libc has no target DWARF formal claim')
        if self.kind=='atol':row['input']=action_string(row['args'][0])
        integer_api_entries.append(row);IntegerPublicReturn(row);return False
class IntegerEntry(gdb.Breakpoint):
    def stop(self):
        if integer_entries:raise RuntimeError('unexpected second numeric invocation before suffix frontier')
        row=vector_abi(vector_row('integer-0-entry'),1);row['value']=action_string(row['args'][0])
        integer_entries.append(row);gdb.execute('source '+@@CHECKPOINT@@)
        for kind in ('isspace','isdigit','atol'):
            address=int(gdb.parse_and_eval('(unsigned int)(void*)'+kind));integer_native_providers[kind]=address;IntegerPublic(kind,address)
        IntegerReturn(row);return False
class IntegerSuffix(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_row('integer-suffix-entry'),3)
        row.update(pattern=action_string(row['args'][0]),argument=action_string(row['args'][1]))
        integer_suffix_entries.append(row);return False
IntegerEntry('*parse_option_integer',internal=True)
IntegerSuffix('*match_option_suffix',internal=True)
end
'''.replace('@@CHECKPOINT@@', repr(str(checkpoint)))
    finish = 'python\n'
    for name in ('entries', 'returns', 'api_entries', 'api_returns', 'suffix_entries'):
        finish += 'open(%r,"w").write(json.dumps(integer_%s,indent=2)+"\\n")\n' % (
            str(output / ('integer-' + name.replace('_', '-') + '.json')), name)
    finish += 'open(%r,"w").write(json.dumps(integer_native_providers,indent=2)+"\\n")\n' % str(output / 'integer-native-providers.json')
    return code, finish + 'end\n'
