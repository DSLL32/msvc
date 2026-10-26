#!/usr/bin/env python3
"""Exact NewSym/NewLeaf normal-RET/nonlocal-error residual, not a CFG engine.
Shared graph/ownership/extent checks retain every App edge. CSuicide and its
failure bodies stay pending/masked; this proves no native longjmp behavior.
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys

project=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
repo=project.parents[2]
profile=json.loads((project/'cf-profile.json').read_text())['target']
container=(repo/profile['path']).read_bytes()
assert hashlib.sha256(container).hexdigest()==profile['sha256']
h=profile['header']; original=container[h['image_file_offset']:h['image_file_offset']+h['initialized_size']]
assert hashlib.sha256(original).hexdigest()==profile['initialized_sha256']

# These exact finite role bytes were source-correlated and reviewed. They
# never imply ownership/admission or returning ABI for a nonlocal endpoint.
EXPECTED={
    (0x2b76f,0x2b7cb):'651dff1c94478e4e0127974592764f38935bd6782b6c9aed5a4fe3c805b57259', # NewLeaf
    (0x643a,0x645d):'4da60e36d1c71900051ba432c497831f26b6d74434bb2b7faff737bfd8df1ac3', # CSuicide
    (0x2c195,0x2c1f2):'2beb2c5796888d6f4318ed514289634bf019835cc8c63c02ba9a4fa305f6809b', # longjmp
    (0x1ad01,0x1ad06):'f92db9c054b048e3b2fa95448c157b097b9c0a41e0b1f3803152b60f199ba8c6', # MyExit
    (0x2ff2c,0x2ff44):'6ac3db268fe8f891b3e2fe0baa303a7c3429566021071e0dc970e760ab5bbeb3', # exit
    (0x2ff44,0x2ff5a):'379f368bb79ab853eed734b695379164c2fc10ea14cebfb9bd84d9f365f50d7e', # _exit
    (0x2feb8,0x2fed2):'d82357b397f4e9b188dc800c79a0002766a31fbd9849e90a3852867605362a97', # OSI __exit
}
for (start,end),digest in EXPECTED.items():
    assert hashlib.sha256(original[start:end]).hexdigest()==digest

def jump(code,at,target):
    assert code[at]==0xe9 and at+5+struct.unpack_from('<i',code,at+1)[0]==target

# NewLeaf's limit branch calls the same nonlocal endpoint after CErr1.
assert original[0x2b7c7:0x2b7cb]==bytes.fromhex('c95a5bc3')
assert original[0x2b7aa:0x2b7af]==bytes.fromhex('b879040000')
for site,target in [(0x2b7af,0x60c0),(0x2b7b4,0x643a)]:
    assert original[site]==0xe8 and site+5+struct.unpack_from('<i',original,site+1)[0]==target
assert original[0x18194:0x181a6]==bytes.fromhex('c366ff053ae1070066833d3ae10700ff72ee')
jump(original,0x181b0,0x643a)
jump(original,0x644e,0x2c195)
jump(original,0x6458,0x1ad01)
jump(original,0x1ad01,0x2ff2c)
jump(original,0x2ff55,0x2feb8)
# longjmp switches SS/ESP and pushes the SAVED continuation; its final RET
# resumes setjmp rather than returning through NewSym's caller frame.
assert original[0x2c1a6:0x2c1af]==bytes.fromhex('8e502a8b601cff7018')
assert original[0x2c1f0:0x2c1f2]==bytes.fromhex('58c3')
# OSI process exit instead restores the CF entry stack and far-returns to
# the loader. Unknown finalizer/hook behavior is not qualified here.
assert original[0x2fe06:0x2fe0c]==bytes.fromhex('892512900700')
assert original[0x2fecb:0x2fed2]==bytes.fromhex('8b2512900700cb')

rebuilt=project/'build/WCC386.EXE'; data=rebuilt.read_bytes()
pe=struct.unpack_from('<I',data,0x3c)[0]
count=struct.unpack_from('<H',data,pe+6)[0]
optional=struct.unpack_from('<H',data,pe+20)[0]
base=struct.unpack_from('<I',data,pe+52)[0]
sections=[]
for k in range(count):
    p=pe+24+optional+40*k
    va,size,offset=struct.unpack_from('<III',data,p+12)
    flags=struct.unpack_from('<I',data,p+36)[0]
    sections.append((base+va,size,offset,flags))
code_base=next(a for a,n,p,flags in sections if flags&0x20)
def read_va(address,size):
    for a,n,p,flags in sections:
        if a<=address and address+size<=a+n:
            return data[p+address-a:p+address-a+size]
    raise AssertionError('unmapped ABI bytes')

symbols={name:code_base+int(offset,16) for offset,name in
    re.findall(r'^0001:([0-9a-fA-F]{8})[ *+]\s+(\S+)',(project/'build/wcc386.map').read_text(),re.M)}
for name,frame,epilogue in [
    ('NewSym',0,'5d5f5e5a595bc3'),
    ('NewLeaf',32,'c95f5e5a595bc3'),
]:
    start=symbols[name+'_'];end=min(a for a in symbols.values() if a>start)
    text=subprocess.check_output(['objdump','-D','-Mintel','--no-show-raw-insn',
        '--start-address='+hex(start),'--stop-address='+hex(end),str(rebuilt)],text=True)
    code=[(int(a,16),m,args.strip()) for a,m,args in re.findall(r'^\s*([0-9a-f]+):\s+(\S+)\s*(.*?)$',text,re.M)]
    returns=[(a,args) for a,m,args in code if m.startswith('ret')]
    assert len(returns)==1 and returns[0][1]=='' and read_va(returns[0][0],1)==b'\xc3'
    # These exact period -od prologues save six registers. NewLeaf has a
    #32-byte local frame restored by LEAVE; NewSym's frame is empty.
    assert read_va(start,14)==bytes.fromhex('53515256575589e581ec')+struct.pack('<I',frame)
    assert read_va(returns[0][0]-6,7)==bytes.fromhex(epilogue)
    tails=[(a,int(args.split()[0],16)) for a,m,args in code if m=='jmp' and not start<=int(args.split()[0],16)<end]
    assert len(tails)==1 and tails[0][1]==symbols['CSuicide_']
for name,target in [('CCusage','MyExit_')]:
    start=symbols[name+'_'];end=min(a for a in symbols.values() if a>start)
    text=subprocess.check_output(['objdump','-D','-Mintel','--no-show-raw-insn',
        '--start-address='+hex(start),'--stop-address='+hex(end),str(rebuilt)],text=True)
    code=[(int(a,16),m,args.strip()) for a,m,args in re.findall(r'^\s*([0-9a-f]+):\s+(\S+)\s*(.*?)$',text,re.M)]
    assert not any(m.startswith('ret') for a,m,args in code)
    tails=[(a,int(args.split()[0],16)) for a,m,args in code if m=='jmp' and not start<=int(args.split()[0],16)<end]
    assert len(tails)==1 and tails[0][1]==symbols[target]
# The actual -ox definition forwards EAX untouched in a balanced10-byte
# CALLexit wrapper. Its syntactic RET0 is unreachable because real exit
# takes the exact nonlocal closure above: original localRET[] vs rebuilt[0].
start=symbols['MyExit_'];end=min(a for a in symbols.values() if a>start)
assert end-start==10
assert read_va(start,3)==bytes.fromhex('5589e5')
assert read_va(start+3,1)==b'\xe8'
assert start+8+struct.unpack('<i',read_va(start+4,4))[0]==symbols['exit_']
assert read_va(start+8,2)==bytes.fromhex('5dc3')
assert original[0x1a0d3:0x1a0d8]==bytes.fromhex('b801000000')
jump(original,0x1a0d8,0x1ad01)
assert '#pragma aux MyExit aborts;' in (project/'src/command.h').read_text()
assert '#define WCC_CSUICIDE_ABORTS' in (project/'src/symbols.c').read_text()
assert '#define WCC_CSUICIDE_ABORTS' in (project/'src/quads.c').read_text()
assert '#pragma aux CSuicide aborts;'  in (project/'src/compiler.h').read_text()
print('PASS NewSym/NewLeaf RET0; CCusage noreturn; MyExit localRET[]/[0], sole exit call; nonlocal closure retained')
