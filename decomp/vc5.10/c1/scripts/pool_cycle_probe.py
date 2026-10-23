"""Native GDB observations for actual pool member returns and committed memory."""
from allocation_state import symbols

def definitions(binary,output):
    names=symbols(binary)
    code='''pool_cycle_events=[]
pool_cycle_returns=[]
pool_commit_api_entries=[]
pool_commit_api_returns=[]
def pool_cycle_preserved():
    return {name:int(gdb.parse_and_eval('(unsigned int)$'+name))for name in ('ebx','esi','edi','ebp')}
class PoolCycleReturn(gdb.Breakpoint):
    def __init__(self,kind,index,address,entry_esp):
        super(PoolCycleReturn,self).__init__('*0x%x'%address);self.kind=kind;self.index=index;self.entry_esp=entry_esp
        pool_cycle_events.append({'kind':'enter','function':kind,'index':index})
    def stop(self):
        pool_cycle_events.append({'kind':'return','function':self.kind,'index':self.index})
        pool_cycle_returns.append({'kind':self.kind,'index':self.index,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'entry_esp':self.entry_esp,'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'last_error':int(gdb.parse_and_eval('last_error')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'compiler_state':capture_modeled()})
        self.enabled=False;return False
class PoolCommitApiReturn(gdb.Breakpoint):
    def __init__(self,address,index,args):
        super(PoolCommitApiReturn,self).__init__('*0x%x'%address);self.index=index;self.args=args
    def stop(self):
        views=[];view=gdb.parse_and_eval('allocations')
        while int(view):
            item=view.dereference();views.append({'address':int(item['address']),'length':int(item['length'])});view=item['next']
        pool_cycle_events.append({'kind':'return','function':'commit-api','index':self.index})
        pool_commit_api_returns.append({'index':self.index,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'last_error':int(gdb.parse_and_eval('last_error')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),
          'compiler_state':capture_modeled(),'maps':open('/proc/%d/maps'%gdb.selected_inferior().pid).read(),'allocations':views})
        open(COMMITTED_PATH,'wb').write(gdb.selected_inferior().read_memory(self.args[0],self.args[1]).tobytes())
        self.enabled=False;return False
class PoolCommitApiEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller.name()!='CompilerArena::CommitExtent':return False
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));words=list(struct.unpack('<5I',gdb.selected_inferior().read_memory(esp,20).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('(unsigned int)$pc')))
        while block and not block.function:block=block.superblock
        pool_cycle_events.append({'kind':'enter','function':'commit-api','index':len(pool_commit_api_entries)})
        pool_commit_api_entries.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':esp,'return':words[0],'args':words[1:],
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{'this':int(caller.read_var('this').cast(gdb.lookup_type('unsigned int')))}},
          'last_error':int(gdb.parse_and_eval('last_error')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'compiler_state':capture_modeled()})
        PoolCommitApiReturn(words[0],len(pool_commit_api_entries)-1,words[1:]);return False
'''
    code=code.replace('COMMITTED_PATH',repr(str(output/'pool-committed-000.bin')))
    code+='PoolCommitApiEntry("*0x%x")\n'%names['recon_VirtualAlloc'][0]
    return code

def record_returns(probe,kind,entries):
    # This breakpoint executes before the target prologue, so both ESP and the
    # return address are physical observations rather than inferred GDB locals.
    needle='        return False\n'
    assert probe.count(needle)==1
    return probe.replace(needle,"        PoolCycleReturn(%r,len(%s)-1,words[0],esp)\n"%(kind,entries)+needle).replace("'esp':esp,'return':words[0]","'preserved':pool_cycle_preserved(),'fp':capture_x87(),'esp':esp,'return':words[0]")
