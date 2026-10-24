"""Indexed actual option loop/CRC observations; no mutable state is cached."""
from pathlib import Path


def setup(output, binary, collector):
    (output / 'option-action-probe-source.py').write_bytes(Path(__file__).read_bytes())
    checkpoint = output / 'collect-first-action.gdb'
    checkpoint.write_text(collector.read_text().replace(
        repr(str(output / 'state.json')), repr(str(output / 'first-action-state.json'))
    ).replace(repr(str(output / 'native-argv.bin')),
              repr(str(output / 'first-action-argv.bin'))))
    code = r'''python
action_matches=[]
action_match_returns=[]
action_entries=[]
action_returns=[]
action_outer_entries=[]
action_outer_returns=[]
action_inner_entries=[]
action_inner_returns=[]
action_signatures=[]
action_signature_returns=[]
action_integer_entries=[]
def action_string(pointer):
    if not pointer:return ''  # Actual NULL formal; no bytes are read.
    value=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(encoding='latin1',length=1024)
    at=value.find(chr(0))
    if at<0:raise RuntimeError('unbounded actual option string')
    return (value[:at].encode('latin1')+bytes([0])).hex()
def action_row(phase,count):
    return vector_abi(vector_row('actions-'+phase),count)
class ActionReturn(gdb.Breakpoint):
    def __init__(self,entry,phase,rows,value=False,signature=False):
        super(ActionReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True)
        self.entry=entry;self.phase=phase;self.rows=rows;self.value=value;self.signature=signature
    def stop(self):
        row=vector_row('actions-'+self.phase+'-%d-return'%self.entry['index'])
        row.update(entry_esp=self.entry['esp'],index=self.entry['index'],eflags=int(gdb.parse_and_eval('(unsigned int)$eflags')))
        if self.value:row['value']=action_string(row['eax'])
        if self.signature:
            row['argument_slots_after']=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12)))
            row['input']=gdb.selected_inferior().read_memory(self.entry['args'][0],self.entry['args'][1]+1).tobytes().hex()
            row['vector']=gdb.selected_inferior().read_memory(row['arena'][2],32768).tobytes().hex()
        self.rows.append(row);self.enabled=False;return False
class ActionMatch(gdb.Breakpoint):
    def stop(self):
        index=len(action_matches);row=action_row('match-entry-%d'%index,4)
        row.update(index=index,argument=action_string(row['args'][1]))
        action_matches.append(row);ActionReturn(row,'match',action_match_returns);return False
class ActionHandler(gdb.Breakpoint):
    def stop(self):
        index=len(action_entries);row=action_row('action-entry-%d'%index,3)
        row.update(index=index,value=action_string(row['args'][1]))
        action_entries.append(row)
        if not index:gdb.execute('source '+@@CHECKPOINT@@)
        ActionReturn(row,'action',action_returns);return False
class ActionNext(gdb.Breakpoint):
    def stop(self):
        inner=gdb.newest_frame().older().name()=='match_option'
        entries=action_inner_entries if inner else action_outer_entries
        returns=action_inner_returns if inner else action_outer_returns
        phase='inner'if inner else'outer';index=len(entries)
        row=action_row(phase+'-entry-%d'%index,0);row['index']=index
        entries.append(row);ActionReturn(row,phase,returns,value=True);return False
class ActionSignature(gdb.Breakpoint):
    def stop(self):
        index=len(action_signatures);row=action_row('signature-%d-entry'%index,3)
        count=row['args'][1]
        if count>1023:raise RuntimeError('unbounded actual public signature input')
        row.update(index=index,input=gdb.selected_inferior().read_memory(row['args'][0],count+1).tobytes().hex(),vector=gdb.selected_inferior().read_memory(row['arena'][2],32768).tobytes().hex())
        action_signatures.append(row);ActionReturn(row,'signature',action_signature_returns,signature=True);return False
class ActionInteger(gdb.Breakpoint):
    def stop(self):
        row=action_row('integer-entry',1);row['value']=action_string(row['args'][0])
        action_integer_entries.append(row);return False
ActionMatch('*match_option',internal=True)
ActionHandler('*apply_option_action',internal=True)
ActionNext('*next_argument',internal=True)
ActionSignature('*recon_mspdb_SigForPbCb',internal=True)
ActionInteger('*parse_option_integer',internal=True)
end
'''.replace('@@CHECKPOINT@@', repr(str(checkpoint)))
    finish = 'python\n'
    for name in ('matches', 'match_returns', 'entries', 'returns', 'outer_entries',
                 'outer_returns', 'inner_entries', 'inner_returns', 'signatures',
                 'signature_returns', 'integer_entries'):
        finish += 'open(%r,"w").write(json.dumps(action_%s,indent=2)+"\\n")\n' % (
            str(output / ('actions-' + name.replace('_', '-') + '.json')), name)
    return code, finish + 'end\n'
