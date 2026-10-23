"""Native-only GDB observations of the real reserve/commit calls and returns."""
import re
import subprocess


def setup(output,binary,collector,allow_arena=False):
    table=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
    def symbol(name):return int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] '+name+'$',table,re.M)[1],16)
    for role in ('entry','return'):
        for index in range(1,3):
            name='reserve-%s-%02d'%(role,index);path=output/('collect-'+name+'.gdb')
            path.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(name+'.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(name+'-argv.bin')))))
    script=output/'observe-allocation.gdb'
    body='''python
import gdb,json,struct
allocation_output=@@OUTPUT@@
allocation_api_entries=[]
allocation_api_returns=[]
allocation_helper_entries=[]
allocation_helper_returns=[]
def allocation_context():
    return {name:int(gdb.parse_and_eval('(unsigned int)$'+name)) for name in ('pc','esp','eax','edi','esi','ebx')}
def allocation_error():return int(gdb.parse_and_eval('last_error'))
def allocation_fpu():return int(gdb.parse_and_eval('$fctrl'))
class AllocationApiReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(AllocationApiReturn,self).__init__('*0x%x'%address,internal=True)
        self.index=index
    def stop(self):
        row=allocation_context();row.update(index=self.index,last_error=allocation_error(),fpu=allocation_fpu(),maps=open('/proc/%d/maps'%gdb.selected_inferior().pid).read())
        allocation_api_returns.append(row)
        if self.index%2:
            if not row['eax']:raise RuntimeError('uncharacterized native commit failure')
            payload=bytes(gdb.selected_inferior().read_memory(row['eax'],0x4000))
            open(allocation_output+'/work-committed-%02d.bin'%(self.index//2+1),'wb').write(payload)
        self.enabled=False
        return False
class AllocationApiEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        if @@ALLOW_ARENA@@ and frame.older().name()=='CompilerArena::Reserve':return False
        if frame.older().name()!='reserve_work_buffer':raise RuntimeError('unexpected VirtualAlloc caller')
        row=allocation_context();words=list(struct.unpack('<5I',bytes(gdb.selected_inferior().read_memory(row['esp'],20))))
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        formals=[{'name':item.name,'type':str(item.type)} for item in block if item.is_argument]
        index=len(allocation_api_entries)
        if index>=4:raise RuntimeError('extra native VirtualAlloc call')
        row.update(index=index,args=words[1:],return_pc=words[0],formal_inventory=formals,last_error=allocation_error(),fpu=allocation_fpu())
        allocation_api_entries.append(row);AllocationApiReturn(words[0],index)
        return False
class AllocationHelperReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(AllocationHelperReturn,self).__init__('*0x%x'%address,internal=True)
        self.index=index
    def stop(self):
        row=allocation_context();row.update(index=self.index,last_error=allocation_error(),fpu=allocation_fpu())
        allocation_helper_returns.append(row)
        gdb.execute('source '+allocation_output+'/collect-reserve-return-%02d.gdb'%(self.index+1))
        self.enabled=False
        return False
class AllocationHelperEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();row=allocation_context();words=list(struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(row['esp'],12))))
        block=gdb.block_for_pc(row['pc'])
        while block and not block.function:block=block.superblock
        formals=[{'name':item.name,'type':str(item.type)} for item in block if item.is_argument]
        index=len(allocation_helper_entries)
        if index>=2 or frame.older().name()!='initialize_work_buffers':raise RuntimeError('unexpected reserve helper call')
        row.update(index=index,args=words[1:],return_pc=words[0],formal_inventory=formals,last_error=allocation_error(),fpu=allocation_fpu())
        allocation_helper_entries.append(row)
        gdb.execute('source '+allocation_output+'/collect-reserve-entry-%02d.gdb'%(index+1))
        AllocationHelperReturn(words[0],index)
        return False
AllocationApiEntry('*0x%x'%@@API@@,internal=True)
AllocationHelperEntry('*0x%x'%@@HELPER@@,internal=True)
end
'''.replace('@@OUTPUT@@',repr(str(output))).replace('@@API@@',str(symbol('recon_VirtualAlloc'))).replace('@@HELPER@@',str(symbol('reserve_work_buffer'))).replace('@@ALLOW_ARENA@@',repr(allow_arena))
    script.write_text(body)
    finish='python\n'
    for name in ('api_entries','api_returns','helper_entries','helper_returns'):
        finish+='open('+repr(str(output/('allocation-'+name.replace('_','-')+'.json')))+',"w").write(json.dumps(allocation_'+name+',indent=2)+"\\n")\n'
    finish+='end\n'
    return 'source '+str(script)+'\n',finish
