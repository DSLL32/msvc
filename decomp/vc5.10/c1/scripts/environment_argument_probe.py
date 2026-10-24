"""Fresh physical tokenizer ABI/state observations, currently NULL input only."""
from allocation_state import symbols

def definitions(binary,output):
 names=symbols(binary)
 return r'''environment_entries=[]
environment_returns=[]
def environment_snapshot():
    return {'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),
      'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
      'preserved':pool_cycle_preserved(),'compiler_state':capture_modeled(),
      'fp':capture_x87(),'crt_state':capture_command_line_crt_state(),
      'invocation':capture_invocation(),'crt':observed_crt_vectors()}
class EnvironmentReturn(gdb.Breakpoint):
    def stop(self):
        environment_returns.append(environment_snapshot());self.enabled=False;return False
class EnvironmentEntry(gdb.Breakpoint):
    def stop(self):
        row=environment_snapshot();frame=gdb.newest_frame();caller=frame.older()
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(row['esp'],8).tobytes()))
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        identity=gdb.parse_and_eval('*c1_host_identity')
        row.update(symbol=frame.name(),args={'arguments':words[1]},words=words,
          formal_inventory=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          caller={'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv')}},
          module_identity=dict({key:identity[key].string()for key in ('name','path','sha256')},flags=int(identity['flags'])),
          module_token=int(gdb.parse_and_eval('(unsigned int)c1_host_module')))
        environment_entries.append(row);EnvironmentReturn('*0x%x'%words[0]);return False
EnvironmentEntry('*0x%x'%ENV_ENTRY_PC)
'''.replace('ENV_ENTRY_PC',str(names['read_env_arguments'][0]))

def final_observation(output):
 return '\n'.join("open(%r,'w').write(json.dumps(%s,indent=2)+'\\n')"%(str(output/file),value)for file,value in [('environment-entries.json','environment_entries'),('environment-returns.json','environment_returns')])+'\n'
