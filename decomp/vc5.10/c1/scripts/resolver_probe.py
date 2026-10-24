"""Actual known-input resolver entry; its pending helper stays a physical INT3."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 return r'''resolver_entries=[]
class ResolverEntry(gdb.Breakpoint):
    def stop(self):
        assert not resolver_entries
        row=vector_abi(vector_snapshot(),4)
        row['args']=dict(zip(('output','input','size'),row['words'][1:]))
        row['input_bytes']=vector_c_string(row['args']['input'])
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        caller=gdb.newest_frame().older()
        row['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}
        row['locale']=native_locale_snapshot('resolver-entry')
        resolver_entries.append(row);return False
ResolverEntry('*0x%x')
'''%names['resolve_option_path'][0]

def final_observation(output):
 return r'''assert value['symbol']=='normalize_path'
caller=gdb.newest_frame().older()
assert caller.name()=='resolve_option_path'
value['caller']['args']={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('output','input','size')}
block=caller.block()
while block and not block.function:block=block.superblock
value['caller']['formal_inventory']=[dict(name=symbol.name,type=str(symbol.type))for symbol in block if symbol.is_argument]
local=caller.read_var('path')
value['resolver_local_buffer']=dict(address=int(local.address.cast(gdb.lookup_type('unsigned int'))),bytes=local.type.sizeof)
value['resolver_path_bytes']=vector_c_string(value['args']['path'])
value['resolver_locale']=native_locale_snapshot('resolver-normalize')
open(%r,'w').write(json.dumps(resolver_entries,indent=2)+'\n')
'''%str(output/'resolver-entries.json')
