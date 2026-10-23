"""Verify frozen first-case inputs and derive the in-process response from raw events."""
import re
import struct
from common import HERE,ROOT,read,sha,hex_records


def derive(text):
    calls=re.findall(r'^export-args (.*)$',text,re.M)
    assert len(calls)==1,'Missing/ambiguous captured export'
    words=calls[0].split()
    assert len(words)==6 and words[:5]==['00000000','00000000','01001100','00000000','2'] and int(words[5],16),'Unexpected captured export arguments'
    assert re.findall(r'^callback ([0-9]+) ([0-9]+)$',text,re.M)==[('0','0')]*4,'Unexpected callback arguments'
    assert re.findall(r'^callback-return (-?[0-9]+)$',text,re.M)==['0']*4,'Unexpected callback returns'
    assert re.findall(r'^export-result (-?[0-9]+)$',text,re.M)==['0'],'Unexpected RC result'
    assert re.findall(r'^exit ([0-9]+)$',text,re.M)==['0'],'Reference did not complete'
    argv=hex_records(text,'argv');env=hex_records(text,'env')
    assert argv==hex_records(text,'main-argv') and env==hex_records(text,'main-env'),'Startup vectors changed before main'
    assert len(argv)==2 and len(env)==16,'Incomplete startup vectors'
    before=hex_records(text,'before');after=hex_records(text,'after');assert len(before)==len(after)==4
    blob=bytearray(b'RCRP\1\0\0\0')
    def word(n):blob.extend(struct.pack('<I',n))
    for vector in [argv,env]:
        word(len(vector))
        for value in vector:
            b=bytes.fromhex(value);word(len(b));blob.extend(b)
    word(4)
    for a,b in zip(before,after):
        aa=bytes.fromhex(a);bb=bytes.fromhex(b);assert len(aa)==len(bb)
        for n in [0,0,len(aa),0]:word(n)
        blob.extend(aa);blob.extend(bb)
    word(0)
    return bytes(blob)


def verify(config):
    reference=HERE/'cases/help/reference';manifest=reference/'manifest.json'
    assert sha(manifest)==config['reference_manifest_sha256'],'First-case master manifest changed'
    spec=read(manifest)
    for name,pin in spec['files'].items():
        path=reference/name;assert path.stat().st_size==pin['bytes'] and sha(path)==pin['sha256'],'Frozen master changed '+name
    for name,pin in spec['dependencies'].items():assert sha(ROOT/name)==pin,'Original dependency changed '+name
    text=(reference/'events.txt').read_text()
    assert (reference/'rcdll-response.bin').read_bytes()==derive(text),'Dependency response was not derived from raw callbacks'
    assert (reference/'argv.bin').read_bytes()==b''.join(bytes.fromhex(x) for x in hex_records(text,'argv'))+b'\0','Frozen argv differs'
    assert (reference/'environment.bin').read_bytes()==b''.join(bytes.fromhex(x) for x in hex_records(text,'env'))+b'\0','Frozen environment differs'
    assert (reference/'stdout.bin').stat().st_size==589 and (reference/'stderr.bin').stat().st_size==0,'Wrong master streams'
    return {p.name:sha(p) for p in reference.iterdir() if p.is_file()}
