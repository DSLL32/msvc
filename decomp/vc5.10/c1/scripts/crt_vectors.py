"""Private actual HOST CRT vectors, distinct from compiler-pass arguments."""
import json,re,struct
from provenance import sha
from allocation_imports import export_rva

def vector(path):
    raw=path.read_bytes();assert len(raw)>=8
    address,count=struct.unpack_from('<II',raw);assert address and count<=16384
    at=8;rows=[]
    for index in range(count):
        assert at+8<=len(raw);pointer,length=struct.unpack_from('<II',raw,at);at+=8
        assert pointer and 0<length<=32768 and at+length<=len(raw)
        value=raw[at:at+length];at+=length
        assert value.endswith(b'\0') and b'\0'not in value[:-1]
        rows.append((pointer,value))
    assert at==len(raw)
    return address,rows

def disjoint(paths):
    spans=[]
    for path in paths:
        address,rows=vector(path);spans.append((address,address+4*(len(rows)+1)))
        spans.extend((pointer,pointer+len(value))for pointer,value in rows)
    spans.sort();assert all(0<a<b<=0x100000000 for a,b in spans)
    assert all(a[1]<=b[0]for a,b in zip(spans,spans[1:]))

def prepare(original,output,candidate):
    assert candidate['crt_vectors']==dict(argv='original-crt-argv.bin',environment='original-crt-environment.bin',scope='actual canonical hostCRT inputs; distinct from Invoke11 and Linux host controls')
    receipt=json.loads((original/'receipt.json').read_text())
    assert sha(original/'candidate.json')==receipt['candidate_sha256']
    names=('original-crt-argv.bin','original-crt-environment.bin')
    for name in names:
        row=receipt['artifacts'][name];assert row['sha256']==sha(original/name) and row['size']==(original/name).stat().st_size
        (output/name).write_bytes((original/name).read_bytes())
    argv,args=vector(original/names[0]);environment,values=vector(original/names[1]);assert len(args)==3
    assert args[1][1]==b'--host-minimal\0'
    assert all(value[:-1].partition(b'=')[0].upper()!=b'MSC_CMD_FLAGS'for pointer,value in values)
    assert(original/names[1]).read_bytes()==(original/'invoke-environment.bin').read_bytes()
    disjoint([original/name for name in names])
    events=(original/'events.txt').read_text();crt=int(re.findall(r'^module name=MSVCRT.dll actual=([0-9a-f]+)',events,re.M)[0],16)
    rows=re.findall(r'^host-crt argc=(\d+) argv=([0-9a-f]+) environment=([0-9a-f]+) argc-export=([0-9a-f]+) argv-export=([0-9a-f]+) environment-export=([0-9a-f]+)$',events,re.M);assert len(rows)==1;r=rows[0]
    assert(int(r[0]),int(r[1],16),int(r[2],16))==(len(args),argv,environment)
    assert[int(x,16)for x in r[3:]]==[crt+export_rva(original/'loaded-msvcrt.bin',name)for name in ('__argc','__argv','_environ')]
    binding=dict(kind='actual HOST CRT vectors; separate from Invoke11 and Linux host controls',original_directory=str(original.resolve()),original_receipt_sha256=sha(original/'receipt.json'),
        files={name:sha(original/name)for name in names},host_argc=len(args),environment_count=len(values),pass_argc=candidate['case_contract']['argc'])
    (output/'crt-vectors-binding.json').write_text(json.dumps(binding,indent=2)+'\n')

def validate(original,native,invocation,before,after):
    names=('original-crt-argv.bin','original-crt-environment.bin')
    new_names=('native-crt-argv.bin','native-crt-environment.bin')
    old=[vector(original/name)for name in names];new=[vector(native/name)for name in new_names]
    assert all([value for pointer,value in left[1]]==[value for pointer,value in right[1]]for left,right in zip(old,new))
    for name in names:assert(native/name).read_bytes()==(original/name).read_bytes()
    binding=json.loads((native/'crt-vectors-binding.json').read_text())
    assert binding['original_receipt_sha256']==sha(original/'receipt.json')
    assert binding['files']=={name:sha(original/name)for name in names}
    assert binding['host_argc']==len(old[0][1])==3 and binding['environment_count']==len(old[1][1])
    assert binding['pass_argc']==invocation['args']['argc']==11
    assert before['crt']==dict(bound=1,argc=3,argv=dict(address=new[0][0],count=3),environment=dict(address=new[1][0],count=len(old[1][1])))
    assert after['crt']==dict(bound=1,environment=new[1][0])
    for directory,files in ((original,names),(native,new_names)):disjoint([directory/name for name in files])
    # The actual host CRT arrays and compiler-pass array have distinct storage.
    # Their strings are individually observed too, rather than identifying
    # vectors solely by a copied argument count.
    events=(original/'events.txt').read_text()
    words=re.findall(r'^trap seq=\d+ rva=00012229 .*stack=([0-9a-f,]+)$',events,re.M);assert len(words)==1
    old_pass=int(words[0].split(',')[2],16);new_pass=invocation['args']['argv']
    for host,pass_address in ((old,old_pass),(new,new_pass)):
        assert all(array!=pass_address for array,rows in host)
        spans=[(array,array+4*(len(rows)+1))for array,rows in host]
        spans.extend((pointer,pointer+len(value))for array,rows in host for pointer,value in rows)
        spans.append((pass_address,pass_address+12*4))
        if host is old:
            for index in range(11):
                record=re.findall(r'^invoke-arg index=%d address=([0-9a-f]+) bytes=(\d+)$'%index,events,re.M);assert len(record)==1
                pointer,length=int(record[0][0],16),int(record[0][1]);spans.append((pointer,pointer+length))
        else:
            spans.extend((pointer,pointer+len(value.encode('ascii'))+1)for pointer,value in zip(invocation['argv_pointers'],invocation['argv']))
        spans.sort();assert all(a[1]<=b[0]for a,b in zip(spans,spans[1:]))
