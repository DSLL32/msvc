"""Actual NULL environment parser entry/return and next allocator ABI."""
from pathlib import Path
from scaler_probe import symbols


def setup(output,binary):
    (output/'env-arguments-probe-source.py').write_bytes(Path(__file__).read_bytes())
    names=symbols(binary)
    code='''python
env_argument_entries=[]
env_argument_returns=[]
env_argument_next=[]
def env_argument_row(phase,count):
    row=fold_row();words=struct.unpack('<%dI'%(count+1),gdb.selected_inferior().read_memory(row['esp'],4*(count+1)))
    row.update(return_pc=words[0],args=list(words[1:]),formal_inventory=checksum_formals(row['pc']),locale=checksum_locale_state(phase))
    return row
class EnvArgumentsReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(EnvArgumentsReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=fold_row();row.update(entry_esp=self.entry['esp'],locale=checksum_locale_state('env-return'))
        env_argument_returns.append(row);self.enabled=False;return False
class EnvArgumentsEntry(gdb.Breakpoint):
    def stop(self):
        row=env_argument_row('env-entry',1);env_argument_entries.append(row);EnvArgumentsReturn(row);return False
class EnvArgumentsNext(gdb.Breakpoint):
    def stop(self):
        env_argument_next.append(env_argument_row('env-next',2));return False
EnvArgumentsEntry('*0x%x'%@@ENTRY@@,internal=True)
EnvArgumentsNext('*0x%x'%@@NEXT@@,internal=True)
end
'''.replace('@@ENTRY@@',str(names['read_env_arguments'][0])).replace('@@NEXT@@',str(names['new_option_vector'][0]))
    finish='python\n'
    for suffix,var in [('entries','env_argument_entries'),('returns','env_argument_returns'),('next','env_argument_next')]:
        finish+='open(%r,"w").write(json.dumps(%s,indent=2)+"\\n")\n'%(str(output/('env-arguments-'+suffix+'.json')),var)
    return code,finish+'end\n'
