"""Explicit input/static-evidence derivation only, never called by fixtures.

Run on a scratch copy of the case directory; committed references are immutable.
"""
import sys,json,hashlib,struct
from pathlib import Path
root=Path(__file__).resolve().parents[3];sys.path.insert(0,str(root/'scripts'))
from binary import PE
from capstone import *
from capstone.x86 import *
pe=PE(root/'../../../toolchains/1.0-nt/bin/LINK.EXE');out=root/'cases/legacy-resource-diagnostics'
sha=lambda x:hashlib.sha256(x).hexdigest()
raw=(root/'cases/legacy-win16-resources/inputs/legacy.res').read_bytes();p=0;payloads=[]
def name():
 global p
 if raw[p]==255:r=struct.unpack_from('<H',raw,p+1)[0];p+=3;return r
 e=raw.index(0,p);r=raw[p:e].decode('cp1252');p=e+1;return r
while p<len(raw):
 t,n=name(),name();flags,size=struct.unpack_from('<HI',raw,p);p+=6;start=p;data=raw[p:p+size];p+=size
 if t in [4,5,6]:
  label={4:'menu',5:'dialog',6:'strings'}[t];(out/'inputs'/f'{label}.bin').write_bytes(data)
  table=struct.unpack('<IIII',pe.read(0x7ff98+t*16,16));payloads.append(dict(type=t,name=n,label=label,source_offset=start,size=size,sha256=sha(data),table_words=list(table),function_va=table[2]))
assert p==len(raw)
md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
cfg=json.loads((root/'cases/legacy-resource-diagnostics/analysis/function-extents.json').read_text())
for x in cfg:
 a,b=int(x['start'],16),int(x['end'],16)
 lines=[];strings={}
 for i in md.disasm(pe.read(a,b-a),a):
  lines.append('%08x %s %-8s %s'%(i.address,i.bytes.hex(),i.mnemonic,i.op_str))
  for op in i.operands:
   if op.type==X86_OP_IMM and 0x80100<=op.imm<0x805a0:strings[hex(op.imm)]=pe.string(op.imm)
 (out/'analysis'/(x['name']+'.txt')).write_text('\n'.join(lines)+'\n')
 x['extent_sha256']=sha(pe.read(a,b-a));x['strings']=strings
(out/'analysis/function-extents.json').write_text(json.dumps(cfg,indent=2)+'\n')
text=next(s for s in pe.sections if s['flags']&0x20);refs=[];debug=0x7f8d0;callbackvas=[x['function_va'] for x in payloads]
md.skipdata=True
for i in md.disasm(pe.data[text['off']:text['off']+text['size']],pe.base+text['rva']):
 if not i.id:continue
 for op in i.operands:
  match=(op.type==X86_OP_MEM and op.mem.disp==debug) or (op.type==X86_OP_IMM and op.imm in callbackvas+[0x7ff98])
  if match:refs.append(dict(address=hex(i.address),bytes=i.bytes.hex(),instruction=i.mnemonic+' '+i.op_str))
for a,b,n in [(0x20fa0,0x210c6,'public-adapter'),(0x42870,0x42e10,'converter-driver'),(0x440a0,0x44270,'converter-dispatch')]:
 (out/'analysis'/(n+'.txt')).write_text('\n'.join('%08x %s %-8s %s'%(i.address,i.bytes.hex(),i.mnemonic,i.op_str) for i in md.disasm(pe.read(a,b-a),a))+'\n')
proof=dict(status='independent static proof frozen before external tests',original_sha256=sha(pe.data),original_image_base=pe.base,table_va=0x7ff98,table_sha256=sha(pe.read(0x7ff98,256)),table_bytes=pe.read(0x7ff98,256).hex(),source_resource=dict(path='../legacy-win16-resources/inputs/legacy.res',sha256=sha(raw),format='Unaligned Win16 .res record: ordinal FF/WORD or ANSIz names, WORD flags, DWORD payload length',original_validated_case='../legacy-win16-resources'),payloads=payloads,resource_debug=dict(va=debug,initial_u32=struct.unpack('<I',pe.read(debug,4))[0]),xref_candidates=refs,public_adapter=dict(va=0x20fa0,end=0x210c6,sha256=sha(pe.read(0x20fa0,0x126)),argc=5,argv=['cvtres','-i386 or -mips','-o','generated temporary name','actual resource input'],converter_call=0x21092,debug_option_absent=True),reachability='External direct minimal code tests only; no CLI reachability claim. Initial resource_debug=0; only converter -d branch sets1; public adapter never supplies it. Full listings and raw typed table freeze that invariant.',class_quirk='Original byte is MOVSX to short then MOVSX to int before subtract0x80; 0x80..0x85 become negative and take unsigned-default class branch. Table labels are statically included but not claimed exercised.')
(out/'analysis/provenance.json').write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps(payloads,indent=2));print('xrefs',refs)
