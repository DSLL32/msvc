"""Derive native LINK state from addressed GDB bytes and real descriptor records."""
import json
from pathlib import Path
import struct
from frontier_state import Memory
from dependency_native import argument_storage


def decode(directory,phase,response,identity,pid):
    directory=Path(directory);raw=json.loads((directory/('native-'+phase+'-gdb.json')).read_bytes())
    assert raw['phase']==phase and raw['pid']==pid
    context=raw['raw_context'];symbols=context['symbols'];backing={}
    for row in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(row['data'])):
            address=row['address']+offset;assert address not in backing or backing[address]==byte
            backing[address]=byte
    read=lambda address,length:bytes(backing[address+i]for i in range(length))
    memory=Memory(read,0);word=lambda address:struct.unpack('<I',read(address,4))[0]
    scalar=lambda name:word(symbols[name])
    expected=response['parent']['phases']['spawn-before'if phase=='link-before'else'spawn-after'if phase=='link-after'else'exit-before']
    errors={name:scalar(name)for name in ('errno','doserrno','last_error')}
    assert errors==expected['errors'] and context['fctrl']==expected['x87_control_word']==639
    for name in ('fmode','commode','mb_cur_max','mb_codepage','lc_codepage'):assert scalar(name)==expected[name]
    assert list(struct.unpack('<6I',read(symbols['lc_handles'],24)))==expected['lc_handles']
    assert scalar('updates')==0 and scalar('arguments_bound')==1
    pctype=scalar('pctype_slot');assert pctype==context['initial_pctype']
    import hashlib
    assert hashlib.sha256(read(pctype-2,514)).hexdigest()==expected['ctype_sha256']
    assert hashlib.sha256(struct.pack('<I',scalar('mb_codepage'))+read(symbols['mbctype'],257)).hexdigest()==expected['mb_state_sha256']
    assert read(context['binding_identity_address'],65)==identity.encode('ascii')+b'\0'
    env=scalar('environment');initial=scalar('initial_environment')
    assert env==initial==scalar('initenv')
    environment=memory.vector(env);pointers=memory.vector_slots[env]
    assert environment==response['parent']['CRT_environment']
    os_address=scalar('os_environment');assert os_address==scalar('initial_os_environment')
    os_bytes=read(os_address,scalar('os_units')*2);assert os_bytes.hex()==response['parent']['OS_environment_hex']
    driver=scalar('driver');words=memory.words(driver,11);compiler_pass=context['compiler_passes']+4*36
    assert words[2]==scalar('active_pass')==compiler_pass and scalar('interrupted')==0
    assert scalar('pass_running')==(1 if phase in ('link-before','link-after')else 0)
    assert scalar('mre_calls')==0
    fields=memory.words(compiler_pass,9)
    assert fields[1:]==tuple(expected['pass_words'][1:])
    assert memory.string(fields[0])==response['child']['path']
    argv=None
    if phase in ('link-before','link-after'):
        address=context['argv'];strings=memory.vector(address);slots=memory.vector_slots[address]
        argv={'address':address,'pointers':slots,'strings':strings}
        assert strings==response['child']['argv']
        assert argument_storage(argv)==[pointer-expected['argument_pointers'][0]for pointer in expected['argument_pointers']]
    if phase=='link-before':
        formals={name:word(address)for name,address in context['spawn_formals'].items()}
        caller={name:word(address)for name,address in context['caller_formals'].items()}
        assert formals=={'mode':0,'file':fields[0],'argv':argv['address']}
        assert caller=={'mode':1,'pass':compiler_pass,'name':fields[0],'environment':0,'args':argv['address']}
    if phase=='link-after':assert context['eax']==0
    if phase=='link-exit':assert word(context['exit_status_address'])==0
    streams=json.loads((directory/('streams-'+phase+'.json')).read_bytes());assert streams['pid']==pid
    iob=list(struct.unpack('<24I',(directory/('streams-'+phase+'-iob.bin')).read_bytes()))
    original=expected['file0_2_words']
    for index in range(3):
        native=iob[index*8:index*8+8];target=original[index*8:index*8+8]
        assert native[1]==target[1] and native[3:]==target[3:]
        assert native[0]==native[2] and target[0]==target[2] and bool(native[0])==bool(target[0])
        assert streams['crt'][index]==streams['win32'][index]
        assert streams['crt'][index]['type']==expected['streams']['crt'][index]['type']
        assert streams['crt'][index]['position']==(expected['streams']['crt'][index]['position_low']if index else None)
        assert streams['crt'][index]['handle']==word(context['osfhnd_address']+4*index)
        assert streams['win32'][index]['handle']==word(context['standard_handles_address']+4*index)
    flags=read(context['osfile_address'],256)
    assert [index for index,flag in enumerate(flags)if flag&1]==[0,1,2]
    assert streams['osfile']==list(flags[:3])==[0xc1,0x81,0x81]
    assert streams['crt'][0]['role']=='stdin' and streams['crt'][1]['role']==streams['crt'][2]['role']=='stdout'
    assert (streams['crt'][1]['device'],streams['crt'][1]['inode'])==(streams['crt'][2]['device'],streams['crt'][2]['inode'])
    return {'pid':pid,'errors':errors,'environment':environment,'environment_address':env,'environment_pointers':pointers,
        'os_address':os_address,'pctype':pctype,'file_words':iob,'streams':{key:streams[key]for key in ('crt','win32','osfile')},'argv':argv}


def verify(directory,response,identity,pid):
    directory=Path(directory);states={phase:decode(directory,phase,response,identity,pid)for phase in ('link-before','link-after','link-postchild','link-exit')}
    before=states['link-before']
    for phase,state in states.items():
        for key in ('environment_address','environment_pointers','os_address','pctype','file_words','streams'):
            assert state[key]==before[key],'native LINK changed unobserved state: '+key
    assert states['link-after']['argv']==before['argv']
    unlink=json.loads((directory/'native-link-unlink-gdb.json').read_bytes())
    expected='.\\'+response['child']['argv'][2][3:]
    assert unlink['pid']==pid and unlink['argument_address']==unlink['caller_buffer_address']
    assert bytes.fromhex(unlink['filename_bytes'])==expected.encode('ascii')+b'\0'
    assert unlink['present_before']is True and unlink['present_after']is False and unlink['result']==0
    after={name:struct.unpack('<I',bytes.fromhex(row['bytes']))[0]for name,row in unlink['after_raw'].items()}
    assert after==states['link-postchild']['errors']==states['link-exit']['errors']
    return states
