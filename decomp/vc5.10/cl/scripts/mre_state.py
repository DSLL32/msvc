"""Derive independent native MRE state/ABI from read-only addressed GDB data."""
import json,os,struct
from pathlib import Path
from clock_state import compare_fp
from mre_native import decode,Provider,configuration


class Memory:
    def __init__(self,ranges):
        self.bytes={}
        for record in ranges:
            address=record['address'];raw=bytes.fromhex(record['bytes'])
            assert type(address)is int and 0<address<=0xffffffff and len(raw)<=262144 and address+len(raw)<=0x100000000
            for offset,byte in enumerate(raw):
                key=address+offset
                assert key not in self.bytes or self.bytes[key]==byte,'contradictory GDB raw ranges'
                self.bytes[key]=byte
    def read(self,address,count):return bytes(self.bytes[address+i]for i in range(count))
    def word(self,address):return struct.unpack('<I',self.read(address,4))[0]
    def words(self,address,count):return list(struct.unpack('<%dI'%count,self.read(address,4*count)))
    def text(self,address):
        if not address:return None
        result=bytearray()
        for i in range(32768):
            byte=self.read(address+i,1)
            if byte==b'\0':return result.decode('latin1')
            result.extend(byte)
        raise AssertionError('unbounded addressed GDB string')
    def vector(self,address):
        pointers=[];strings=[]
        for i in range(4096):
            pointer=self.word(address+4*i)
            if not pointer:break
            pointers.append(pointer);strings.append(self.text(pointer))
        else:raise AssertionError('unbounded addressed GDB vector')
        return {'address':address,'pointers':pointers,'strings':strings}


def state(record):
    memory=Memory(record['ranges']);symbols=record['symbols'];data=record['data_addresses']
    scalar=lambda name:memory.word(symbols[name])
    fields={name:scalar(symbol)for name,symbol in{
        'errno':'errno','doserrno':'recon_doserrno','last_error':'last_error',
        'fmode':'recon_fmode','commode':'recon_commode','mb_cur_max':'recon_mb_cur_max',
        'mb_codepage':'recon_mb_codepage','lc_codepage':'recon_lc_codepage',
        'minimal_rebuild':'minimal_rebuild','reproducible':'reproducible','first_dependency_scan':'first_dependency_scan',
        'interrupted':'interrupted','pass_running':'pass_running'}.items()}
    fields['lc_handles']=memory.words(symbols['recon_lc_handle'],6)
    fields['mbctype']=memory.read(data['recon_mbctype'],257).hex()
    fields['pctype']=scalar('recon_pctype');fields['ctype']=memory.read(fields['pctype']-2,514).hex()
    fields['environment']=memory.vector(scalar('recon_crt_environment'));fields['initial_environment']=memory.vector(scalar('recon_crt_initenv'))
    address=scalar('recon_crt_os_environment');units=scalar('recon_crt_os_environment_units')
    assert address and 2<=units<=65536
    fields['os_environment']={'address':address,'units':units,'updates':scalar('recon_crt_environment_updates'),'bytes':memory.read(address,2*units).hex()}
    fields['file_words']=memory.words(data['recon_iob'],24)
    fields['descriptors']=[i for i in range(256)if memory.read(data['recon_osfile']+i,1)[0]&1]
    fields['fd3_flags']=memory.read(data['recon_osfile']+3,1)[0]
    fields['driver']=scalar('driver');fields['driver_words']=memory.words(fields['driver'],11)
    fields['scratch']=memory.read(data['option_buffer'],1024).hex();fields['options']=[]
    pointer=fields['driver_words'][0]
    for i in range(4096):
        if not pointer:break
        words=memory.words(pointer,6)
        fields['options'].append({'address':pointer,'words':words,**{name:{'address':words[slot],'text':memory.text(words[slot])}for slot,name in((2,'matching'),(3,'name'),(4,'value'))}})
        pointer=words[0]
    else:raise AssertionError('cyclic addressed option list')
    pointer=scalar('pdb_name');fields['pdb']={'address':pointer,'text':memory.text(pointer)}
    return fields,memory


def verify(original,directory,provider=None,signature_candidate=False,selector_candidate=False):
    directory=Path(directory);raw=json.loads((directory/'mre-native.json').read_bytes())
    own_provider=provider is None
    if own_provider:
        # Pure offline verifier: no socket/thread or configuration generation.
        from mre_native import Pointers,sha,encoded
        from mre_object_packet import decode as original_packet
        from case_inputs import load
        provider=Provider.__new__(Provider);provider.original=Path(original).resolve();provider.directory=directory.resolve()
        retention=directory/'native-retention.json'
        if retention.exists():
            import hashlib
            index=json.loads(retention.read_bytes())
            assert index['schema']==1 and index['native_receipt_sha256']==hashlib.sha256((directory/'native-prefix.json').read_bytes()).hexdigest()
            for name,item in index['files'].items():
                relative=Path(name);assert not relative.is_absolute()and '..'not in relative.parts
                path=directory/relative;assert path.is_file()and not path.is_symlink()
                value=path.read_bytes();assert item=={'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()},'retained native payload changed'
            execution=Path(index['execution_directory'])
            assert execution.is_absolute()and execution.parent==Path('/root/.ai-tmp/msvc2/vc5.10/cl')and execution.name.startswith('mre-provider-native-')
            provider.directory=execution;provider.artifact_directory=directory.resolve()
        if selector_candidate:
            from mre_selector_packet import decode as selector_response
            provider.packet=selector_response(provider.original)
        elif signature_candidate:
            from mre_signature_packet import decode as signature_response
            provider.packet=signature_response(provider.original)
        else:provider.packet=original_packet(provider.original)
        provider.pointer=Pointers();provider.inputs=load('mre-object')['inputs']
        provider.config_path=directory/'mre-binding.bin';provider.config=provider.config_path.read_bytes()
        provider.idb=(provider.original/provider.packet['intermediate_files'][0]['owned_contents']).read_bytes()
        provider.os_block=(provider.original/'observer/mre-1-before-os-environment.bin').read_bytes()
        provider.native_before=provider.native_after=None
        assert raw['response_sha256']==sha(encoded(provider.packet))and raw['config_sha256']==sha(provider.config)
        assert raw['original_capture_sha256']==provider.packet['capture_sha256']
        from dependency_packet import vector
        unused,environment=vector((provider.original/'environment.bin').read_bytes())
        assert provider.config==configuration(raw['response_sha256'],[value.decode('latin1')for pointer,value in environment],provider.os_block,provider.idb),'different actual MRE binding bytes'
    entry=json.loads((directory/'native-entry-errors.json').read_bytes())
    assert entry==provider.packet['entry_profile'],'different native main-entry profile'
    temporary=json.loads((directory/'native-mktemp-gdb.json').read_bytes())
    actual=temporary['calls'];expected=provider.packet['entry_and_mktemp']['calls']
    assert len(actual)==len(expected)==1
    for name in('template','result','before','after'):assert actual[0][name]==expected[0][name],'native mktemp contract differs: '+name
    assert actual[0]['result_alias']is True
    assert raw['errors']==[]and [r['kind']for r in raw['requests']]==list(range(7))
    from mre_elf import Layout
    execution=directory/'native-execution.json'
    report=json.loads((execution if execution.exists()else directory/'native-prefix.json').read_bytes())
    binary=directory/'native-CL.EXE'
    if not binary.exists():binary=Path(__file__).resolve().parents[1]/'build/native-trace/CL.EXE'
    layout=Layout(binary,report['native_sha256'])
    pid=raw['pid'];assert all(r['pid']==pid for r in raw['requests'])
    assert bytes.fromhex(raw['requests'][0]['payload_hex'])==provider.config
    previous=None;observations=[]
    for kind in range(1,7):
        index=(kind-1)//2;after=not(kind&1);phase='mre-%d-%s'%(index+1,'after'if after else'before')
        record=json.loads((directory/('native-'+phase+'-gdb.json')).read_bytes())
        assert record['phase']==phase and record['pid']==pid
        layout.bind(record)
        derived,memory=state(record);request=decode(bytes.fromhex(raw['requests'][kind]['payload_hex']))
        # Actual bytes read at the real function boundary must independently
        # agree with all corresponding fields the response host reported.
        for name,value in derived.items():assert value==request[name],'GDB disagrees with MRE request: '+name
        registers=record['general'];arguments=record['arguments'];argc=(6,2,1)[index]
        assert memory.words(arguments['stack'],argc+1)==[arguments['return'],*arguments['values']]
        if after:
            assert registers['eip']==arguments['return']and registers['esp']==arguments['stack']+4*(argc+1)
            before=observations[-1]
            assert before['arguments']==arguments
            assert all(before['general'][name]==registers[name]for name in('ebx','esi','edi','ebp')),'native MRE stdcall nonvolatile changed'
            assert before['x87']==record['x87'],'native MRE changed full live x87 state'
            if index==0:assert registers['eax']==1
            if index==1:assert registers['eax']==request['driver_value'] # ignored semantic word, actual alias only
        else:
            assert registers['esp']==arguments['stack']
            layout.entry(record,index)
        args=arguments['values']
        if index==0:
            assert args==[request['owner_out'],request['path']['address'],request['error_out'],request['message'],0,1]
            assert memory.text(args[1])=='case.idb'and memory.read(args[3],1024).hex()==request['scratch']
            assert args[3]==record['data_addresses']['option_buffer'],'MRE message is not shared program scratch'
            if after:assert memory.word(args[0])==request['owner_value']and memory.word(args[2])==0
        elif index==1:
            assert args==[request['engine'],request['driver_out']]
            if after:assert memory.word(args[1])==request['driver_value']
        else:assert args==[request['mre_driver']]
        original_context=(provider.original/('observer/'+phase+'-context.bin')).read_bytes()
        compare_fp(original_context[28:140].hex(),record['x87'])
        assert memory.word(record['symbols']['mre_call_count'])==(kind//2)
        streams=json.loads((directory/('streams-'+phase+'.json')).read_bytes())
        assert streams['pid']==pid and streams['mre_calls']==kind//2
        assert memory.read(record['data_addresses']['recon_iob'],96)==(directory/('streams-'+phase+'-iob.bin')).read_bytes()
        for i,observed in enumerate(request['streams']):
            assert streams['osfile'][i]==observed['osfile']
            for family in('crt','win32'):
                expected=dict(observed[family]);expected['position']=None if expected['type']==2 else expected['position']
                assert {k:streams[family][i][k]for k in expected}==expected
                table='recon_osfhnd'if family=='crt'else'standard_handles'
                assert memory.word(record['data_addresses'][table]+4*i)==expected['handle']
        fd=record['descriptor3']
        if kind==1:assert fd is None
        else:
            expected=request['fd3'];handle=expected['handle']
            assert fd=={'handle':handle['handle'],'device':handle['device'],'inode':handle['inode'],
                'size':25600,'position':1024,'flags':os.O_CLOEXEC|os.O_RDWR}
            assert memory.word(record['data_addresses']['recon_osfhnd']+12)==fd['handle']
        if signature_candidate or selector_candidate:
            wanted=provider.packet['target_objects'][phase]['include_input']
            assert memory.word(record['symbols']['use_standard_include'])==wanted['use_standard_include']
            assert memory.text(derived['driver_words'][9])==wanted['text']
        if own_provider:
            # Absence was asserted live before Open. Offline state contains the
            # created output; do not pretend it is the earlier filesystem.
            provider.validate(kind,request,live=False)
        observations.append(record);previous=request
    if selector_candidate:
        from selector_native_state import verify_tail
        return verify_tail(provider,directory,layout,pid,observations,previous)
    record=json.loads((directory/'native-mre-frontier-gdb.json').read_bytes())
    assert record['phase']=='mre-frontier'and record['pid']==pid
    caller_authentication=layout.frontier(record,signature_candidate)
    frontier,memory=state(record);arguments=record['arguments']
    assert arguments['state']==frontier['driver']==memory.word(arguments['slot'])
    if signature_candidate:
        ebp=record['general']['ebp'];signature_ebp=arguments['signature_ebp']
        assert memory.word(ebp)==signature_ebp
        selector_return=memory.word(ebp+4);layout.call(selector_return,'option_matching_selector','option_signature')
        assert arguments['option_slot']==ebp+8 and arguments['selectors_slot']==ebp+12
        assert memory.word(arguments['option_slot'])==arguments['option']==frontier['driver_words'][0]
        assert memory.word(arguments['selectors_slot'])==arguments['selectors']and memory.text(arguments['selectors'])=='Mm'
        assert arguments['slot']==signature_ebp+8
        assert memory.word(signature_ebp)==observations[-1]['general']['ebp']
        layout.call(memory.word(signature_ebp+4),'option_signature','prepare_jobs')
        wanted=provider.packet['target_objects']['mre-frontier']['include_input']
        assert memory.word(record['symbols']['use_standard_include'])==wanted['use_standard_include']
        assert memory.text(frontier['driver_words'][9])==wanted['text']
    else:assert arguments['caller']=='prepare_jobs'
    assert memory.read(arguments['pending'],20)==bytes(20)
    assert memory.word(record['symbols']['mre_call_count'])==3
    expected=list(previous['driver_words']);expected[5]=0;expected[6]=frontier['driver']+20
    assert frontier['driver_words']==expected,'wrong live MRE signature deferred-list state'
    for name,value in frontier.items():
        if name!='driver_words':assert value==previous[name],'changed post-MRE frontier '+name
    original_context=(provider.original/'observer/mre-frontier-context.bin').read_bytes()
    compare_fp(original_context[28:140].hex(),record['x87'])
    assert record['x87']==observations[-1]['x87']and record['descriptor3']==observations[-1]['descriptor3']
    assert (directory/'trace.txt').read_bytes()==(provider.original/'trace.txt').read_bytes()
    assert (directory/'stdout.bin').read_bytes()==b''and(directory/'stderr.bin').read_bytes()==(provider.original/'stderr.bin').read_bytes()
    assert (directory/'case.idb').read_bytes()==provider.idb
    return {'case':'mre-object','scope':'initial prefix, no acceptance/normal close credit','calls':[1,1,1],
            'frontier':'option_matching_selector'if signature_candidate else'option_signature','native_pid':pid,'actual_native_ABI_state_and_file_match':True,'frontier_caller_authentication':caller_authentication}
