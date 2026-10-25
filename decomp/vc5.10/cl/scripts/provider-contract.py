#!/usr/bin/env python3
"""CL-owned typed dependency contracts only. No original/proof/capture fallback.

The C IO adapter starts this process after the checkpoint. Only these three
actual CL wire families and the frozen per-case full inputs are recognized.
"""
import hashlib,json,os,struct,sys
from pathlib import Path
MAX_MESSAGE=2*1024*1024
sha=lambda value:hashlib.sha256(value).hexdigest()
class Reader:
    def __init__(self, data):
        if not isinstance(data, bytes) or len(data) > MAX_MESSAGE:
            raise ValueError('invalid native dependency payload')
        self.data, self.offset = data, 0

    def take(self, length):
        if length < 0 or length > len(self.data) - self.offset:
            raise ValueError('truncated native dependency payload')
        value = self.data[self.offset:self.offset + length]
        self.offset += length
        return value

    def word(self):
        return struct.unpack('<I', self.take(4))[0]

    def words(self, count):
        return list(struct.unpack('<%dI' % count, self.take(4 * count)))

    def text(self):
        length = self.word()
        if length > 32767:
            raise ValueError('oversized native dependency string')
        value = self.take(length)
        if b'\0' in value:
            raise ValueError('embedded NUL in native dependency string')
        return value.decode('latin1')

    def vector(self, required=True):
        address, count = self.words(2)
        if count > 4096 or (not address and (count or required)):
            raise ValueError('invalid native dependency vector')
        pointers, strings = [], []
        for unused in range(count):
            pointer = self.word()
            if not pointer:
                raise ValueError('NULL native dependency string pointer')
            pointers.append(pointer)
            strings.append(self.text())
        return {'address': address, 'pointers': pointers, 'strings': strings}

    def handle(self):
        handle, kind, device_lo, device_hi, inode_lo, inode_hi, pos_lo, pos_hi = self.words(8)
        if not handle or kind not in (1, 2):
            raise ValueError('unobserved native stream type')
        position = pos_lo | pos_hi << 32
        if position & (1 << 63):
            position -= 1 << 64
        if position < 0 and not (kind == 2 and position == -1):
            raise ValueError('unobserved native stream position')
        return {'handle': handle, 'type': kind,
                'device': device_lo | device_hi << 32,
                'inode': inode_lo | inode_hi << 32, 'position': position}

    def finish(self):
        if self.offset != len(self.data):
            raise ValueError('unused native dependency payload bytes')


def c1_decode(data):
    reader = Reader(data)
    fields = dict(zip(('errno', 'doserrno', 'last_error', 'controlfp', 'fmode',
                       'commode', 'mb_cur_max', 'mb_codepage', 'lc_codepage'), reader.words(9)))
    fields['lc_handles'] = reader.words(6)
    fields['mbctype'] = reader.take(257).hex()
    fields['ctype'] = reader.take(514).hex()
    fields.update(zip(('interrupted', 'pass_running', 'pass_flags', 'pass_is_current',
                       'module_is_c1', 'invoke_is_c1', 'abort_is_c1', 'provider_state',
                       'environment_is_initial', 'initial_environment_is_initenv'), reader.words(10)))
    for name in ('pass_is_current', 'module_is_c1', 'invoke_is_c1', 'abort_is_c1',
                 'environment_is_initial', 'initial_environment_is_initenv'):
        if fields[name] not in (0, 1):
            raise ValueError('untyped native dependency alias observation')
    fields['pass_name'], fields['pass_environment'] = reader.text(), reader.text()
    fields['argc'], fields['last'] = reader.words(2)
    fields['argv'] = reader.vector(required=False)
    fields['environment'] = reader.vector()
    fields['initial_environment'] = reader.vector()
    os_address, os_units, updates = reader.words(3)
    if not os_address or not 2 <= os_units <= 65536:
        raise ValueError('invalid native OS environment block')
    os_bytes = reader.take(os_units * 2)
    if not os_bytes.endswith(b'\0\0\0\0'):
        raise ValueError('unterminated native OS environment block')
    # Validate UTF-16 rather than accepting malformed surrogate sequences.
    os_text = os_bytes.decode('utf-16le')
    if '\0\0' in os_text[:-2] or not os_text.endswith('\0\0'):
        raise ValueError('embedded terminator in native OS environment block')
    fields['os_environment'] = {'address': os_address, 'updates': updates,
                                'bytes': os_bytes.hex()}
    fields['file_words'] = reader.words(24)
    fields['streams'] = []
    for unused in range(3):
        fields['streams'].append({'osfile': reader.word(), 'crt': reader.handle(),
                                  'win32': reader.handle()})
    fields['open_descriptors'] = reader.words(reader.word())
    if len(fields['open_descriptors']) > 256 or fields['open_descriptors'] != sorted(set(fields['open_descriptors'])):
        raise ValueError('invalid native open-descriptor inventory')
    fields['cwd'] = reader.text()
    reader.finish()
    return fields


def argument_storage(vector):
    """Prove the reached caller's contiguous live strings and disjoint argv."""
    address, pointers, strings = (vector[name] for name in ('address', 'pointers', 'strings'))
    if not address or not pointers or len(pointers) != len(strings):
        raise ValueError('missing actual argument storage')
    offsets, length = [], 0
    for pointer, text in zip(pointers, strings):
        if pointer != pointers[0] + length:
            raise ValueError('argument strings are not contiguous')
        offsets.append(length)
        length += len(text.encode('latin1')) + 1
    if not (address + 4 * (len(pointers) + 1) <= pointers[0] or pointers[0] + length <= address):
        raise ValueError('argument vector overlaps live strings')
    return offsets


def stream_roles(fields):
    """Resolve CRT and Win32 aliases separately, retaining their observed roles."""
    streams = fields['streams']
    if fields['open_descriptors'] != [0, 1, 2]:
        raise ValueError('unexpected open target descriptor')
    roles = []
    for index, stream in enumerate(streams):
        words = fields['file_words'][index * 8:index * 8 + 8]
        pointer, count, base, flags, descriptor, character, capacity, temporary = words
        if pointer != base or count or temporary or descriptor != index or character:
            raise ValueError('unobserved FILE fields/live buffer')
        if index == 0:
            if not pointer or flags != 0x101 or capacity != 4096:
                raise ValueError('stdin provider storage differs')
        elif pointer or flags != 2 or capacity:
            raise ValueError('output FILE state differs')
        crt, win32 = stream['crt'], stream['win32']
        if crt != win32:
            raise ValueError('CRT/Win32 standard handle roles differ')
        if crt['type'] != (2 if index == 0 else 1):
            raise ValueError('standard stream kind differs')
        roles.append({'osfile': stream['osfile'], 'position': crt['position']})
    left, right = streams[1]['crt'], streams[2]['crt']
    if (left['device'], left['inode']) != (right['device'], right['inode']):
        raise ValueError('stdout/stderr do not alias the same file')
    return roles


def c1_validate(fields, expected, cwd, output_file, lowio_flags, previous=None):
    """Reject unknown ABI, state, stream or pointer-ownership inputs before effects."""
    for name, value in expected['scalar'].items():
        if fields[name] != value:
            raise ValueError('native dependency input differs: ' + name)
    for name in ('mbctype', 'ctype', 'pass_name', 'pass_environment'):
        if fields[name] != expected[name]:
            raise ValueError('native dependency input differs: ' + name)
    if fields['argv']['strings'] != expected['argv']:
        raise ValueError('native dependency argv differs')
    if expected['argv']:
        if argument_storage(fields['argv']) != expected['argv_offsets']:
            raise ValueError('native dependency argv storage offsets differ')
    elif fields['argv']['address'] or fields['argv']['pointers']:
        raise ValueError('unexpected lifecycle argument storage')
    for name in ('environment', 'initial_environment'):
        value = fields[name]
        if value['strings'] != expected[name] or len(set(value['pointers'])) != len(value['pointers']):
            raise ValueError('native dependency namespace/order/ownership differs: ' + name)
    current, initial = fields['environment'], fields['initial_environment']
    if current['address'] == initial['address'] or set(current['pointers']) & set(initial['pointers']):
        raise ValueError('initial CRT environment was not deep-copied')
    os_environment = fields['os_environment']
    if os_environment['bytes'] != expected['os_environment'] or os_environment['updates'] != expected['os_updates']:
        raise ValueError('native dependency OS namespace/order differs')
    if fields['cwd'] != str(Path(cwd).resolve()):
        raise ValueError('native dependency working directory differs')
    roles = stream_roles(fields)
    reference = Path(output_file).stat()
    for index, (observed, wanted) in enumerate(zip(fields['streams'], expected['streams'])):
        if observed['osfile'] != lowio_flags[index] or observed['crt']['type'] != wanted['type']:
            raise ValueError('native dependency lowio flags/handle kind differs')
        if index:
            handle = observed['crt']
            if handle['position'] != wanted['position'] or \
               (handle['device'], handle['inode']) != (reference.st_dev, reference.st_ino):
                raise ValueError('native dependency output role/position differs')
    if previous is not None:
        for name in ('environment', 'initial_environment', 'os_environment', 'file_words', 'streams', 'open_descriptors'):
            if fields[name] != previous[name]:
                raise ValueError('native DLL lifecycle changed caller-owned state: ' + name)
    return {'argument_offsets': expected['argv_offsets'], 'streams': roles,
            'environment_strings': len(current['strings']), 'initial_strings_deep_copied': True}

def mre_decode(payload,sort=False):
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


class MreContract:
    def __init__(self,data):
        self.packet=data;self.directory=Path.cwd();self.pointer=Pointers()
        self.inputs=data['inputs'];self.environment_text=[x[1]for x in data['environment']]
        self.os_block=bytes.fromhex(data['os_environment']);self.idb=bytes.fromhex(data['idb'])
        self.final_idb=bytes.fromhex(data['final_idb']);self.native_before=None;self.native_after=None
        self.config_path=Path('mre-binding.bin');self.config=self.config_path.read_bytes()
    @property
    def artifacts(self):return self.directory
    def validate(self,kind,native,live=True):
        for item in self.inputs:
            raw=(self.artifacts/item['name']).read_bytes()
            assert len(raw)==item['bytes']and sha(raw)==item['sha256'],'MRE actual complete input differs'
        assert not(self.artifacts/'case.pdb').exists(),'unexpected MRE PDB input/output'
        assert self.config_path.read_bytes()==self.config,'MRE configuration changed after binding'
        index=(kind-1)//2;after=not(kind&1);phase='mre-%d-%s'%(index+1,'after'if after else'before')
        call=self.packet['calls'][index];expected=call['after'if after else'before'];objects=self.packet['target_objects'][phase];state=self.packet['states'][phase]
        assert {name:native[name]for name in('errno','doserrno','last_error')}==expected['errors'],'MRE actual error pre/post state'
        assert native['controlfp']==0x9001f and expected['x87_control_word']==0x27f
        for name in('fmode','commode','mb_cur_max','mb_codepage','lc_codepage','lc_handles'):assert native[name]==expected[name]
        assert bytes.fromhex(native['mbctype'])==bytes.fromhex(state['mbctype'])
        assert bytes.fromhex(native['ctype'])==bytes.fromhex(state['ctype'])
        bind=self.pointer.bind;bind(expected['pctype_address'],native['pctype'])
        for name in('environment','initial_environment'):
            original_address=self.packet['environment_address'];original_entries=[(address,text.encode('latin1'))for address,text in self.packet['environment']]
            actual=native[name];bind(original_address,actual['address'])
            assert actual['strings']==[value.decode('latin1')for pointer,value in original_entries]
            for (pointer,value),address in zip(original_entries,actual['pointers']):bind(pointer,address)
        assert native['os_environment']['bytes']==self.os_block.hex()and native['os_environment']['updates']==0
        bind(state['os_address'],native['os_environment']['address'])
        assert native['descriptors']==([0,1,2]if kind in(1,12)else[0,1,2,3])
        for i in range(3):
            original_words=expected['file0_2_words'][i*8:i*8+8];actual=native['file_words'][i*8:i*8+8]
            for offset in(0,2,7):bind(original_words[offset],actual[offset])
            for offset in(1,3,4,5,6):assert original_words[offset]==actual[offset]
            assert native['streams'][i]['osfile']==state['lowio'][i]
            for family in('crt','win32'):
                original=expected['streams'][family][i];observed=native['streams'][i][family]
                assert original['type']==observed['type']
                bind(original['handle'],observed['handle'])
                if original['type']==1:assert observed['position']==original['position_low']and original['position_high']==0
            crt,win=native['streams'][i]['crt'],native['streams'][i]['win32']
            assert crt==win,'MRE CRT/Win32 standard-handle correspondence'
        if kind not in(1,12):
            fd=native['fd3'];assert fd and native['fd3_flags']==0x11
            assert fd['handle']['type']==1 and fd['handle']['position']==1024
            assert fd['cloexec']==1 and fd['access']&3==2 and bytes.fromhex(fd['bytes'])==self.idb
            if self.native_after is not None:assert fd==self.native_after['fd3'],'MRE provider fd lifetime/identity changed'
        else:
            assert native['fd3_flags']==0 and native['fd3']is None
            if live:
                if kind==1:assert not(self.artifacts/'case.idb').exists(),'MRE Open requires absent IDB'
                else:assert (self.artifacts/'case.idb').read_bytes()==self.final_idb,'MRE Close actual output bytes differ'
        original_driver=objects['prepare_jobs_arguments'][0];bind(original_driver,native['driver'])
        for original,actual in zip(objects['driver_words'],native['driver_words']):bind(original,actual)
        assert bytes.fromhex(native['scratch'])==bytes.fromhex(state['scratch']),'MRE program-produced scratch changed'
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
        elif index==5:
            bind(call['arguments'][0],native['engine']);assert native['write_arg']==call['arguments'][1]==1
            assert not any(native[n]for n in('owner_out','error_out','message','mre_driver','driver_out','driver_value','owner_value','error_value','reproducible_arg'))and native['path']['text']is None
        else:
            bind(call['arguments'][0],native['mre_driver'])
            assert not any(native[n]for n in('owner_out','error_out','message','engine','driver_out','driver_value','owner_value','error_value','reproducible_arg','write_arg'))and native['path']['text']is None
        if index>=3:
            bind(objects['prepare_jobs_arguments'][2],native['pending_work'])
            assert native['pending_words']==[0]*5,'unknown MRE pending payload'
        if kind==1:self.native_before=native
        if kind==2:self.native_after=native
        if kind>=2:
            for name in('file_words','streams','environment','initial_environment','os_environment','pctype','driver','driver_words','options','scratch'):
                if kind>=7 and name in('driver_words','scratch'):continue
                assert native[name]==self.native_before[name],'MRE target state reseed/mutation '+name


def record(path):
    raw=Path(path).read_bytes()
    return {'bytes':len(raw),'sha256':sha(raw)}


def verify_files(expected):
    for name,item in expected.items():
        assert Path(name).name==name and record(name)==item,'different complete dependency input: '+name


def create_output(name,oracle,expected):
    assert Path(name).name==name and not Path(name).exists(),'preexisting dependency output'
    raw=Path(oracle).read_bytes()
    assert expected=={'bytes':len(raw),'sha256':sha(raw)},'different captured dependency output'
    with Path(name).open('xb')as file:file.write(raw)


def serve(fd):
    contract=json.loads(Path('dependency-contract.json').read_bytes())
    family=contract['kind'];pid=os.getppid();cwd=Path.cwd();phase=0;spawned=0;previous=None
    mre=MreContract(contract)if family=='mre'else None
    config=Path('mre-binding.bin'if mre else'dependency-config.bin'if family=='minimal'else'link-binding.bin').read_bytes()
    identity=config[8:72];assert len(identity)==64 and all(c in b'0123456789abcdef'for c in identity)
    def read(count,eof=False):
        raw=bytearray()
        while len(raw)<count:
            block=os.read(fd,count-len(raw))
            if not block and eof and not raw:return None
            assert block,'incomplete CL dependency request'
            raw+=block
        return bytes(raw)
    def write(raw):
        while raw:
            count=os.write(fd,raw);assert count>0;raw=raw[count:]
    while True:
        magic=read(4,True)
        if magic is None:break
        if magic==b'MRE1':
            kind,size,owner=struct.unpack('<3I',read(12))
            assert family=='mre'and owner==pid and kind==phase<=12 and size<=MAX_MESSAGE
            assert read(64)==identity;payload=read(size)
            if kind==0:assert payload==config
            else:mre.validate(kind,mre_decode(payload,sort=kind>=7))
            phase+=1;write(struct.pack('<4sI',b'RMA1',kind))
        elif magic==b'CLD1':
            kind,size=struct.unpack('<2I',read(8))
            assert family=='minimal'and kind==phase<=3 and 68<=size<=MAX_MESSAGE
            owner,=struct.unpack('<I',read(4));assert owner==pid and read(64)==identity
            payload=read(size-68);effect=0
            if kind==0:assert payload==config
            else:
                fields=c1_decode(payload)
                c1_validate(fields,contract['contracts'][str(kind)],cwd,Path('/proc')/str(pid)/'fd/1',contract['lowio_flags'],previous)
                for item in contract['inputs']:assert record(item['name'])=={'bytes':item['bytes'],'sha256':item['sha256']}
                if kind==3:verify_files(contract['outputs'])
                else:
                    for name in contract['outputs']:assert not Path(name).exists()
                for name in('test.obj','vc50.pdb','vc50.idb'):assert not Path(name).exists()
                if kind==2:
                    for name,item in contract['outputs'].items():create_output(name,'.cl-c1-'+name,item)
                    effect=1
                previous=fields
            phase+=1;write(struct.pack('<4s5I',b'CLR1',0,effect,0,2 if effect else 0,0))
        elif magic==b'RSP2':
            assert family in('minimal','link')and not spawned
            assert family!='minimal'or phase==4,'C2 before complete C1 lifecycle'
            size,=struct.unpack('<I',read(4));assert 32<=size<=4*1024*1024
            r=Reader(read(size));mode,owner,argc,envc=r.words(4)
            errors=r.words(3);source=r.word();file=r.text();working=r.text()
            assert argc<=4096 and envc<=16384
            argv=[r.text()for unused in range(argc)];env=[r.text()for unused in range(envc)];r.finish()
            child=contract['child'];before=child['before']if family=='minimal'else contract['parent_state']['before']
            assert mode==0 and owner==pid and source==2
            assert errors==[before[k]for k in('errno','doserrno','last_error')]
            assert file==child['executable'if family=='minimal'else'path']and argv==child['argv']and env==child['environment']
            assert Path(working).resolve()==cwd
            verify_files(child['provider_inputs'])
            if family=='minimal':
                verify_files(contract['outputs']);assert record('C2.EXE')==child['executable_input']
                for name,item in child['outputs'].items():create_output(name,'.cl-c2-'+name,item)
                out=bytes.fromhex(child['stdout_hex']);err=bytes.fromhex(child['stderr_hex']);after=child['after']
            else:
                verify_files(child['inputs']);item=child['output'];create_output(item['name'],'.cl-link-output',{'bytes':item['bytes'],'sha256':item['sha256']})
                out=err=b'';after=contract['parent_state']['after']
            spawned=1
            write(struct.pack('<3I6I2I',0x32505352,5,0,*errors,*[after[k]for k in('errno','doserrno','last_error')],len(out),len(err))+out+err)
        else:raise AssertionError('unknown CL dependency protocol')
    assert (phase,spawned)==((13,0)if family=='mre'else(4,1)if family=='minimal'else(0,1)),'missing mandatory CL dependency call'


if __name__=='__main__':
    assert len(sys.argv)==2 and sys.argv[1].isdigit()
    serve(int(sys.argv[1]))
