"""Format and loader helpers for ordinary original-only CF capture."""
import hashlib,json,os,struct,sys
from pathlib import Path
from ipc import ProtocolError

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
    from debugger import execute as debug_execute
    return debug_execute(request)

if __name__=='__main__':
    try:execute(json.loads(Path(sys.argv[1]).read_text()))
    except BaseException as error:
        print('watcom-cf backend: '+str(error),file=sys.stderr);sys.exit(1)
