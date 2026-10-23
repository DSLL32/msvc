"""Verify raw GDB stops/entry observations against the captured native ELF."""
import re
import struct
import subprocess
from pathlib import Path


def symbols(binary,data=False):
    rows={}
    text=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
    for line in text.splitlines():
        parts=line.split()
        if len(parts)==4 and (data or parts[2]in ('T','t','W','w')):
            rows[parts[3]]=(int(parts[0],16),int(parts[1],16))
    return rows


def load_bias(binary,maps):
    data=binary.read_bytes();assert data[:6]==b'\x7fELF\x01\x01'
    offset=struct.unpack_from('<I',data,28)[0];size,count=struct.unpack_from('<HH',data,42)
    segments=[]
    for i in range(count):
        kind,file_offset,vaddr,_,filesize,memsize,flags,align=struct.unpack_from('<IIIIIIII',data,offset+i*size)
        if kind==1:segments.append((file_offset,vaddr,filesize,memsize))
    candidates=[]
    for line in maps.splitlines():
        parts=line.split(maxsplit=5)
        if len(parts)!=6 or parts[5]!=str(binary):continue
        start=int(parts[0].split('-')[0],16);file_offset=int(parts[2],16)
        for p,v,_,_ in segments:
            if p&~4095==file_offset:candidates.append(start-(v&~4095))
    assert candidates and len(set(candidates))==1,'native ELF load mapping is ambiguous'
    return candidates[0]


def verify(binary,maps,raw,state,config):
    table=symbols(binary);bias=load_bias(binary,maps)
    name=state['function'];assert name in table,'frontier name has no ELF function'
    start,size=table[name];assert size>0
    registers=re.findall(r'^eip\s+(0x[0-9a-f]+)\b',raw,re.M)
    assert len(registers)==1 and int(registers[0],16)==state['pc'],'raw GDB EIP differs from state'
    assert 'SIGTRAP' in raw,'raw GDB did not observe SIGTRAP'
    assert re.search(r'^'+re.escape(name)+r'(?: \+ [0-9]+)? in section \.text of ',raw,re.M),'raw GDB symbol differs'
    decoded=subprocess.check_output(['objdump','-d','-Mintel','--disassemble='+name,str(binary)],text=True)
    traps={int(a,16)for a in re.findall(r'^\s*([0-9a-f]+):\s+cc\s+int3\s*$',decoded,re.M)}
    static_pc=state['pc']-bias
    assert any(start<=at<start+size and static_pc in (at,at+1)for at in traps),'native PC is not the named ELF INT3'
    entries=re.findall(r'^C2-SOURCE-ENTRY (\S+) (0x[0-9a-f]+)$',raw,re.M)
    indexed={r['name']:r.get('native',r['name'].lstrip('_'))for r in config['driver']+config['support']if r['name']in config['accepted']and r.get('kind')!='import'}
    for logical,address in entries:
        assert logical in indexed and indexed[logical]in table
        entry,size=table[indexed[logical]];actual=int(address,16)-bias
        assert size>0 and entry<=actual<entry+size,'raw entry is outside its named ELF function'
    assert [n for n,_ in entries]==state['trace'],'raw GDB entry order differs from derived state'
    return {'load_bias':hex(bias),'frontier_symbol':name,'symbol_start':hex(start),'symbol_size':size,'static_pc':hex(static_pc),'int3_sites':[hex(n)for n in sorted(traps)]}

def verify_completion(binary,maps,raw,state,config,status=1):
    import json
    assert set(state)=={'completed','exit_status','trace','arguments','MSC_CMD_FLAGS'},'completion contains an invented frontier'
    assert state['completed']is True and state['exit_status']==status
    assert re.findall(r'^C2-PROCESS-EXIT (-?[0-9]+)$',raw,re.M)==[str(status)],'native raw process exit is absent/different'
    observed=re.findall(r'^C2-PROCESS-INPUT (.+)$',raw,re.M)
    assert len(observed)==1 and json.loads(observed[0])=={'arguments':state['arguments'],'MSC_CMD_FLAGS':state['MSC_CMD_FLAGS']},'actual native process input differs'
    assert 'SIGTRAP'not in raw and not re.findall(r'^eip\s+',raw,re.M),'trap cannot be completion'
    table=symbols(binary);bias=load_bias(binary,maps)
    entries=re.findall(r'^C2-SOURCE-ENTRY (\S+) (0x[0-9a-f]+)$',raw,re.M)
    indexed={r['name']:r.get('native',r['name'].lstrip('_'))for r in config['driver']if r['name']in config['accepted']}
    for logical,address in entries:
        assert logical in indexed and indexed[logical]in table
        start,size=table[indexed[logical]]
        assert size>0 and start<=int(address,16)-bias<start+size,'completion entry is outside named ELF function'
    assert entries and [name for name,_ in entries]==state['trace'],'empty/different completion trace'
    return {'load_bias':hex(bias),'completed':True,'exit_status':status,'source_entries':len(entries)}
