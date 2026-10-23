#!/usr/bin/env python3
"""Approved main-only endpoint proof, supplementary to canonical CFG checks.
No recursive discovery, fabricated RET convention, CRT acceptance or execution.
"""
import hashlib,json,re,struct,subprocess
from pathlib import Path
from inventory import ROOT, inventory
BINARY=Path(__file__).resolve().parents[1]
OUT=Path('/root/.ai-tmp/msvc2/watcom10.0a/wlink/main-frontier-04')

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def pe(path):
 data=path.read_bytes();header=struct.unpack_from('<I',data,0x3c)[0];opt=header+24
 base=struct.unpack_from('<I',data,opt+28)[0];count=struct.unpack_from('<H',data,header+6)[0];sec=opt+struct.unpack_from('<H',data,header+20)[0]
 sections=[struct.unpack_from('<IIII',data,sec+i*40+8) for i in range(count)]
 def offset(va):
  rva=va-base
  for virtual_size,start,size,file in sections:
   if start<=rva<start+size:return file+rva-start
  raise ValueError('unmapped VA')
 def read(va,size):return data[offset(va):offset(va)+size]
 imports={};desc=offset(base+struct.unpack_from('<I',data,opt+104)[0])
 while any(data[desc:desc+20]):
  lookup,_,_,name,first=struct.unpack_from('<IIIII',data,desc);at=offset(base+name);dll=data[at:data.index(b'\0',at)].decode();i=0
  while True:
   item=struct.unpack_from('<I',data,offset(base+(lookup or first)+i*4))[0]
   if not item:break
   assert not item&0x80000000
   at=offset(base+item)+2;imports[base+first+i*4]=(dll,data[at:data.index(b'\0',at)].decode());i+=1
  desc+=20
 return read,imports

def main_endpoint(path,start,end,exit_address):
 # A stronger whole-extent linear ABI check: reject RET anywhere, even if
 # unreachable. Canonical shared CFG supplies actual graph/ownership proof.
 cmd=['objdump','-d','-Mintel','--insn-width=15','--start-address='+hex(start),'--stop-address='+hex(end),str(path)]
 p=subprocess.run(cmd,capture_output=True,check=True);instructions=[];cursor=start;tails=[]
 for line in p.stdout.decode().splitlines():
  m=re.match(r'^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*(.*)$',line)
  if not m:continue
  address=int(m[1],16);raw=bytes.fromhex(m[2]);asm=m[3].split();assert address==cursor and raw and asm, line;cursor+=len(raw)
  mnemonic=asm[0];assert mnemonic not in ('ret','retf','iret','iretd','lret','(bad)'),line
  if mnemonic.startswith('j') or mnemonic.startswith('loop'):
   assert len(asm)>=2 and re.fullmatch(r'(?:0x)?[0-9a-f]+',asm[1]),line
   target=int(asm[1],16)
   if not start<=target<end:
    assert mnemonic=='jmp' and cursor==end and target==exit_address,line
    tails.append(dict(address=address,target=target,bytes=raw.hex()))
  instructions.append(dict(address=address,bytes=raw.hex(),assembly=m[3]))
 assert cursor==end and len(tails)==1
 read,_=pe(path)
 return dict(start=start,end_exclusive=end,body_sha256=hashlib.sha256(read(start,end-start)).hexdigest(),ret_instructions_anywhere=0,sole_external_tail=tails[0],instruction_count=len(instructions))

def main():
 inventory()
 r=subprocess.run(['python3',str(BINARY/'scripts/audit-main-candidate.py')],capture_output=True)
 assert r.returncode==0,r.stderr.decode()
 graph=Path('/root/.ai-tmp/msvc2/watcom10.0a/wlink/main-frontier-01/graph.stdout.txt')
 original=ROOT/'toolchains/watcom10.0a/binnt/WLINK.EXE';rebuilt=BINARY/'build/original/WLINK.EXE'
 symbols={m[2]:int(m[1],16)+0x410000 for line in (BINARY/'build/original/wlink.map').read_text().splitlines() if (m:=re.match(r'0001:([0-9a-fA-F]+)\s*[+*]?\s*(\S+)',line))}
 o,oi=pe(original);b,bi=pe(rebuilt)
 assert o(0x433fc5,5)==bytes.fromhex('e981190000')
 assert o(0x43594b,2)==bytes.fromhex('89c2')
 assert o(0x43595d,6)==bytes.fromhex('52e8c53d0000')
 assert o(0x439728,6)==bytes.fromhex('ff25b0004600') and oi[0x4600b0]==('KERNEL32.DLL','ExitProcess')
 # Shipped canonical CRT differs inside the permitted boundary. Verify real
 # exit_ calls its own _exit_, which tails to its own __exit_ and ExitProcess.
 ex,low,nt=symbols['exit_'],symbols['_exit_'],symbols['__exit_']
 assert b(ex,3)==bytes.fromhex('5289c2') and b(ex+17,1)==b'\xe8'
 assert ex+22+struct.unpack('<i',b(ex+18,4))[0]==low
 assert b(low+32,1)==b'\xe9' and low+37+struct.unpack('<i',b(low+33,4))[0]==nt
 assert b(nt+25,2)==bytes.fromhex('53e8')
 thunk=nt+31+struct.unpack('<i',b(nt+27,4))[0]
 assert b(thunk,2)==bytes.fromhex('ff25')
 iat=struct.unpack('<I',b(thunk+2,4))[0];assert bi[iat]==('KERNEL32.DLL','ExitProcess')
 capture=BINARY/'cases/help/observer-capture-01/events.derived-redacted.txt.gz'
 import gzip
 events=gzip.decompress(capture.read_bytes())
 for line in [b'actual-ExitProcess-request status=0',b'owned-success-confirmed exit-event=1 signaled=1 event-status=0 actual-status=0']:
  assert line in events
 proof={'scope':'Root-approved main-only positive noreturn endpoint; no global RET relaxation or CRT acceptance','canonical_graph_sha256':sha(graph),'original':main_endpoint(original,0x4242e0,0x4243c8,0x433f96),'rebuilt':main_endpoint(rebuilt,symbols['main_'],symbols['Spawn_'],ex),'exit_closure':{'original_exit_tail':0x433fc5,'original_NTFini':0x43594b,'original_status_push_and_call':0x43595d,'original_thunk':0x439728,'original_iat':0x4600b0,'rebuilt_exit':ex,'rebuilt__exit':low,'rebuilt_NTFini':nt,'rebuilt_thunk':thunk,'rebuilt_iat':iat,'import':'KERNEL32.DLL!ExitProcess','no_return_model':'Documented nonreturning ExitProcess boundary, independently observed request0/actualexit0; no fabricated cleanup/RETset','original_closure_bytes_sha256':hashlib.sha256(o(0x433f96,52)+o(0x43594b,24)+o(0x439728,6)).hexdigest()},'bindings':{str(p.relative_to(ROOT)):sha(p) for p in [original,ROOT/'toolchains/watcom10.0a/.patched/WLINK.EXE',rebuilt,BINARY/'build/original/wlink.map',capture,BINARY/'src/link.h']},'application_acceptance':False,'remaining':'Fresh masked original/native Spawn frontier required. Strict RET checks remain required for genuinely returning application bodies.'}
 OUT.mkdir(exist_ok=True);(OUT/'main-exit-proof.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof,indent=2))
if __name__=='__main__':main()
