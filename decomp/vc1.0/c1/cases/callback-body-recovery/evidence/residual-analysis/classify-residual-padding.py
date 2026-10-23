from pathlib import Path
import json,sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
R=Path('/root/msvc2');B=R/'decomp/vc1.0/c1';S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');sys.path.insert(0,str(B/'scripts'));from binary import PE
pe=PE(R/'toolchains/1.0-nt/bin/C13232.EXE');D=Cs(CS_ARCH_X86,CS_MODE_32);j=json.loads((S/'analysis/residual-largest-1009.json').read_text());rows=[]
for r in j['intervals']:
 a,b=int(r['start'],16),int(r['end'],16);raw=pe.read(a,b-a);ins=list(D.disasm(raw,a));patterns=[];offset=a
 for x in ins:
  if x.address!=offset:break
  op=x.op_str.replace(' ','')
  yes=x.mnemonic=='nop'or(x.mnemonic=='mov'and len(op.split(','))==2 and op.split(',')[0]==op.split(',')[1])or(x.mnemonic=='lea'and op in ['esp,[esp]','ecx,[ecx]','eax,[eax]','edi,[edi]','esi,[esi]','ebx,[ebx]','ebp,[ebp]','edx,[edx]'])or(x.mnemonic=='add'and op=='eax,0')
  if not yes:break
  patterns.append(x.mnemonic+' '+x.op_str);offset+=x.size
 candidate=offset==b and not r['pending_entries'] and b%4==0
 rows.append(dict(**r,padding_candidate=candidate,recognized_instructions=patterns if candidate else None))
count=sum(r['bytes']for r in rows if r['padding_candidate']);out=dict(mixed_bytes=j['total_mixed_bytes'],recognized_alignment_candidates=count,other_mixed_bytes=j['total_mixed_bytes']-count,policy='Read-only analysis, zero acceptance credit. Candidate only: complete isolated interval decodes to recognized MSVC no-op/alignment sequence, no named pending entry, following boundary aligned. Includes ADD EAX,0 flag-setting filler; no claim unreachable without full reference/CFG proof. Unknown/table/data/CRT/function classification remains pending.',intervals=rows);(S/'analysis/residual-padding-candidates-1009.json').write_text(json.dumps(out,indent=2)+'\n');print(count,'padding-pattern candidate bytes; othermixed',out['other_mixed_bytes'])
