"""Physical vector and arena1 observations; provider entry is an external stop."""
from pathlib import Path
from scaler_probe import symbols


def setup(output,binary,collector,argument=False,matcher=False):
    (output/'vector-probe-source.py').write_bytes(Path(__file__).read_bytes());names=symbols(binary)
    prefix=output/'collect-vector-prefix.gdb'
    prefix.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'vector-prefix-state.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'vector-prefix-argv.bin'))))
    argument_checkpoint=output/'collect-argument-entry.gdb'
    if argument:argument_checkpoint.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'argument-entry-state.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'argument-entry-argv.bin'))))
    public_checkpoint=output/'collect-public-signature-entry.gdb';matcher_checkpoint=output/'collect-matcher-entry.gdb'
    if matcher:
        for script,state,argv in [(public_checkpoint,'signature-entry-state.json','public-signature-entry-argv.bin'),(matcher_checkpoint,'matcher-entry-state.json','matcher-entry-argv.bin')]:
            if script.exists():raise ValueError('collector path already owned: '+script.name)
            script.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/state))).replace(repr(str(output/'native-argv.bin')),repr(str(output/argv))))
    code='''python
vector_entries=[]
vector_returns=[]
vector_member_entries=[]
vector_member_returns=[]
vector_api_entries=[]
vector_api_returns=[]
vector_public_entries=[]
vector_next_entries=[]
argument_returns=[]
argument_matchers=[]
signature_returns=[]
matcher_next_entries=[]
matcher_next_returns=[]
matcher_actions=[]
def vector_row(phase):
    row=fold_row();arena=gdb.parse_and_eval('frontend_bss.arenas[1]');address=int(arena.address)
    row.update(arena_address=address,arena=list(struct.unpack('<8I',gdb.selected_inferior().read_memory(address,32))),locale=checksum_locale_state(phase))
    return row
def vector_abi(row,count):
    words=struct.unpack('<%dI'%(count+1),gdb.selected_inferior().read_memory(row['esp'],4*(count+1)))
    row.update(return_pc=words[0],args=list(words[1:]),formal_inventory=checksum_formals(row['pc']),caller=gdb.newest_frame().older().name(),eflags=int(gdb.parse_and_eval('(unsigned int)$eflags')))
    return row
class VectorReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(VectorReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        index=len(vector_returns);row=vector_row('vector-%d-return'%index);raw=gdb.selected_inferior().read_memory(row['eax'],12).tobytes();count,capacity,arena_index,items=struct.unpack('<hhII',raw)
        row.update(entry_esp=self.entry['esp'],header=[count,capacity,arena_index,items],raw_header=raw.hex(),items=gdb.selected_inferior().read_memory(items,160).tobytes().hex())
        open(@@OUTPUT@@+'/vector-%d-committed.bin'%index,'wb').write(gdb.selected_inferior().read_memory(row['arena'][2],row['arena'][3]).tobytes())
        vector_returns.append(row);self.enabled=False;return False
class VectorEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_row('vector-%d-entry'%len(vector_entries)),2);typ=gdb.lookup_type('CompilerVector');row['layout']={'size':int(typ.sizeof),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof),str(f.type)]for f in typ.fields()}}
        if not vector_entries:gdb.execute('source '+@@PREFIX@@)
        vector_entries.append(row);VectorReturn(row);return False
class VectorMemberReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(VectorMemberReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('vector-member-return');row['entry_esp']=self.entry['esp'];vector_member_returns.append(row);self.enabled=False;return False
class VectorMemberEntry(gdb.Breakpoint):
    def stop(self):
        if gdb.newest_frame().older().name()!='new_option_vector':return False
        row=vector_abi(vector_row('vector-member-entry'),0);vector_member_entries.append(row);VectorMemberReturn(row);return False
class VectorApiReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(VectorApiReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('vector-API-return');row.update(entry_esp=self.entry['esp'],maps=open('/proc/%d/maps'%gdb.selected_inferior().pid).read());views=[];view=gdb.parse_and_eval('allocations')
        while int(view):
            if len(views)>=32:raise RuntimeError('vector reservation inventory unbounded')
            item=view.dereference();views.append({'address':int(item['address']),'bytes':int(item['length'])});view=item['next']
        row['owned_reservations']=views;vector_api_returns.append(row)
        args=self.entry['args']
        if args[1:]!=[32768,4096,4]or row['eax']!=args[0]:raise RuntimeError('unknown actual vector commit outcome')
        open(@@OUTPUT@@+'/vector-commit-zero.bin','wb').write(gdb.selected_inferior().read_memory(args[0],args[1]).tobytes());self.enabled=False;return False
class VectorApiEntry(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older()
        if caller.name()!='CompilerArena::CommitExtent'or int(caller.read_var('this'))!=int(gdb.parse_and_eval('&frontend_bss.arenas[1]')):return False
        row=vector_abi(vector_row('vector-API-entry'),4);row['caller_this']=int(caller.read_var('this'));vector_api_entries.append(row);VectorApiReturn(row);return False
class ArgumentReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(ArgumentReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('argument-return');row.update(entry_esp=self.entry['esp'],result=gdb.selected_inferior().read_memory(row['eax'],4).tobytes().hex());argument_returns.append(row);self.enabled=False;return False
class ArgumentMatcher(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_row('argument-next'),4);argument_matchers.append(row)
        if @@MATCHER@@:gdb.execute('source '+@@MATCHER_CHECKPOINT@@)
        return False
class VectorNextEntry(gdb.Breakpoint):
    def stop(self):
        if @@MATCHER@@ and gdb.newest_frame().older().name()=='match_option':
            row=vector_abi(vector_row('matcher-next-entry'),0);matcher_next_entries.append(row);MatcherNextReturn(row);return False
        row=vector_abi(vector_row('vector-next'),0);vector_next_entries.append(row)
        if @@ARGUMENT@@:
            gdb.execute('source '+@@ARGUMENT_CHECKPOINT@@);ArgumentReturn(row)
        return False
class VectorPublicEntry(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_row('signature-entry'),3);row['input']=gdb.selected_inferior().read_memory(row['args'][0],4).tobytes().hex();vector_public_entries.append(row)
        if @@MATCHER@@:
            gdb.execute('source '+@@PUBLIC_CHECKPOINT@@);SignatureReturn(row);return False
        print('C1XX_PUBLIC_FRONTIER MSPDB50!SigForPbCb');return True
class SignatureReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(SignatureReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('signature-return');row.update(entry_esp=self.entry['esp'],argument_slots_after=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12))),eflags=int(gdb.parse_and_eval('(unsigned int)$eflags')));signature_returns.append(row);self.enabled=False;return False
class MatcherNextReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(MatcherNextReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('matcher-next-return');value=gdb.Value(row['eax']).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1')
        if len(value)>=256:raise RuntimeError('matcher next value unbounded')
        row.update(entry_esp=self.entry['esp'],value=(value.encode('latin1')+bytes([0])).hex());matcher_next_returns.append(row);self.enabled=False;return False
class MatcherAction(gdb.Breakpoint):
    def stop(self):
        row=vector_abi(vector_row('matcher-action'),3);value=gdb.Value(row['args'][1]).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1')
        if len(value)>=256:raise RuntimeError('matcher action value unbounded')
        row['value']=(value.encode('latin1')+bytes([0])).hex();matcher_actions.append(row);return False
if @@MATCHER@@:MatcherAction('*apply_option_action',internal=True)
VectorEntry('*0x%x'%@@ENTRY@@,internal=True)
VectorMemberEntry('*0x%x'%@@MEMBER@@,internal=True)
VectorApiEntry('*recon_VirtualAlloc',internal=True)
VectorPublicEntry('*recon_mspdb_SigForPbCb',internal=True)
VectorNextEntry('*next_argument',internal=True)
if @@ARGUMENT@@:ArgumentMatcher('*match_option',internal=True)
end
'''.replace('@@MATCHER@@',repr(matcher)).replace('@@PUBLIC_CHECKPOINT@@',repr(str(public_checkpoint))).replace('@@MATCHER_CHECKPOINT@@',repr(str(matcher_checkpoint))).replace('@@ARGUMENT@@',repr(argument)).replace('@@ARGUMENT_CHECKPOINT@@',repr(str(argument_checkpoint))).replace('@@PREFIX@@',repr(str(prefix))).replace('@@OUTPUT@@',repr(str(output))).replace('@@ENTRY@@',str(names['new_option_vector'][0])).replace('@@MEMBER@@',str(names['_ZN13CompilerArena12CommitExtentEv'][0]))
    finish='python\n'
    for name in ('entries','returns','member_entries','member_returns','api_entries','api_returns','public_entries','next_entries'):
        finish+='open(%r,"w").write(json.dumps(vector_%s,indent=2)+"\\n")\n'%(str(output/('vector-'+name.replace('_','-')+'.json')),name)
    for name in ('returns','matchers'):finish+='open(%r,"w").write(json.dumps(argument_%s,indent=2)+"\\n")\n'%(str(output/('argument-'+name+'.json')),name)
    for name,var in [('signature-returns','signature_returns'),('matcher-next-entries','matcher_next_entries'),('matcher-next-returns','matcher_next_returns'),('matcher-actions','matcher_actions')]:finish+='open(%r,"w").write(json.dumps(%s,indent=2)+"\\n")\n'%(str(output/(name+'.json')),var)
    return code,finish+'end\n'
