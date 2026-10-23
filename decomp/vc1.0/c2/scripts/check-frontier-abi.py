"""Audit original/canonical RET cleanup for the explicitly resumed C2 queue."""
import importlib.util
import json
from pathlib import Path
import time
from pe_indexes import indexes

started = time.perf_counter()
spec = importlib.util.spec_from_file_location('frontier_abi_graph',Path('scripts/audit-graph.py'))
graph = importlib.util.module_from_spec(spec)
spec.loader.exec_module(graph)
base_decoder = graph.FunctionDecoder
captures = []
class ObservedDecoder(base_decoder):
    def __init__(self,*args):
        super().__init__(*args)
        self.visited = {}
        captures.append(self)
    def instruction(self,address):
        instruction = super().instruction(address)
        if instruction is not None:
            self.visited[address] = instruction
        return instruction
graph.FunctionDecoder = ObservedDecoder
original, rebuilt, config, originals, canonical = indexes()
checked = []
first, last = 1074, 1089
assert len(config['accepted']) >= last
for name in config['accepted'][first:last]:
    assert name.startswith('@')
    expected = max(int(name.rsplit('@',1)[1])-8,0)
    rows = []
    for image, records in [(original,originals),(rebuilt,canonical)]:
        record = next(r for r in records if r['name'] == name)
        graph.edges(image,records,record)
        returns = [dict(address=hex(address),stack_bytes=int(op,0) if op else 0)
                   for address,size,mnemonic,op in captures[-1].visited.values()
                   if mnemonic.split()[-1] == 'ret']
        assert returns and all(r['stack_bytes'] == expected for r in returns),(name,returns,expected)
        rows.append(returns)
    checked.append(dict(name=name,callee_stack_bytes=expected,original=rows[0],canonical=rows[1]))
receipt = dict(method='CFG traversal of original/canonical machine RET operands; jump-table data not decoded as instructions',
    checkpoint=last,accepted_slice=[first,last],checked=checked,seconds=time.perf_counter()-started,
    return_width_evidence='scheduler_instruction_cycles original defines AX; all reconstructed callers explicitly consume word/byte or receive its unsigned-short result')
Path('cases/resumed-frontier-abi-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('%d original/canonical function cleanup ABIs match'%len(checked))
