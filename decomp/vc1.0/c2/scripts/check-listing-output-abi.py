"""Prove listing/output entry ABIs in original and canonical machine code."""
from pathlib import Path
import importlib.util,json,time
from pe_indexes import indexes
started=time.perf_counter();spec=importlib.util.spec_from_file_location('codeview_abi_graph',Path('scripts/audit-graph.py'));graph=importlib.util.module_from_spec(spec);spec.loader.exec_module(graph)
base=graph.FunctionDecoder;observed=[]
class Decoder(base):
    def __init__(self,*args):super().__init__(*args);self.visited={};observed.append(self)
    def instruction(self,address):
        item=super().instruction(address)
        if item is not None:self.visited[address]=item
        return item
graph.FunctionDecoder=Decoder
original,canonical,config,left,right=indexes();checked=[]
for name in config['accepted'][1162:1180]:
    assert name.startswith('@')
    expected=max(int(name.rsplit('@',1)[1])-8,0)
    sides=[]
    for image,records in [(original,left),(canonical,right)]:
        row=next(r for r in records if r['name']==name);graph.edges(image,records,row);instructions=list(observed[-1].visited.values());returns=[dict(address=hex(a),stack_bytes=int(op,0)if op else 0)for a,size,mn,op in instructions if mn.split()[-1]=='ret']
        assert returns and all(r['stack_bytes']==expected for r in returns),(name,returns)
        sides.append(returns)
    checked.append(dict(name=name,original=sides[0],canonical=sides[1],callee_stack_bytes=expected,abi='fastcall ECX/EDX with any additional arguments callee-cleaned; byte/word widths declared in original prototypes'))
Path('cases/listing-output-abi-validation.json').write_text(json.dumps(dict(checked=checked,seconds=time.perf_counter()-started,accepted_slice=[1162,1180],word_argument='coff_write_word original6eff3 storesCX; original/native frontier observed65529, source unsigned short parameter preserves low16 bits',fill_arguments='coff_fill_bytes CL/EDX byte/count are independently exercised by original/native isolated fill probe'),indent=2)+'\n');print(len(checked),'listing/output cleanup ABIs match')
