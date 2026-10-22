"""Analysis only: check older CRT variants against their shared C1 originals."""
import sys,json,hashlib
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
sys.path.insert(0,str(Path(__file__).resolve().parent))
from binary import PE
from win32_indexes import imports
root=Path(__file__).resolve().parents[4];a=PE(root/'toolchains/1.0-nt/bin/C13232.EXE');b=PE(root/'toolchains/1.0-nt/bin/C1XX3232.EXE')
def records(folder,pe):
 return json.loads((folder/'functions.json').read_text())['driver']+json.loads((folder/'runtime.json').read_text())+imports(pe)
ra=records(root/'decomp/vc1.0/c1',a);rb=records(root/'decomp/vc1.0/c1xx',b)
def target(rs,address):
 matches=[r['name'] for r in rs if r['start']==address]
 return sorted(set(matches))[0] if matches else hex(address)
cs=Cs(CS_ARCH_X86,CS_MODE_32);cs.detail=True
pairs=[('sopen',0x50f50,0x51340,0x80a80,0x80e70),('setvbuf',0x529b0,0x52a6f,0x82580,0x8263f),('freebuf',0x572d0,0x57306,0x877c0,0x877f6),('fseek',0x53f90,0x54026,0x83e00,0x83e96),('tzset',0x59e20,0x59f7b,0x8a830,0x8a98b),('isindst',0x59f80,0x5a043,0x8a990,0x8aa53),('ftell',0x517e0,0x51914,0x813b0,0x814e4)]
def initial_word(pe,address):
 section=next(s for s in pe.sections if s['rva']<=address-pe.base<s['rva']+s['virtual_size'])
 if section['name']=='.bss':return 0
 return int.from_bytes(pe.read(address,4),'little')
reports=[]
for name,x,xe,y,ye in pairs:
 ia=list(cs.disasm(a.read(x,xe-x),x));ib=list(cs.disasm(b.read(y,ye-y),y));assert len(ia)==len(ib),(name,len(ia),len(ib))
 forward={};backward={};edges=[]
 for p,q in zip(ia,ib):
  assert p.mnemonic==q.mnemonic and p.size==q.size,(name,p,q)
  px=bytearray(p.bytes);qx=bytearray(q.bytes)
  if p.operands and p.operands[0].type==X86_OP_IMM and (p.mnemonic=='call' or p.mnemonic.startswith('j')):
   pt,qt=p.operands[0].imm,q.operands[0].imm
   if x<=pt<xe:
    assert pt-x==qt-y,(p,q)
   else:
    pn,qn=target(ra,pt),target(rb,qt);assert pn==qn,(name,hex(p.address),pn,qn);edges.append(pn)
   px[p.imm_offset:p.imm_offset+p.imm_size]=b'\0'*p.imm_size;qx[q.imm_offset:q.imm_offset+q.imm_size]=b'\0'*q.imm_size
  elif p.operands and p.operands[0].type==X86_OP_IMM and p.imm_size==4:
   u,v=p.operands[0].imm,q.operands[0].imm
   if any(sec['name'] in ('.data','.rdata','.bss') and sec['rva']<=u-a.base<sec['rva']+sec['virtual_size'] for sec in a.sections):
    assert forward.setdefault(u,v)==v and backward.setdefault(v,u)==u,(p,q)
    px[p.imm_offset:p.imm_offset+p.imm_size]=b'\0'*p.imm_size;qx[q.imm_offset:q.imm_offset+q.imm_size]=b'\0'*q.imm_size
  for po,qo in zip(p.operands,q.operands):
   if po.type==X86_OP_MEM and any(sec['rva'] <= po.mem.disp-a.base < sec['rva']+sec['virtual_size'] for sec in a.sections):
    u,v=po.mem.disp,qo.mem.disp
    assert forward.setdefault(u,v)==v and backward.setdefault(v,u)==u,(p,q)
    px[p.disp_offset:p.disp_offset+p.disp_size]=b'\0'*p.disp_size;qx[q.disp_offset:q.disp_offset+q.disp_size]=b'\0'*q.disp_size
  assert px==qx,(name,p,q,px.hex(),qx.hex())
 pointer_defaults=[]
 for old_data,new_data in list(forward.items()):
  av,bv=initial_word(a,old_data),initial_word(b,new_data)
  spelling={('tzset',0x77988):'PST',('tzset',0x7798c):'PDT'}.get((name,old_data))
  if spelling:
   assert a.string(av)==b.string(bv)==spelling,'timezone name default differs'
   assert forward.setdefault(av,bv)==bv and backward.setdefault(bv,av)==av,'timezone name identity differs'
   pointer_defaults.append(dict(original=hex(av),c1xx=hex(bv),text=spelling))
  else:
   assert av==bv,(name,hex(old_data),hex(new_data),'initial CRT global differs')
 reports.append(dict(initial_data_values_equal=True,pointer_defaults=pointer_defaults,name=name,original_c1_range=[hex(x),hex(xe)],original_c1xx_range=[hex(y),hex(ye)],instructions=len(ia),all_nonrelocated_instruction_bytes_equal=True,internal_control_flow_equal=True,external_calls=sorted(set(edges)),data_address_aliases_preserved=True,data_aliases={hex(k):hex(v) for k,v in forward.items()}))
print(json.dumps(dict(targets=dict(c1_sha256=hashlib.sha256(a.data).hexdigest(), c1xx_sha256=hashlib.sha256(b.data).hexdigest()),variants=reports),indent=2))
