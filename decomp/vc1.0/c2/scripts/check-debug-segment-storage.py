"""Prove segment globals share their original storage through real operands."""
import importlib.util
import json
from pathlib import Path
import time
from capstone.x86_const import X86_OP_MEM, X86_OP_IMM
from pe_indexes import indexes, publics

started=time.perf_counter()
spec=importlib.util.spec_from_file_location('segment_graph',Path('scripts/audit-graph.py'))
graph=importlib.util.module_from_spec(spec);spec.loader.exec_module(graph)
base=graph.FunctionDecoder
observed=[]
class Decoder(base):
    def __init__(self,*args):
        super().__init__(*args);self.visited={};observed.append(self)
    def instruction(self,address):
        item=super().instruction(address)
        if item is not None:self.visited[address]=item
        return item
graph.FunctionDecoder=Decoder
original,canonical,config,originals,canonicals=indexes()
original_rows={r['name']:r for r in originals};canonical_rows={r['name']:r for r in canonicals}
names=publics()
slots={0x79144:'directive_segment',0x79148:'coff_function_fpo_segment',0x7914c:'debug_symbols_segment',
       0x7b8d0:'bss_segment',0x7b8d4:'code_segment',0x7b8d8:'data_segment',0x7b8dc:'const_segment',
       0x7b8e0:'debug_types_segment',0x7b8e8:'tls_segment'}
assert '_coff_function_debug_segment' not in names,'Duplicate storage for original7914c'
assert len({names['_'+name] for name in slots.values()})==len(slots)

def references(image,records,row,addresses):
    graph.edges(image,records,row);decoder=observed[-1];found={}
    for address,size,mnemonic,operands in decoder.visited.values():
        if not any(hex(a) in operands for a in addresses):continue
        ins=decoder.detail(address,size)
        for op in ins.operands:
            value=None
            if op.type==X86_OP_MEM and not op.mem.base and not op.mem.index:value=op.mem.disp&0xffffffff
            elif op.type==X86_OP_IMM and ins.mnemonic in ('mov','push'):value=op.imm&0xffffffff
            if value in addresses:found.setdefault(value,[]).append(dict(address=hex(address),instruction=mnemonic+' '+operands,operand_kind='absolute memory' if op.type==X86_OP_MEM else 'address value'))
    return found

rows=[]
for row in config['driver']:
    if row['name'] not in config['accepted']:continue
    left=references(original,originals,row,slots)
    if not left:continue
    right=references(canonical,canonicals,canonical_rows[row['name']],{names['_'+slots[a]] for a in left})
    for address,uses in left.items():
        symbol='_'+slots[address];target=names[symbol]
        assert target in right,(row['name'],hex(address),symbol,'original storage lost/split')
        rows.append(dict(function=row['name'],original_storage=hex(address),canonical_symbol=symbol,
                         canonical_storage=hex(target),original_uses=uses,canonical_uses=right[target]))
assert {r['function'] for r in rows if r['original_storage']=='0x7914c'} >= {
    '_initialize_segments','_prepare_backend_symbols','_write_codeview_signature','_write_codeview_object_name'}
# A second all-zero storage is insufficient: rebinding7914c to bss must fail
# actual signature-writer operand identity, independently of stored values.
writer=canonical_rows['_write_codeview_signature']
wrong=names['_bss_segment'];right=references(canonical,canonicals,writer,{wrong})
assert wrong not in right,'Invalid negative: signature writer unexpectedly uses bss global'
receipt=dict(checkpoint=len(config['accepted']),method='Accepted original CFG instruction operands mapped to canonical CFG references; absolute MEM and address-construction MOV/PUSH IMM, never raw numeric byte matching',
 slots=[dict(original=hex(a),canonical='_'+n,address=hex(names['_'+n]))for a,n in slots.items()],checked=rows,
 negative='Rebinding original7914c to separate zero-valued bss storage rejected by actual signature writer operands',
 boundary_exclusion='coff_finish_module6c9db CMP79144 is the one-past end of16-entry index79104..79144, not a directive_segment pointer reference; unchanged original/canonical array-bound graph retained',
 limit='This verifies the introduced segment/debug global domain and global address identity; it does not infer all indirect heap aliases or claim branch coverage.',seconds=time.perf_counter()-started)
Path('cases/debug-segment-storage-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('%d function/storage operand pairs match; split-storage negative rejected'%len(rows))
