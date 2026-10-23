#!/usr/bin/env python3
"""Verify the scoped hold in the native object, with no controller enforcing it."""
import argparse,json,re,resource,shutil,signal,struct,subprocess,time
from pathlib import Path
from protocol import HERE,sha
p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args()
assert not a.output.exists();a.output.mkdir(parents=True);started=time.perf_counter()
for source,name in [('build/native/class-boundary-host','probe.elf'),('build/native/rcdll-host','production.elf'),('build/native/characters.o','characters.o'),('build/native-class-boundary-source.json','compiled-source.json'),('build/native-source.json','production-compiled-source.json')]:shutil.copy2(HERE/source,a.output/name)
def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
probe=a.output/'probe.elf';runs=[]
for argument,status,output in [('105',0,b'16\n'),('256',-signal.SIGTRAP,b'')]:
    begin=time.perf_counter();r=subprocess.run([str(probe),argument],capture_output=True,preexec_fn=no_core,timeout=5)
    assert r.returncode==status and r.stdout==output and not r.stderr
    runs.append(dict(argument=argument,status=r.returncode,stdout=r.stdout.hex(),stderr=r.stderr.hex(),seconds=time.perf_counter()-begin))
def function(path):
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(path)],text=True)
    rows=re.findall(r'^([0-9a-f]+) ([0-9a-f]+) T rc_character_class$',symbols,re.M);assert len(rows)==1
    address,size=[int(v,16)for v in rows[0]];data=path.read_bytes();phoff=struct.unpack_from('<I',data,28)[0];phbytes,count=struct.unpack_from('<HH',data,42)
    for i in range(count):
        kind,raw,start,_,length=struct.unpack_from('<IIIII',data,phoff+phbytes*i)
        if kind==1 and start<=address and address+size<=start+length:return address,data[raw+address-start:raw+address-start+size]
    raise AssertionError('function not ELF-backed')
address,body=function(probe);production_address,production_body=function(a.output/'production.elf')
data=(a.output/'characters.o').read_bytes();shoff=struct.unpack_from('<I',data,32)[0];shbytes,count=struct.unpack_from('<HH',data,46)
sections=[struct.unpack_from('<10I',data,shoff+shbytes*i)for i in range(count)];object_bodies=[]
for section in sections:
    if section[1]!=2:continue
    strings=sections[section[6]];names=data[strings[4]:strings[4]+strings[5]]
    for at in range(section[4],section[4]+section[5],section[9]):
        name,value,size,kind,_,owner=struct.unpack_from('<IIIBBH',data,at)
        if names[name:names.index(b'\0',name)]==b'rc_character_class':
            assert kind&15==2 and owner>0;source=sections[owner]
            object_bodies.append(data[source[4]+value:source[4]+value+size])
assert len(object_bodies)==1 and len(object_bodies[0])==len(body)==len(production_body)

# Decode the real instruction listing: a raw CC byte could also occur in data.
listing=subprocess.check_output(['objdump','-d','-Mintel','--start-address='+str(address),'--stop-address='+str(address+len(body)),str(probe)],text=True)
traps=re.findall(r'^\s*([0-9a-f]+):\s+cc\s+int3\s*$',listing,re.M);assert len(traps)==1
offset=int(traps[0],16)-address;assert body[offset]==production_body[offset]==0xcc
assert body[offset-8:offset+1]==production_body[offset-8:offset+1]==object_bodies[0][offset-8:offset+1]
command=['gdb','--batch','-q',str(probe),'-ex','handle SIGTRAP stop print nopass','-ex','run 256','-ex','printf "CLASS_HOLD character=%u pc=%p function=%p\\n", character, $pc, rc_character_class','-ex','x/1bx $pc-1','-ex','bt 2']
begin=time.perf_counter();r=subprocess.run(command,capture_output=True,text=True,timeout=5);assert r.returncode==0
(a.output/'gdb.stdout').write_text(r.stdout);(a.output/'gdb.stderr').write_text(r.stderr);(a.output/'probe-disassembly.txt').write_text(listing)
assert 'Program received signal SIGTRAP'in r.stdout
rows=re.findall(r'CLASS_HOLD character=(\d+) pc=(0x[0-9a-f]+) function=(0x[0-9a-f]+)',r.stdout);assert len(rows)==1
character,pc,function_address=[int(v,0)for v in rows[0]];assert character==256 and pc-1-function_address==offset
assert re.search(r'0x[0-9a-f]+\s+<rc_character_class\+\d+>:\s+0xcc',r.stdout)
result=dict(status='pass',scope='Native source INT3 stops class256 before the unavailable boundary-word read. Authentic Win32 getter is unchanged; no alias/default value is claimed.',plain_runs=runs,gdb=dict(status=0,target_exit=None,signal='SIGTRAP',character=256,guard_offset=offset,seconds=time.perf_counter()-begin),production_guard=dict(function=hex(production_address),offset=offset,same_compiled_object=True,guard_bytes=body[offset-8:offset+1].hex()),files={f.name:sha(f)for f in a.output.iterdir()if f.is_file()},seconds=time.perf_counter()-started,checker_sha256=sha(Path(__file__)))
(a.output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
