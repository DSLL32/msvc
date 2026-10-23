"""Native real getenv namespace and single GetSystemInfo profile consumption."""
def setup(output,collector):
    for stem in ('hook-entry','system-info-entry','system-info-return'):
        (output/('collect-'+stem+'.gdb')).write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(stem+'.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(stem+'-argv.bin')))))
    script=output/'observe-system-info.gdb'
    script.write_text('''python
import gdb,json,struct,hashlib
system_output=@@OUTPUT@@
system_entries=[]
system_returns=[]
environment_entries=[]
environment_returns=[]
def system_context():return {name:int(gdb.parse_and_eval('(unsigned int)$'+name)) for name in ('pc','esp','eax','edi','esi','ebx')}
def system_profile_state():
    p=gdb.parse_and_eval('native_system_profile')
    return dict(bound=int(p['bound']),system_calls=int(p['system_calls']),memory_calls=int(p['memory_calls']),reference=p['reference_sha256'].string())
def system_vector(expression,path):
    pointer=gdb.parse_and_eval(expression);values=[];index=0
    while int(pointer[index]):
        if index>=16384:raise RuntimeError('unbounded CRT vector')
        address=int(pointer[index]);raw=pointer[index].string(encoding='latin1').encode('latin1')+b'\\0'
        values.append((address,raw));index+=1
    blob=struct.pack('<II',int(pointer),len(values))
    for address,raw in values:blob+=struct.pack('<II',address,len(raw))+raw
    open(system_output+'/'+path,'wb').write(blob)
    return dict(address=int(pointer),count=len(values))
class EnvironmentReturn(gdb.Breakpoint):
    def stop(self):
        row=system_context();row.update(bound=int(gdb.parse_and_eval('recon_crt_arguments_bound')),environment=int(gdb.parse_and_eval('(unsigned int)recon_crt_environment')))
        environment_returns.append(row);self.enabled=False
        return False
class EnvironmentEntry(gdb.Breakpoint):
    def stop(self):
        if environment_entries or gdb.newest_frame().older().name()!='initialize_intermediate_files':raise RuntimeError('unexpected target getenv call')
        row=system_context();words=struct.unpack('<2I',bytes(gdb.selected_inferior().read_memory(row['esp'],8)))
        name=gdb.Value(words[1]).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1')
        row.update(return_pc=words[0],name=name,bound=int(gdb.parse_and_eval('recon_crt_arguments_bound')),argc=int(gdb.parse_and_eval('recon_crt_argc')),argv=system_vector('recon_crt_argv','native-crt-argv.bin'),environment=system_vector('recon_crt_environment','native-crt-environment.bin'))
        host_values=[item for item in open('/proc/%d/environ'%gdb.selected_inferior().pid,'rb').read().split(b'\\0') if item.startswith(b'MSC_CMD_FLAGS=')]
        row['host_MSC_CMD_FLAGS']=[dict(bytes=len(item),sha256=hashlib.sha256(item).hexdigest()) for item in host_values]
        environment_entries.append(row);EnvironmentReturn('*0x%x'%words[0],internal=True)
        return False
class SystemReturn(gdb.Breakpoint):
    def stop(self):
        row=system_context();row.update(profile=system_profile_state(),last_error=int(gdb.parse_and_eval('last_error')),fpu=int(gdb.parse_and_eval('$fctrl')))
        system_returns.append(row);gdb.execute('source '+system_output+'/collect-system-info-return.gdb');self.enabled=False
        return False
class SystemEntry(gdb.Breakpoint):
    def stop(self):
        if system_entries or gdb.newest_frame().older().name()!='c1xx_main':raise RuntimeError('unexpected GetSystemInfo call')
        row=system_context();words=struct.unpack('<2I',bytes(gdb.selected_inferior().read_memory(row['esp'],8)))
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        row.update(return_pc=words[0],argument=words[1],formal_inventory=[{'name':item.name,'type':str(item.type)} for item in block if item.is_argument],profile=system_profile_state(),last_error=int(gdb.parse_and_eval('last_error')),fpu=int(gdb.parse_and_eval('$fctrl')))
        system_entries.append(row);gdb.execute('source '+system_output+'/collect-system-info-entry.gdb');SystemReturn('*0x%x'%words[0],internal=True)
        return False
environment_bp=EnvironmentEntry('*recon_getenv',internal=True)
system_bp=SystemEntry('*recon_GetSystemInfo',internal=True)
end
'''.replace('@@OUTPUT@@',repr(str(output))))
    observer='break *initialize_frontend_hook\ncommands\nsilent\nsource '+str(output/'collect-hook-entry.gdb')+'\ncontinue\nend\nsource '+str(script)+'\n'
    finish='python\n'
    for variable,name in [('system_entries','system-info-entries.json'),('system_returns','system-info-returns.json'),('environment_entries','target-getenv-entries.json'),('environment_returns','target-getenv-returns.json')]:finish+='open('+repr(str(output/name))+',"w").write(json.dumps('+variable+',indent=2)+"\\n")\n'
    return observer,finish+'end\n'
