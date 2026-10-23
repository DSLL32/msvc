"""Native real early-option entries/returns; first full state anchors prior scan."""
def setup(output):
    script=output/'observe-early-switches.gdb'
    body='''python
import gdb,json,struct
option_output=@@OUTPUT@@
option_entries=[]
option_returns=[]
def option_context():return {name:int(gdb.parse_and_eval('(unsigned int)$'+name)) for name in ('pc','esp','eax','edi','esi','ebx')}
def option_values():return [int(gdb.parse_and_eval('frontend_data.'+name)) for name in ('option_zb','option_bm','option_zm_percent')]
class EarlyOptionReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(EarlyOptionReturn,self).__init__('*0x%x'%address,internal=True)
        self.index=index
    def stop(self):
        row=option_context();row.update(index=self.index,values=option_values(),last_error=int(gdb.parse_and_eval('last_error')),fpu=int(gdb.parse_and_eval('$fctrl')))
        option_returns.append(row);self.enabled=False
        return False
class EarlyOptionEntry(gdb.Breakpoint):
    def stop(self):
        index=len(option_entries)
        if index>=25 or gdb.newest_frame().older().name()!='initialize_intermediate_files':raise RuntimeError('unknown early option caller/count')
        row=option_context();words=list(struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(row['esp'],12))))
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        row.update(index=index,args=words[1:],return_pc=words[0],formal_inventory=[{'name':item.name,'type':str(item.type)} for item in block if item.is_argument],values=option_values(),last_error=int(gdb.parse_and_eval('last_error')),fpu=int(gdb.parse_and_eval('$fctrl')))
        option_entries.append(row)
        if index==0:gdb.execute('source '+option_output+'/collect-early-option-entry.gdb')
        EarlyOptionReturn(words[0],index)
        return False
option_entry_bp=EarlyOptionEntry('*process_early_option',internal=True)
end
'''.replace('@@OUTPUT@@',repr(str(output)))
    script.write_text(body)
    finish='python\nopen('+repr(str(output/'early-switch-entries.json'))+',"w").write(json.dumps(option_entries,indent=2)+"\\n")\nopen('+repr(str(output/'early-switch-returns.json'))+',"w").write(json.dumps(option_returns,indent=2)+"\\n")\nend\n'
    return 'source '+str(script)+'\n',finish
