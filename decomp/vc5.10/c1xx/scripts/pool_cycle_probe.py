"""Native observations adapted from C1's return/API transport; own C1XX contract."""
from scaler_probe import symbols
from pathlib import Path


def definitions(binary,output):
    (output/"pool-cycle-probe-source.py").write_bytes(Path(__file__).read_bytes())
    names=symbols(binary)
    code='''pool_cycle_events=[]
pool_cycle_returns=[]
pool_commit_api_entries=[]
pool_commit_api_returns=[]
pool_cli_entries=[]
class PoolCycleReturn(gdb.Breakpoint):
    def __init__(self,kind,index,address,entry_esp):
        super(PoolCycleReturn,self).__init__('*0x%x'%address,internal=True);self.kind=kind;self.index=index;self.entry_esp=entry_esp
        pool_cycle_events.append({'kind':'enter','function':kind,'index':index})
    def stop(self):
        row=scaler_context();row.update(kind=self.kind,index=self.index,entry_esp=self.entry_esp,guard=scaler_guard())
        pool_cycle_returns.append(row);pool_cycle_events.append({'kind':'return','function':self.kind,'index':self.index})
        self.enabled=False;return False
class PoolCommitApiReturn(gdb.Breakpoint):
    def __init__(self,address,args):
        super(PoolCommitApiReturn,self).__init__('*0x%x'%address,internal=True);self.args=args
    def stop(self):
        row=scaler_context();row.update(guard=scaler_guard(),maps=open('/proc/%d/maps'%gdb.selected_inferior().pid).read())
        views=[];view=gdb.parse_and_eval('allocations')
        while int(view):
            if len(views)>=32:raise RuntimeError('unbounded owned reservation list')
            item=view.dereference();views.append({'address':int(item['address']),'bytes':int(item['length'])});view=item['next']
        row['owned_reservations']=views;pool_commit_api_returns.append(row)
        if self.args[1:]!=[65536,4096,4] or row['eax']!=self.args[0]:raise RuntimeError('unknown actual commit response')
        open(@@COMMITTED@@,'wb').write(gdb.selected_inferior().read_memory(self.args[0],65536).tobytes())
        pool_cycle_events.append({'kind':'return','function':'commit-api','index':0});self.enabled=False;return False
class PoolCommitApiEntry(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older()
        if caller.name()!='CompilerArena::CommitExtent':return False
        row=scaler_context();words=list(struct.unpack('<5I',gdb.selected_inferior().read_memory(row['esp'],20)))
        if pool_commit_api_entries:raise RuntimeError('extra commit API call')
        row.update(return_pc=words[0],args=words[1:],caller=caller.name(),caller_this=int(caller.read_var('this')),guard=scaler_guard())
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        row['formal_inventory']=[{'name':item.name,'type':str(item.type)}for item in block if item.is_argument]
        pool_commit_api_entries.append(row);pool_cycle_events.append({'kind':'enter','function':'commit-api','index':0});PoolCommitApiReturn(words[0],words[1:]);return False
class PoolCycleCLI(gdb.Breakpoint):
    def stop(self):
        row=scaler_context();words=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12)));row.update(return_pc=words[0],args=words[1:],guard=scaler_guard())
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        row['formal_inventory']=[{'name':item.name,'type':str(item.type)}for item in block if item.is_argument]
        pool_cli_entries.append(row);return False
PoolCommitApiEntry('*0x%x'%@@API@@,internal=True)
PoolCycleCLI('*0x%x'%@@CLI@@,internal=True)
'''
    return code.replace('@@COMMITTED@@',repr(str(output/'pool-committed.bin'))).replace('@@API@@',str(names['recon_VirtualAlloc'][0])).replace('@@CLI@@',str(names['process_compiler_options'][0]))


def finish(output):
    code=''
    for name,value in [('pool-cycle-events','pool_cycle_events'),('pool-cycle-returns','pool_cycle_returns'),('pool-commit-api-entries','pool_commit_api_entries'),('pool-commit-api-returns','pool_commit_api_returns'),('pool-cli-entries','pool_cli_entries')]:code+='open('+repr(str(output/(name+'.json')))+',"w").write(json.dumps('+value+',indent=2)+"\\n")\n'
    code+='open('+repr(str(output/'pool-final-committed.bin'))+',"wb").write(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval("(unsigned int)frontend_bss.arenas[0].base")),65536).tobytes())\n'
    return code
