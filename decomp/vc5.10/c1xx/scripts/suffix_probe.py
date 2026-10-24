"""True native suffix entry/return and pending append ABI; no target mocks."""
from pathlib import Path


def setup(output, binary, collector):
    (output / 'suffix-probe-source.py').write_bytes(Path(__file__).read_bytes())
    checkpoint = output / 'collect-first-suffix.gdb'
    checkpoint.write_text(collector.read_text().replace(
        repr(str(output / 'state.json')), repr(str(output / 'first-suffix-state.json'))
    ).replace(repr(str(output / 'native-argv.bin')),
              repr(str(output / 'first-suffix-argv.bin'))))
    code = r'''python
suffix_entries=[]
suffix_returns=[]
suffix_append_entries=[]
class SuffixReturn(gdb.Breakpoint):
    def __init__(self,entry):
        super(SuffixReturn,self).__init__('*0x%x'%entry['return_pc'],internal=True);self.entry=entry
    def stop(self):
        row=vector_row('actions-suffix-%d-return'%self.entry['index'])
        row.update(index=self.entry['index'],entry_esp=self.entry['esp'],argument_slots_after=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(row['esp'],12))))
        row.update(pattern=action_string(self.entry['args'][0]),argument=action_string(self.entry['args'][1]))
        suffix_returns.append(row);self.enabled=False;return False
class SuffixEntry(gdb.Breakpoint):
    def stop(self):
        index=len(suffix_entries);row=vector_abi(vector_row('actions-suffix-entry-%d'%index),3)
        row.update(index=index,pattern=action_string(row['args'][0]),argument=action_string(row['args'][1]))
        suffix_entries.append(row)
        if not index:gdb.execute('source '+@@CHECKPOINT@@)
        SuffixReturn(row);return False
class SuffixAppendEntry(gdb.Breakpoint):
    def stop(self):
        if suffix_append_entries:return False  # Keep only actual first-prefix alias.
        row=vector_abi(vector_row('suffix-next'),2)
        row.update(value=action_string(row['args'][1]))
        header=gdb.selected_inferior().read_memory(row['args'][0],12).tobytes()
        count,capacity,arena,items=struct.unpack('<hhiI',header)
        if count!=0 or capacity!=40 or arena!=1:raise RuntimeError('pending append known vector header')
        row.update(vector_header=header.hex(),items=gdb.selected_inferior().read_memory(items,160).tobytes().hex(),committed=gdb.selected_inferior().read_memory(row['arena'][2],32768).tobytes().hex())
        suffix_append_entries.append(row);return False
SuffixEntry('*match_option_suffix',internal=True)
SuffixAppendEntry('*append_option_vector',internal=True)
end
'''.replace('@@CHECKPOINT@@', repr(str(checkpoint)))
    finish = 'python\n'
    for name in ('entries', 'returns', 'append_entries'):
        finish += 'open(%r,"w").write(json.dumps(suffix_%s,indent=2)+"\\n")\n' % (
            str(output / ('suffix-' + name.replace('_', '-') + '.json')), name)
    return code, finish + 'end\n'
