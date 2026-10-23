#!/usr/bin/env python3
"""Explicit stopped original-loader capture; never imported by native fixtures.

Requires a separately reviewed, hash-bound plan. Default stops before launch;
explicit entry_handoff executes only known loader instructions and the installed
CF entry INT3. No original CF instruction or application credit is admitted.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[4]
SCRATCH=Path('/root/.ai-tmp/msvc2')
PREFIX=Path('/root/.ai-tmp/vc1.0-capture/wine')
# Avoid this folder's analysis-only inspect.py shadowing stdlib inspect.
sys.path[:]=[str(ROOT/'tooling/watcom-capture')]+[path for path in sys.path
    if Path(path or '.').resolve()!=Path(__file__).resolve().parent]
from broker import Broker
from cf_identity import Image
from ipc import Client,ProtocolError
from wcc_handoff import enter

STUB=ROOT/'toolchains/watcom10.0a/binnt/WCC386.EXE'
PAYLOAD=ROOT/'toolchains/watcom10.0a/binb/WCC386.EXE'
STUB_SHA='8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d'
PAYLOAD_SHA='c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b'
HEADER=(0x4643,0x2200,0x8209c,0x7c448,0x91000,0x2fe00)


def sha(data):return hashlib.sha256(data).hexdigest()
def file_sha(path):return sha(Path(path).read_bytes())
def require(condition,message):
    if not condition:raise ProtocolError(message)
def win(path):
    path=Path(path).resolve()
    for root,drive in ((ROOT,'R:'),(SCRATCH,'S:')):
        try:return drive+'\\'+str(path.relative_to(root)).replace('/','\\')
        except ValueError:pass
    raise ProtocolError('path outside shared drives')
def private(path,data):
    descriptor=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
    os.fchmod(descriptor,0o600)
    with os.fdopen(descriptor,'wb') as stream:stream.write(data)
def read_all(broker,address,size):
    require(0<size<=0x100000 and address+size<=0x100000000,'bounded stopped-memory read')
    return b''.join(broker.read(address+offset,min(512,size-offset))
                    for offset in range(0,size,512))
def terminated(broker,address,double=False):
    require(address>0,'null input pointer')
    result=bytearray();previous=1
    while len(result)<65536:
        current=address+len(result);region=broker.region(current)
        count=min(512,65536-len(result),region['base']+region['size']-current)
        require(region['state']==0x1000 and not region['protect']&0x101,
                'input string is not committed/readable')
        for value in broker.read(current,count):
            result.append(value)
            if not value and (not double or not previous):return bytes(result)
            previous=value
    raise ProtocolError('unterminated bounded original input')


def loader_bytes(broker,base):
    """Thin PE loaded-byte identity, not graph discovery or classification."""
    source=STUB.read_bytes();require(sha(source)==STUB_SHA,'loader file changed')
    pe=struct.unpack_from('<I',source,0x3c)[0]
    require(source[pe:pe+4]==b'PE\0\0','PE identity')
    optional=pe+24;count=struct.unpack_from('<H',source,pe+6)[0]
    size=struct.unpack_from('<H',source,pe+20)[0]
    preferred=struct.unpack_from('<I',source,optional+28)[0]
    sections=[]
    for index in range(count):
        row=optional+size+40*index
        name=source[row:row+8].rstrip(b'\0').decode('ascii')
        virtual,rva,raw_size,raw=struct.unpack_from('<4I',source,row+8)
        sections.append((name,virtual,rva,raw_size,raw))
    def raw_at(rva,length):
        for name,virtual,start,raw_size,raw in sections:
            if start<=rva and rva+length<=start+raw_size:return raw+rva-start
        raise ProtocolError('PE raw address outside pinned sections')
    reloc_rva,reloc_size=struct.unpack_from('<II',source,optional+96+5*8)
    cursor=raw_at(reloc_rva,reloc_size);end=cursor+reloc_size;fixups=[]
    while cursor<end:
        page,length=struct.unpack_from('<II',source,cursor)
        require(8<=length<=end-cursor and not length%2,'PE relocation block bounds')
        for offset in range(cursor+8,cursor+length,2):
            word=struct.unpack_from('<H',source,offset)[0];kind=word>>12
            require(kind in (0,3),'unexpected loader PE relocation type')
            if kind:fixups.append(page+(word&4095))
        cursor+=length
    require(cursor==end and len(fixups)==137,'loader PE relocation count')
    proof=[]
    for name,virtual,rva,raw_size,raw in sections:
        if name not in ('BEGTEXT','DGROUP'):continue
        expected=bytearray(source[raw:raw+raw_size]);selected=[]
        for address in fixups:
            if rva<=address<rva+raw_size:
                require(address+4<=rva+raw_size,'PE fixup crosses owned section')
                value=struct.unpack_from('<I',expected,address-rva)[0]
                struct.pack_into('<I',expected,address-rva,(value+base-preferred)&0xffffffff)
                selected.append(address)
        actual=read_all(broker,base+rva,raw_size)
        require(actual==expected,'actual loaded loader '+name+' byte mismatch')
        proof.append({'name':name,'rva':rva,'bytes':raw_size,'fixups':len(selected),
                      'actual_sha256':sha(actual),'expected_sha256':sha(expected)})
    require([row['name'] for row in proof]==['BEGTEXT','DGROUP'],'loader section identity')
    return {'preferred_base':preferred,'observed_base':base,'sections':proof,
            'total_pe_fixups':len(fixups),'imports_excluded':'OS-resolved .idata'}


def capture(plan_path):
    plan=json.loads(plan_path.read_text());work=Path(plan['work']).resolve()
    require(work.is_relative_to(SCRATCH),'capture directory outside shared scratch')
    require(plan['maximum_helper_launches']==1 and plan['raw_arguments']=='?', 'unexpected capture scope')
    require(file_sha(STUB)==STUB_SHA and file_sha(PAYLOAD)==PAYLOAD_SHA,'original identities')
    require((PREFIX/'dosdevices/r:').resolve()==ROOT and
            (PREFIX/'dosdevices/s:').resolve()==SCRATCH,'shared drive mapping changed')
    for path,digest in plan['bindings'].items():require(file_sha(path)==digest,'binding changed: '+path)
    require(str(Path(__file__).resolve()) in plan['bindings'],'runner is not source bound')
    image=Image.parse(PAYLOAD.read_bytes(),PAYLOAD_SHA,HEADER)
    require(image.groups==8 and len(image.fixups)==11801,'WCC relocation schema changed')
    work.mkdir();(work/'cwd').mkdir();private(work/'stdin.bin',b'')
    env=os.environ.copy();env.update(plan['environment_updates'])
    for name in plan['environment_remove']:env.pop(name,None)
    require(env['WINEPREFIX']==str(PREFIX),'nonshared Wine prefix')
    private(work/'host-environment.json',(json.dumps(dict(sorted(env.items())),indent=2)+'\n').encode())
    receipt={'scope':plan['scope'],'plan_sha256':file_sha(plan_path),'bindings':plan['bindings'],
             'accepted_functions':0,'cf_instructions_executed':0,'timing':{},
             'private_environment_sha256':file_sha(work/'host-environment.json')}
    start=time.monotonic();child=None;broker=None;channel=None;out=err=b'';forced=False
    def save():
        receipt['elapsed_seconds']=time.monotonic()-start
        (work/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    try:
        with Client(work/'channel.bin',seconds=60,heartbeat_seconds=.01) as channel:
            command=['/usr/bin/wine',win(plan['broker']),win(channel.path),win(STUB),'?',win(work/'cwd'),
                     win(work/'stdin.bin'),win(work/'stdout.bin'),win(work/'stderr.bin'),
                     win(work/'events.bin'),'10010','e9','55000','2000']
            receipt['command']=command;save()
            child=subprocess.Popen(command,cwd=work,env=env,stdin=subprocess.DEVNULL,
                                   stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            receipt['owned_launcher_pid']=child.pid;broker=Broker(channel)
            event,entry=broker.entry(0x10010,0xe9);base=event['base']
            receipt['entry']={'event':event,'context':entry}
            receipt['loaded_loader']=loader_bytes(broker,base)
            _,receipt['segments']=broker.flat_selectors(entry)
            receipt['timing']['setup_entry_identity_seconds']=time.monotonic()-start
            tick=time.monotonic();index,stop,actual=broker.wcc_prefix()
            require(stop['pid']==event['pid'] and stop['tid']==event['tid'],'foreign launch stop')
            require(not actual['eflags']&0x100,'TF active at launch')
            require(broker.read(base+0x10763,5)==b'\xcc\xad\xf8\xff\xff','launch trap changed')
            receipt['launch']={'event':stop,'context':actual,'trap_index':index}
            receipt['timing']['fixed_loader_prefix_seconds']=time.monotonic()-tick
            slots=struct.unpack('<4I',broker.read(base+0x30000,16))
            entry_address,allocation,base_address,break_flag=slots
            require(allocation==base_address and entry_address==allocation+image.entry_offset,
                    'actual CF allocation/entry globals disagree')
            require(0<allocation and allocation+image.allocation_size<=0x100000000,'allocation overflow')
            require(actual['esi']==entry_address and actual['ebx']==allocation and
                    actual['edx']==base+0x10788 and actual['edi']==actual['esp']+0x104,
                    'actual loader launch register contract mismatch')
            params=struct.unpack('<5I',broker.read(actual['edi'],20))
            require(params[3]==base+0x3000c and params[4]==0,'actual five-field parameter contract')
            name=terminated(broker,params[0]);tail=terminated(broker,params[1])
            require(name.decode('ascii').lower()==win(PAYLOAD).lower()+'\0','actual program-name input differs')
            require(tail==b'?\0','actual raw command tail differs')
            environment=terminated(broker,params[2],double=True)
            private(work/'payload-environment.bin',environment)
            receipt['launch_inputs']={'slots':list(slots),'parameters':list(params),
                    'program_name_hex':name.hex(),'command_tail_hex':tail.hex(),
                    'environment_sha256':sha(environment),'environment_bytes':len(environment),
                    'environment_private_path':str(work/'payload-environment.bin')}
            regions=[];cursor=allocation
            while cursor<allocation+image.allocation_size:
                region=broker.region(cursor);regions.append(region)
                require(0<region['allocation_base']<=allocation and region['state']==0x1000 and
                        region['protect'] in (2,4,8,0x20,0x40,0x80),'CF allocation not committed/readable')
                cursor=min(region['base']+region['size'],allocation+image.allocation_size)
            receipt['allocation_regions']=regions
            tick=time.monotonic();snapshot=read_all(broker,allocation,image.loaded_size)
            private(work/'relocated-initialized.bin',snapshot)
            receipt['timing']['initialized_snapshot_seconds']=time.monotonic()-tick
            tick=time.monotonic();receipt['cf_identity']=image.compare(snapshot,allocation)
            receipt['timing']['full_relocation_comparison_seconds']=time.monotonic()-tick
            require(broker.context()==actual and broker.read(base+0x30000,16)==struct.pack('<4I',*slots) and
                    broker.read(base+0x10763,5)==b'\xcc\xad\xf8\xff\xff','stopped state changed during inspection')
            if plan.get('entry_handoff',False):
                tick=time.monotonic()
                receipt['entry_handoff']=enter(broker,event,actual,allocation,entry_address,snapshot)
                receipt['timing']['six_loader_instructions_and_actual_entry_int3_seconds']=time.monotonic()-tick
            tick=time.monotonic();broker.stop();out,err=child.communicate(timeout=8)
            receipt['timing']['owned_teardown_seconds']=time.monotonic()-tick
            require(child.returncode==0 and not out and not err,'broker failed or unexpected launcher streams')
            log=(work/'events.bin').read_bytes()
            require(b'owned-teardown exit-event=1 signaled=1' in log and
                    b'cf-instructions=0' in log,'missing actual owned stop/termination record')
            require((work/'stdout.bin').read_bytes()==b'' and (work/'stderr.bin').read_bytes()==b'',
                    'payload produced output before launch')
            receipt['passed']=True
            receipt['events']=broker.events;receipt['channel']=channel.records
    except BaseException as error:
        receipt['failure']=repr(error)
        raise
    finally:
        if child is not None:
            try:out,err=child.communicate(timeout=6)
            except subprocess.TimeoutExpired:
                child.kill();out,err=child.communicate(timeout=3);forced=True
            private(work/'launcher.stdout',out);private(work/'launcher.stderr',err)
            receipt['broker_status']=child.returncode;receipt['forced_outer_stop_failed_proof']=forced
            receipt['actual_broker_terminated']=child.poll() is not None
        if broker is not None:receipt['events']=broker.events
        if channel is not None:receipt['channel']=channel.records
        for name in ('events.bin','stdout.bin','stderr.bin','relocated-initialized.bin'):
            path=work/name
            if path.exists():receipt.setdefault('files',{})[name]={'bytes':path.stat().st_size,'sha256':file_sha(path)}
        save()
    print(json.dumps({'path':str(work),'passed':receipt['passed'],'timing':receipt['timing']}))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan',type=Path,required=True)
    capture(parser.parse_args().plan.resolve())
