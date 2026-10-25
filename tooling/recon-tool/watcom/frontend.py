"""Private explicit-original backend; native replay never imports this package.

Only the hash-bound fixed WCC loader handoff is currently exposed. The actual
CF entry INT3 is retained: no CF body instruction or application oracle runs.
"""
import hashlib,json,os,struct,subprocess,sys,time
from pathlib import Path
from broker import Broker
from cf_identity import Image
from ipc import Client,ProtocolError
from wcc_handoff import enter

ROOT=SCRATCH=STUB=None
STUB_SHA=None
def sha(data):return hashlib.sha256(data).hexdigest()
def file_sha(path):return sha(Path(path).read_bytes())
def require(condition,message):
    if not condition:raise ProtocolError(message)
def win(path):
    path=Path(path).resolve()
    for root,drive in ((ROOT,'R:'),(SCRATCH,'S:'),(Path('/tmp/recon'),'T:')):
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


def verify_overlay(block,overlay):
    require(isinstance(block,bytes) and 2<=len(block)<=32767 and block.endswith(b'\0\0'),
            'invalid reached payload environment block')
    entries=block.decode('ascii').split('\0')[:-2] if block!=b'\0\0' else []
    actual={}
    for entry in entries:
        equal=entry.find('=',1 if entry.startswith('=') else 0)
        require(equal>0,'invalid reached environment entry')
        name,value=entry[:equal].upper(),entry[equal+1:]
        require(name not in actual,'duplicate reached environment name')
        actual[name]=value
    for name,value in overlay.items():
        folded=name.upper()
        require((folded not in actual) if value is None else actual.get(folded)==value,
                'requested target overlay differs at '+name)
    return {'verified':True,'names':sorted(overlay,key=str.upper),'actual_entries':len(actual)}

def validate_launch_command(argv):
    require(len(argv)==15 and all(isinstance(item,str) and '\0' not in item and len(item)<=4096 for item in argv),
            'invalid CF subprocess argument shape')
    require(Path(argv[0]).is_absolute(),'Wine executable must be an absolute host path')
    return argv

def execute(request):
    global ROOT,SCRATCH,STUB,STUB_SHA
    ROOT=Path(request['repo']);SCRATCH=Path.home()/'.ai-tmp/msvc2'
    STUB=Path(request['loader']);STUB_SHA=request['profile']['loader']['sha256']
    target=Path(request['target']);profile=request['profile'];meta=profile['target'];work=Path(request['root'])
    h=meta['header'];header=(0x4643,h['image_file_offset'],h['initialized_size'],h['relocation_offset'],h['allocation_size'],h['entry_offset'])
    image=Image.parse(target.read_bytes(),meta['sha256'],header)
    require(image.header_offset==h['header_file_offset'] and image.groups==meta['relocation_groups'] and
            len(image.fixups)==meta['relocation_count'] and sha(image.initialized)==meta['initialized_sha256'] and
            sha(image.initialized[image.relocation_offset:])==meta['relocation_sha256'],'CF metadata mismatch')
    require(file_sha(STUB)==STUB_SHA,'loader changed before launch')
    start=time.monotonic();receipt={'backend':'watcom-cf','mode':'entry-stop','oracle_written':False,
        'original_cf_instructions_executed':0,'first_entry_order':[], 'floating_save':None,
        'sources':request['sources'],'profile_sha256':request['profile_sha256'],'timing':{},'events':[]}
    child=None;b=None;channel=None;out=err=b'';forced=False
    try:
        with Client(work/'cf-channel',seconds=request['seconds'],heartbeat_seconds=.01) as channel:
            argv=[request['wine'],request['helper'],win(channel.path),win(STUB),request['arguments'],
                  win(Path(request['cwd'])),request['stdin'],win(work/'original-stdout'),
                  win(work/'original-stderr'),win(work/'cf-events'),'10010','e9',
                  str(request['timeout_ms']),'1000',win(Path(request['environment']))]
            validate_launch_command(argv)
            receipt['command']=argv
            child=subprocess.Popen(argv,cwd=request['cwd'],stdin=subprocess.DEVNULL,
                    stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            receipt['launcher_pid']=child.pid;b=Broker(channel)
            event,entry=b.entry(0x10010,0xe9);base=event['base']
            receipt['loader_entry']={'event':event,'context':entry}
            receipt['loaded_loader']=loader_bytes(b,base)
            _,receipt['segments']=b.flat_selectors(entry)
            index,stop,actual=b.wcc_prefix()
            require(stop['pid']==event['pid'] and stop['tid']==event['tid'] and not actual['eflags']&0x100,
                    'foreign launch stop or active TF')
            require(b.read(base+0x10763,5)==b'\xcc\xad\xf8\xff\xff','launch trap changed')
            receipt['launch']={'event':stop,'context':actual,'trap_index':index}
            slots=struct.unpack('<4I',b.read(base+0x30000,16));entry_address,allocation,base_address,break_flag=slots
            require(allocation==base_address and entry_address==allocation+image.entry_offset and
                    0<allocation and allocation+image.allocation_size<=0x100000000,'actual CF allocation/entry mismatch')
            require(actual['esi']==entry_address and actual['ebx']==allocation and actual['edx']==base+0x10788 and
                    actual['edi']==actual['esp']+0x104,'loader launch register contract mismatch')
            params=struct.unpack('<5I',b.read(actual['edi'],20))
            require(params[3]==base+0x3000c and params[4]==0,'five-field launch parameters mismatch')
            name=terminated(b,params[0]);tail=terminated(b,params[1]);environment=terminated(b,params[2],double=True)
            require(name.decode('ascii').lower()==win(target).lower()+'\0','payload self path differs')
            require(tail==request['arguments'].encode('ascii')+b'\0','payload command tail differs')
            receipt['target_environment']=verify_overlay(environment,request['target_env'])
            receipt['launch_inputs']={'slots':list(slots),'parameters':list(params),'program_name_hex':name.hex(),
                    'command_tail_hex':tail.hex(),'environment_sha256':sha(environment),'environment_bytes':len(environment)}
            regions=[];cursor=allocation
            while cursor<allocation+image.allocation_size:
                region=b.region(cursor);regions.append(region)
                require(0<region['allocation_base']<=allocation and region['state']==0x1000 and
                        region['protect'] in (2,4,8,0x20,0x40,0x80),'CF allocation not committed/readable')
                cursor=min(region['base']+region['size'],allocation+image.allocation_size)
            receipt['allocation_regions']=regions
            snapshot=read_all(b,allocation,image.loaded_size);receipt['cf_identity']=image.compare(snapshot,allocation)
            require(b.context()==actual and b.read(base+0x30000,16)==struct.pack('<4I',*slots),'stopped launch changed')
            receipt['entry_handoff']=enter(b,event,actual,allocation,entry_address,snapshot)
            b.stop();out,err=child.communicate(timeout=6)
            require(child.returncode==0 and not out and not err,'broker failed/unexpected launcher streams')
            log=(work/'cf-events').read_bytes()
            require(b'owned-teardown exit-event=1 signaled=1' in log and b'cf-instructions=0' in log,
                    'missing actual owned teardown/zero-CF-instruction evidence')
            require((work/'original-stdout').read_bytes()==b'' and (work/'original-stderr').read_bytes()==b'',
                    'unexpected output before CF entry')
            receipt['passed']=True
    except BaseException as error:
        receipt['error']=repr(error)
        raise
    finally:
        if child is not None:
            try:out,err=child.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                child.kill();out,err=child.communicate(timeout=3);forced=True
            (work/'cf-launcher-stdout').write_bytes(out);(work/'wine-diagnostics').write_bytes(err)
            receipt['broker_status']=child.returncode;receipt['forced_outer_stop_failed_proof']=forced
        if b is not None:receipt['events']=b.events
        if channel is not None:receipt['rpc_count']=len(channel.records)
        receipt['elapsed_ms']=(time.monotonic()-start)*1000
        if (work/'cf-events').exists():receipt['event_log']=(work/'cf-events').read_text(errors='replace')
        (work/'cf-result').write_text(json.dumps(receipt))
    return receipt

if __name__=='__main__':
    try:execute(json.loads(Path(sys.argv[1]).read_text()))
    except BaseException as error:
        print('watcom-cf backend: '+str(error),file=sys.stderr);sys.exit(1)
