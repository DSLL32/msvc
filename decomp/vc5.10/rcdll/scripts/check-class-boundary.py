#!/usr/bin/env python3
"""Verify the scoped hold in the native object, with no controller enforcing it."""
import argparse,json,re,resource,shutil,signal,struct,subprocess,time
from pathlib import Path
from protocol import HERE,sha
p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);p.add_argument('--kind',choices=['class','directive','descriptor'],default='class');a=p.parse_args()
spec={'class':dict(probe='class-boundary-host',object='characters.o',compiled='native-class-boundary-source.json',function='rc_character_class',good='105',result=b'16\n',bad='256',value=256,expression='character'), 'directive':dict(probe='directive-boundary-host',object='directive-names.o',compiled='native-directive-boundary-source.json',function='rc_directive_code',good='line',result=b'10\n',bad='unicode',value=257,expression='word[0]'), 'descriptor':dict(probe='descriptor-boundary-host',object='lexer.o',compiled='native-descriptor-boundary-source.json',function='rc_next_token',good='7',result=b'94 7\n',bad='128',value=128,expression='token',traps=2)}[a.kind]
assert not a.output.exists();a.output.mkdir(parents=True);started=time.perf_counter()
for source,name in [('build/native/'+spec['probe'],'probe.elf'),('build/native/rcdll-host','production.elf'),('build/native/'+spec['object'],'target.o'),('build/'+spec['compiled'],'compiled-source.json'),('build/native-source.json','production-compiled-source.json')]:shutil.copy2(HERE/source,a.output/name)
def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
probe=a.output/'probe.elf';runs=[]
for argument,status,output in [(spec['good'],0,spec['result']),(spec['bad'],-signal.SIGTRAP,b'')]:
    begin=time.perf_counter();r=subprocess.run([str(probe),argument],capture_output=True,preexec_fn=no_core,timeout=5)
    assert r.returncode==status and r.stdout==output and not r.stderr
    runs.append(dict(argument=argument,status=r.returncode,stdout=r.stdout.hex(),stderr=r.stderr.hex(),seconds=time.perf_counter()-begin))
def function(path):
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(path)],text=True)
    rows=re.findall(r'^([0-9a-f]+) ([0-9a-f]+) T '+re.escape(spec['function'])+'$',symbols,re.M);assert len(rows)==1
    address,size=[int(v,16)for v in rows[0]];data=path.read_bytes();phoff=struct.unpack_from('<I',data,28)[0];phbytes,count=struct.unpack_from('<HH',data,42)
    for i in range(count):
        kind,raw,start,_,length=struct.unpack_from('<IIIII',data,phoff+phbytes*i)
        if kind==1 and start<=address and address+size<=start+length:return address,data[raw+address-start:raw+address-start+size]
    raise AssertionError('function not ELF-backed')
address,body=function(probe);production_address,production_body=function(a.output/'production.elf')
data=(a.output/'target.o').read_bytes();shoff=struct.unpack_from('<I',data,32)[0];shbytes,count=struct.unpack_from('<HH',data,46)
sections=[struct.unpack_from('<10I',data,shoff+shbytes*i)for i in range(count)];object_bodies=[]
for section in sections:
    if section[1]!=2:continue
    strings=sections[section[6]];names=data[strings[4]:strings[4]+strings[5]]
    for at in range(section[4],section[4]+section[5],section[9]):
        name,value,size,kind,_,owner=struct.unpack_from('<IIIBBH',data,at)
        if names[name:names.index(b'\0',name)]==spec['function'].encode():
            assert kind&15==2 and owner>0;source=sections[owner]
            object_bodies.append(data[source[4]+value:source[4]+value+size])
assert len(object_bodies)==1 and len(object_bodies[0])==len(body)==len(production_body)

# Decode the real instruction listing: a raw CC byte could also occur in data.
listing=subprocess.check_output(['objdump','-d','-Mintel','--start-address='+str(address),'--stop-address='+str(address+len(body)),str(probe)],text=True)
traps=re.findall(r'^\s*([0-9a-f]+):\s+cc\s+int3\s*$',listing,re.M);assert len(traps)==spec.get('traps',1)
command=['gdb','--batch','-q',str(probe),'-ex','handle SIGTRAP stop print nopass','-ex','run '+spec['bad'],'-ex','printf "NATIVE_HOLD value=%u pc=%p function=%p\\n", '+spec['expression']+', $pc, '+spec['function'],'-ex','x/1bx $pc-1','-ex','bt 2']
begin=time.perf_counter();r=subprocess.run(command,capture_output=True,text=True,timeout=5);assert r.returncode==0
(a.output/'gdb.stdout').write_text(r.stdout);(a.output/'gdb.stderr').write_text(r.stderr);(a.output/'probe-disassembly.txt').write_text(listing)
assert 'Program received signal SIGTRAP'in r.stdout
rows=re.findall(r'NATIVE_HOLD value=(\d+) pc=(0x[0-9a-f]+) function=(0x[0-9a-f]+)',r.stdout);assert len(rows)==1
character,pc,function_address=[int(v,0)for v in rows[0]];offset=pc-1-function_address
assert character==spec['value']and address+offset in [int(v,16)for v in traps]
assert body[offset]==production_body[offset]==0xcc
assert body[offset-8:offset+1]==production_body[offset-8:offset+1]==object_bodies[0][offset-8:offset+1]
assert re.search(r'0x[0-9a-f]+\s+<'+re.escape(spec['function'])+r'\+\d+>:\s+0xcc',r.stdout)
result=dict(status='pass',kind=a.kind,function=spec['function'],scope='Native source INT3 stops the declared unsupported input before its unavailable data read. Authentic Win32 body is unchanged; unavailable alias/data remains unclaimed.',plain_runs=runs,gdb=dict(status=0,target_exit=None,signal='SIGTRAP',argument_value=spec['value'],guard_offset=offset,seconds=time.perf_counter()-begin),production_guard=dict(function=hex(production_address),offset=offset,same_compiled_object=True,guard_bytes=body[offset-8:offset+1].hex()),files={f.name:sha(f)for f in a.output.iterdir()if f.is_file()},seconds=time.perf_counter()-started,checker_sha256=sha(Path(__file__)))
if a.kind=='descriptor':result['controlled_dependency']=dict(name='rc_read_number',original_stub='localized only in auxiliary stubs object',source='scripts/descriptor-boundary-host.c',known_result=7,unknown_result=128,scope='Out-of-graph negative setup only, not number-parser acceptance or a target output claim.')
(a.output/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
