"""Strict native initial MRE prefix provider, outside the reconstructed graph.

No dependency code is executed. Before every replay effect the actual native
request is compared to the authenticated original prefix. Unknown lifecycle and
all later MRE calls remain denied. This module does not authorize acceptance.
"""
import hashlib,json,socket,struct,threading
from pathlib import Path
from dependency_native import Reader
from dependency_packet import vector
from mre_object_packet import decode as original_packet
from case_inputs import load

PROJECT=Path(__file__).resolve().parents[1]
sha=lambda value:hashlib.sha256(value).hexdigest()
encoded=lambda value:(json.dumps(value,sort_keys=True,indent=2)+'\n').encode()
MAX_MESSAGE=2*1024*1024


def configuration(identity,environment,os_block,idb):
    assert len(identity)==64 and all(c in '0123456789abcdef'for c in identity)
    assert 0<len(environment)<=4096 and len(os_block)%2==0 and os_block.endswith(b'\0\0\0\0')
    result=b'RCM1'+struct.pack('<I',1)+identity.encode()+struct.pack('<I',len(environment))
    for value in environment:
        value=value.encode('latin1');assert value and b'\0'not in value and len(value)<=32767
        result+=struct.pack('<I',len(value))+value
    assert len(idb)==25600
    return result+struct.pack('<I',len(os_block)//2)+os_block+struct.pack('<I',len(idb))+idb


def decode(payload,sort=False):
    reader=Reader(payload)
    fields=dict(zip(('errno','doserrno','last_error','controlfp','fmode','commode','mb_cur_max','mb_codepage','lc_codepage'),reader.words(9)))
    fields['lc_handles']=reader.words(6)
    fields['mbctype']=reader.take(257).hex();fields['ctype']=reader.take(514).hex();fields['pctype']=reader.word()
    fields['environment']=reader.vector();fields['initial_environment']=reader.vector()
    address,units,updates=reader.words(3)
    assert address and 2<=units<=65536
    raw=reader.take(units*2);text=raw.decode('utf-16le')
    assert text.endswith('\0\0')and '\0\0'not in text[:-2]
    fields['os_environment']={'address':address,'units':units,'updates':updates,'bytes':raw.hex()}
    fields['file_words']=reader.words(24);fields['streams']=[]
    for unused in range(3):fields['streams'].append({'osfile':reader.word(),'crt':reader.handle(),'win32':reader.handle()})
    count=reader.word();assert count<=256
    fields['descriptors']=reader.words(count);assert fields['descriptors']==sorted(set(fields['descriptors']))
    fields['fd3_flags']=reader.word()
    if fields['fd3_flags']&1:
        fields['fd3']={'handle':reader.handle(),'cloexec':reader.word(),'access':reader.word()}
        size=reader.word();assert size==25600
        fields['fd3']['bytes']=reader.take(size).hex()
    else:fields['fd3']=None
    fields['driver']=reader.word();fields['driver_words']=reader.words(11);fields['scratch']=reader.take(1024).hex()
    count=reader.word();assert count<=4096
    fields['options']=[]
    def text():
        address=reader.word()
        return {'address':address,'text':reader.text()if address else None}
    for unused in range(count):
        fields['options'].append({'address':reader.word(),'words':reader.words(6),**{name:text()for name in('matching','name','value')}})
    fields.update(zip(('minimal_rebuild','reproducible','first_dependency_scan','interrupted','pass_running'),reader.words(5)))
    fields['pdb']=text()
    fields['owner_out']=reader.word();fields['path']=text()
    fields.update(zip(('error_out','message','reproducible_arg','write_arg','engine','driver_out','mre_driver','owner_value','error_value','driver_value'),reader.words(10)))
    fields['cwd']=text()
    if sort:
        fields['pending_work']=reader.word();fields['pending_words']=reader.words(5)
    reader.finish();return fields


class Pointers:
    """Injective actual ownership/alias relation, persistent across all six events."""
    def __init__(self):self.forward={0:0};self.reverse={0:0}
    def bind(self,original,native):
        assert type(original)is int and type(native)is int and 0<=native<=0xffffffff
        assert self.forward.get(original,native)==native,'changed native alias'
        assert self.reverse.get(native,original)==original,'collapsed distinct original roles'
        self.forward[original]=native;self.reverse[native]=original


class Provider:
    def __init__(self,original,directory,signature_candidate=False,selector_candidate=False,sort_candidate=False):
        self.original=Path(original).resolve();self.directory=Path(directory).resolve()
        self.signature_candidate=signature_candidate
        assert all(type(v)is bool for v in(signature_candidate,selector_candidate,sort_candidate))and sum((signature_candidate,selector_candidate,sort_candidate))<=1
        self.sort_candidate=sort_candidate
        self.selector_candidate=selector_candidate
        if sort_candidate:
            from mre_sort_packet import decode as sort_response
            self.packet=sort_response(self.original)
        elif selector_candidate:
            from mre_selector_packet import decode as selector_response
            self.packet=selector_response(self.original)
        elif signature_candidate:
            from mre_signature_packet import decode as signature_response
            self.packet=signature_response(self.original)
        else:
            self.packet=original_packet(self.original)
            assert self.packet==json.loads((PROJECT/'cases/mre-object/prefix-packet.json').read_bytes()),'unreviewed MRE prefix packet'
        self.identity=sha(encoded(self.packet));self.pointer=Pointers();self.phase=0;self.pid=None
        self.requests=[];self.errors=[];self.native_before=None;self.native_after=None
        self.inputs=load('mre-object')['inputs']
        env_address,environment=vector((self.original/'environment.bin').read_bytes())
        self.initial_env_address=env_address;self.environment_text=[value.decode('latin1')for address,value in environment]
        self.os_block=(self.original/'observer/mre-1-before-os-environment.bin').read_bytes()
        self.idb=(self.original/self.packet['intermediate_files'][0]['owned_contents']).read_bytes()
        assert sha(self.idb)==self.packet['intermediate_files'][0]['sha256']
        self.config=configuration(self.identity,self.environment_text,self.os_block,self.idb)
        self.config_path=self.directory/'mre-binding.bin';self.config_path.write_bytes(self.config)
        (self.directory/'mre-response.json').write_bytes(encoded(self.packet))
        self.server,self.target=socket.socketpair();self.server.settimeout(20)
        self.thread=threading.Thread(target=self._serve,daemon=True);self.thread.start()

    @property
    def artifacts(self):return getattr(self,'artifact_directory',self.directory)

    def environment(self):return {'CL_MRE_FD':str(self.target.fileno()),'CL_MRE_CONFIG':str(self.config_path)}
    def descriptors(self):return(self.target.fileno(),)

    @staticmethod
    def _read(stream,count,eof=False):
        result=bytearray()
        while len(result)<count:
            chunk=stream.recv(count-len(result))
            if not chunk and eof and not result:return None
            assert chunk,'truncated MRE request'
            result.extend(chunk)
        return bytes(result)

    def _serve(self):
        try:
            while True:
                header=self._read(self.server,16,eof=True)
                if header is None:return
                magic,kind,size,pid=struct.unpack('<4sIII',header)
                assert magic==b'MRE1'and 0<=size<=MAX_MESSAGE and pid>0
                assert self.pid in(None,pid);self.pid=pid
                assert self._read(self.server,64)==self.identity.encode()
                payload=self._read(self.server,size)
                assert kind==self.phase and kind<=(8 if self.sort_candidate else 6),'MRE missing/extra/ordering violation'
                if kind==0:assert payload==self.config,'actual MRE configuration differs'
                else:self.validate(kind,decode(payload,sort=kind>=7))
                self.requests.append({'kind':kind,'pid':pid,'payload_hex':payload.hex()})
                self.phase+=1;self.server.sendall(struct.pack('<4sI',b'RMA1',kind))
        except Exception as error:
            self.errors.append(repr(error));self.server.close()

    def validate(self,kind,native,live=True):
        for item in self.inputs:
            raw=(self.artifacts/item['name']).read_bytes()
            assert len(raw)==item['bytes']and sha(raw)==item['sha256'],'MRE actual complete input differs'
        assert not(self.artifacts/'case.pdb').exists(),'unexpected MRE PDB input/output'
        assert self.config_path.read_bytes()==self.config,'MRE configuration changed after binding'
        index=(kind-1)//2;after=not(kind&1);phase='mre-%d-%s'%(index+1,'after'if after else'before')
        call=self.packet['calls'][index];expected=call['after'if after else'before'];objects=self.packet['target_objects'][phase]
        assert {name:native[name]for name in('errno','doserrno','last_error')}==expected['errors'],'MRE actual error pre/post state'
        assert native['controlfp']==0x9001f and expected['x87_control_word']==0x27f
        for name in('fmode','commode','mb_cur_max','mb_codepage','lc_codepage','lc_handles'):assert native[name]==expected[name]
        assert bytes.fromhex(native['mbctype'])==(self.original/('observer/'+phase+'-mb-state.bin')).read_bytes()[4:]
        assert bytes.fromhex(native['ctype'])==(self.original/('observer/'+phase+'-ctype.bin')).read_bytes()
        bind=self.pointer.bind;bind(expected['pctype_address'],native['pctype'])
        for name in('environment','initial_environment'):
            original_address,original_entries=vector((self.original/'environment.bin').read_bytes())
            actual=native[name];bind(original_address,actual['address'])
            assert actual['strings']==[value.decode('latin1')for pointer,value in original_entries]
            for (pointer,value),address in zip(original_entries,actual['pointers']):bind(pointer,address)
        assert native['os_environment']['bytes']==self.os_block.hex()and native['os_environment']['updates']==0
        words=struct.unpack('<32I',(self.original/('observer/'+phase+'-crt-words.bin')).read_bytes())
        bind(words[19],native['os_environment']['address'])
        assert native['descriptors']==([0,1,2]if kind==1 else[0,1,2,3])
        for i in range(3):
            original_words=expected['file0_2_words'][i*8:i*8+8];actual=native['file_words'][i*8:i*8+8]
            for offset in(0,2,7):bind(original_words[offset],actual[offset])
            for offset in(1,3,4,5,6):assert original_words[offset]==actual[offset]
            lowio=(self.original/('observer/'+phase+'-lowio-0.bin')).read_bytes()
            assert native['streams'][i]['osfile']==lowio[36*i+4]
            for family in('crt','win32'):
                original=expected['streams'][family][i];observed=native['streams'][i][family]
                assert original['type']==observed['type']
                bind(original['handle'],observed['handle'])
                if original['type']==1:assert observed['position']==original['position_low']and original['position_high']==0
            crt,win=native['streams'][i]['crt'],native['streams'][i]['win32']
            assert crt==win,'MRE CRT/Win32 standard-handle correspondence'
        if kind!=1:
            fd=native['fd3'];assert fd and native['fd3_flags']==0x11
            assert fd['handle']['type']==1 and fd['handle']['position']==1024
            assert fd['cloexec']==1 and fd['access']&3==2 and bytes.fromhex(fd['bytes'])==self.idb
            if self.native_after is not None:assert fd==self.native_after['fd3'],'MRE provider fd lifetime/identity changed'
        else:
            assert native['fd3_flags']==0 and native['fd3']is None
            if live:assert not(self.artifacts/'case.idb').exists(),'MRE Open requires absent IDB'
        original_driver=objects['prepare_jobs_arguments'][0];bind(original_driver,native['driver'])
        for original,actual in zip(objects['driver_words'],native['driver_words']):bind(original,actual)
        assert bytes.fromhex(native['scratch'])==(self.original/('observer/'+phase+'-shared-scratch.bin')).read_bytes(),'MRE program-produced scratch changed'
        assert len(objects['options'])==len(native['options'])
        for original,actual in zip(objects['options'],native['options']):
            bind(original['address'],actual['address'])
            row=[original[name]for name in('next','flags','matching_pointer','name_pointer','value_pointer','origin')]
            assert row[1]==actual['words'][1]
            for i in(0,2,3,4,5):bind(row[i],actual['words'][i])
            for i,name in((2,'matching'),(3,'name'),(4,'value')):
                assert actual[name]['text']==original[name]and actual[name]['address']==actual['words'][i]
        assert native['minimal_rebuild']==native['first_dependency_scan']==1 and native['reproducible']==native['interrupted']==native['pass_running']==0
        assert native['pdb']['text']=='case.idb';assert Path(native['cwd']['text']).resolve()==self.directory
        if index==0:
            arguments=call['arguments'];bind(arguments[0],native['owner_out']);bind(arguments[1],native['path']['address'])
            bind(arguments[2],native['error_out']);bind(arguments[3],native['message'])
            assert native['path']['text']=='case.idb'and native['reproducible_arg']==0 and native['write_arg']==1
            assert native['path']['address']==native['pdb']['address']and not any(native[n]for n in('engine','driver_out','mre_driver','driver_value'))
            if after:
                bind(self.packet['opaque_roles']['engine'],native['owner_value']);assert native['error_value']==0
            else:assert native['owner_value']==native['error_value']==0 # Not read; wire absence markers only.
        elif index==1:
            bind(call['arguments'][0],native['engine']);bind(call['arguments'][1],native['driver_out'])
            if after:
                bind(self.packet['opaque_roles']['driver'],native['driver_value']);assert native['driver_value']==native['engine']+4
            else:assert native['driver_value']==0 # Uninitialized output is not observed.
            assert not any(native[n]for n in('owner_out','error_out','message','mre_driver','owner_value','error_value','reproducible_arg','write_arg'))and native['path']['text']is None
        else:
            bind(call['arguments'][0],native['mre_driver'])
            assert not any(native[n]for n in('owner_out','error_out','message','engine','driver_out','driver_value','owner_value','error_value','reproducible_arg','write_arg'))and native['path']['text']is None
        if index==3:
            bind(call['arguments'][1],native['pending_work'])
            assert native['pending_words']==[0]*5,'unknown MRE pending payload'
        if kind==1:self.native_before=native
        if kind==2:self.native_after=native
        if kind>=2:
            for name in('file_words','streams','environment','initial_environment','os_environment','pctype','driver','driver_words','options','scratch'):
                if kind>=7 and name in('driver_words','scratch'):continue
                assert native[name]==self.native_before[name],'MRE target state reseed/mutation '+name

    def finish(self,pid):
        result={'schema':1,'pid':pid,'response_sha256':self.identity,'config_sha256':sha(self.config),
                'original_capture_sha256':self.packet['capture_sha256'],'requests':self.requests,'errors':self.errors,
                'scope':'Fresh comparator-candidate four-call MRE response'if self.sort_candidate else'Fresh selector-candidate MRE response'if self.selector_candidate else'Fresh signature-candidate MRE response'if self.signature_candidate else'Initial MRE prefix only; no acceptance or later lifecycle success'}
        (self.directory/'mre-native.json').write_bytes(encoded(result))
        assert not self.errors,self.errors
        assert self.pid==pid and self.phase==(9 if self.sort_candidate else 7) and [r['kind']for r in self.requests]==list(range(9 if self.sort_candidate else 7)),'MRE mandatory1/1/1 inventory failed'
        assert (self.directory/'case.idb').read_bytes()==self.idb
        return result

    def close(self):
        self.target.close()
        try:self.server.shutdown(socket.SHUT_RDWR)
        except OSError:pass
        self.thread.join(timeout=1);self.server.close()
