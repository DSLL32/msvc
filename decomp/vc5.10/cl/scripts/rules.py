"""Verify the complete encountered option grammar in all three compiled images."""
import hashlib
import importlib.machinery
import json
from pathlib import Path
import struct
import subprocess
from binary import PE

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()

class ELF:
    def __init__(self,path):
        self.data=Path(path).read_bytes();assert self.data[:5]==b'\x7fELF\x01'
        shoff=struct.unpack_from('<I',self.data,32)[0]
        count=struct.unpack_from('<H',self.data,48)[0]
        self.sections=[struct.unpack_from('<10I',self.data,shoff+40*i)for i in range(count)]
        self.symbols={}
        for line in subprocess.check_output(['nm','-n','--defined-only',str(path)],text=True).splitlines():
            fields=line.split()
            if len(fields)==3:self.symbols[fields[2]]=int(fields[0],16)
    def read(self,address,size):
        for section in self.sections:
            if section[3]<=address and address+size<=section[3]+section[5]:
                if section[1]==8:return bytes(size)
                offset=section[4]+address-section[3];return self.data[offset:offset+size]
        raise AssertionError('unbacked native grammar address')
    def string(self,address):
        value=bytearray()
        for i in range(4096):
            byte=self.read(address+i,1)
            if byte==b'\0':return value.decode('latin1')
            value+=byte
        raise AssertionError('unterminated grammar literal')


def decode(image,start,callback_symbols):
    result=[]
    for i in range(175):
        fields=struct.unpack('<5I',image.read(start+20*i,20))
        if not fields[0]:
            assert i==174 and fields==(0,)*5,'wrong grammar terminator'
            return result
        values=[image.string(v)if v else None for v in fields[:4]]
        if '@' in values[0]:
            assert fields[4] in callback_symbols,'unknown callback pointer in @-mode rule'
            extra={'handler':callback_symbols[fields[4]]}
        else:
            assert fields[4] not in callback_symbols,'callback pointer used as requirement text'
            extra={'requirements':image.string(fields[4])if fields[4] else None}
        result.append(dict(pattern=values[0],add=values[1],conflict=values[2],replace=values[3],extra=extra))
    raise AssertionError('missing grammar terminator')


def verify(project=PROJECT,root=ROOT):
    proof=json.loads((project/'cases/help/rule-grammar.json').read_text())
    original_path=root/'toolchains/5.10/bin/CL.EXE';original=PE(original_path)
    assert sha(original_path)==proof['target_sha256']
    assert hashlib.sha256(original.read(proof['start'],175*20)).hexdigest()==proof['record_bytes_sha256']
    original_callbacks={r['address']:r['name']for r in proof['callbacks']}
    expected=decode(original,proof['start'],original_callbacks)
    identity=importlib.machinery.SourceFileLoader('cl_runtime_identity',str(project/'scripts/identify-runtime.py')).load_module()
    public=identity.public_map(project/'build/cl.map')
    rebuilt=PE(project/'build/cl.exe')
    actual=decode(rebuilt,public['_option_rules'],{public['_'+r['name']]:r['name']for r in proof['callbacks']})
    assert actual==expected,'rebuilt Win32 grammar fields/handler identities differ'
    native=ELF(project/'build/native-trace/CL.EXE')
    actual=decode(native,native.symbols['option_rules'],{native.symbols[r['name']]:r['name']for r in proof['callbacks']})
    assert actual==expected,'native grammar fields/handler identities differ'
    return {'records':174,'record_size':20,'terminator':'five zero words',
        'decoded_sha256':hashlib.sha256(json.dumps(expected,sort_keys=True).encode()).hexdigest(),
        'callbacks':proof['callbacks'],'verified_images':{str(path):sha(path)for path in
            (original_path,project/'build/cl.exe',project/'build/native-trace/CL.EXE')}}

if __name__=='__main__':print(json.dumps(verify(),indent=2))
