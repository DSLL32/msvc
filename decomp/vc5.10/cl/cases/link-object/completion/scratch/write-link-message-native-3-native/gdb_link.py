"""Read real native LINK API boundaries; never set inferior state or skip calls."""
import json
from pathlib import Path
import struct
import gdb
from gdb_mktemp import errors,string
from gdb_streams import capture as streams


def observe(directory,phase,argv=None,result=None):
    directory=Path(directory);inferior=gdb.selected_inferior();ranges=[]
    value=lambda expression:int(gdb.parse_and_eval(expression))&0xffffffff
    addresses={name:value('(unsigned)&'+expression)for name,expression in {
        'fmode':'recon_fmode','commode':'recon_commode','mb_cur_max':'recon_mb_cur_max','mb_codepage':'recon_mb_codepage',
        'lc_codepage':'recon_lc_codepage','lc_handles':'recon_lc_handle','mbctype':'recon_mbctype','pctype_slot':'recon_pctype',
        'environment_slot':'recon_crt_environment','initenv_slot':'recon_crt_initenv','os_environment_slot':'recon_crt_os_environment'}.items()}
    def read(address,length):
        raw=bytes(inferior.read_memory(address,length));ranges.append({'address':address,'data':raw.hex()});return raw
    from frontier_state import Memory
    memory=Memory(read,0);word=lambda expression:value(expression)
    scalars={name:word(expression)for name,expression in {'fmode':'recon_fmode','commode':'recon_commode','mb_cur_max':'recon_mb_cur_max',
        'mb_codepage':'recon_mb_codepage','lc_codepage':'recon_lc_codepage','updates':'recon_crt_environment_updates'}.items()}
    scalars['lc_handles']=[word('recon_lc_handle[%d]'%i)for i in range(6)]
    pctype=value('(unsigned)recon_pctype');mbctype=value('(unsigned)recon_mbctype')
    scalars['mbctype_hex']=read(mbctype,257).hex();scalars['ctype_hex']=read(pctype-2,514).hex()
    env=value('(unsigned)recon_crt_environment');initial=value('(unsigned)link_initial_environment')
    os_environment=value('(unsigned)recon_crt_os_environment');units=value('recon_crt_os_environment_units')
    assert 2<=units<=65536
    environment=memory.vector(env);pointers=memory.vector_slots[env]
    os_bytes=read(os_environment,units*2)
    driver=value('(unsigned)driver');current=value('(unsigned)driver->current_pass');passes=value('(unsigned)compiler_passes')
    pass_fields=list(struct.unpack('<9I',read(current,36)))
    record={'phase':phase,'pid':inferior.pid,'errors':errors(),'x87_control_word':value('$fctrl'),'scalars':scalars,'addresses':addresses,
        'pctype':pctype,'initial_pctype':value('(unsigned)(character_flags+1)'),
        'environment_address':env,'initial_environment_address':initial,'initenv':value('(unsigned)recon_crt_initenv'),
        'environment':environment,'environment_pointers':pointers,'os_address':os_environment,
        'initial_os_address':value('(unsigned)link_initial_os_environment'),'os_environment_hex':os_bytes.hex(),
        'environment_binding_identity':gdb.parse_and_eval('link_response_identity').string(),
        'execution':{'interrupted':value('interrupted'),'pass_running':value('pass_running'),'active_pass':value('(unsigned)active_pass'),
            'driver':driver,'current_pass':current,'compiler_passes':passes},'pass_fields':pass_fields,
        'pass_name':memory.string(pass_fields[0]),'pass_environment':memory.string(pass_fields[2]),
        'ranges':ranges,'mre_calls':value('mre_call_count'),'result':result}
    if argv is not None:
        record['argv_address']=argv;record['argv']=memory.vector(argv);record['argv_pointers']=memory.vector_slots[argv]
        record['ranges']=ranges
    # A separate decoder derives the state from these addressed bytes; the
    # convenient fields above are not authoritative acceptance inputs.
    raw_symbols={name:value('(unsigned)&'+expression)for name,expression in {
        'errno':'errno','doserrno':'recon_doserrno','last_error':'last_error',
        'updates':'recon_crt_environment_updates','environment':'recon_crt_environment',
        'initial_environment':'link_initial_environment','initenv':'recon_crt_initenv',
        'os_environment':'recon_crt_os_environment','initial_os_environment':'link_initial_os_environment',
        'os_units':'recon_crt_os_environment_units','driver':'driver','interrupted':'interrupted',
        'active_pass':'active_pass','pass_running':'pass_running','mre_calls':'mre_call_count',
        'arguments_bound':'recon_crt_arguments_bound'}.items()}
    raw_symbols.update(addresses)
    for name,address in raw_symbols.items():read(address,24 if name=='lc_handles'else 4)
    read(driver,44)
    record['raw_context']={'symbols':raw_symbols,'compiler_passes':passes,
        'initial_pctype':value('(unsigned)(character_flags+1)'),
        'binding_identity_address':value('(unsigned)link_response_identity'),
        'fctrl':value('$fctrl'),'eax':value('$eax'),'argv':argv}
    read(record['raw_context']['binding_identity_address'],65)
    record['raw_context']['osfile_address']=value('(unsigned)recon_osfile')
    record['raw_context']['osfhnd_address']=value('(unsigned)recon_osfhnd')
    record['raw_context']['standard_handles_address']=value('(unsigned)standard_handles')
    read(record['raw_context']['osfile_address'],256);read(record['raw_context']['osfhnd_address'],1024);read(record['raw_context']['standard_handles_address'],12)
    record['ranges']=ranges
    path=directory/('native-'+phase+'-gdb.json');assert not path.exists(),'repeated LINK state observation'
    path.write_text(json.dumps(record,indent=2)+'\n');streams(directory,phase)


def install(directory):
    directory=Path(directory)
    assert (directory/'link-response.json').is_file()
    class SpawnReturn(gdb.FinishBreakpoint):
        def __init__(self,frame,argv):super().__init__(frame,internal=True);self.argv=argv
        def stop(self):observe(directory,'link-after',self.argv,int(self.return_value));return False
    class Spawn(gdb.Breakpoint):
        def stop(self):
            frame=gdb.newest_frame();assert frame.older().name()=='execute_pass'
            mode=int(frame.read_var('mode'));file=int(frame.read_var('file'))&0xffffffff;argv=int(frame.read_var('argv'))&0xffffffff
            observe(directory,'link-before',argv)
            path=directory/'native-link-before-gdb.json';record=json.loads(path.read_text())
            record['spawn_formals']={'mode':mode,'file':file,'path':string(file),'argv':argv}
            record['raw_context']['spawn_formals']={name:int(frame.read_var(name).address)&0xffffffff for name in ('mode','file','argv')}
            caller=frame.older()
            record['raw_context']['caller_formals']={name:int(caller.read_var(name).address)&0xffffffff for name in ('mode','pass','name','environment','args')}
            for address in list(record['raw_context']['spawn_formals'].values())+list(record['raw_context']['caller_formals'].values()):
                data=bytes(gdb.selected_inferior().read_memory(address,4));record['ranges'].append({'address':address,'data':data.hex()})
            path.write_text(json.dumps(record,indent=2)+'\n')
            SpawnReturn(frame,argv);return False
    class UnlinkReturn(gdb.FinishBreakpoint):
        def __init__(self,frame,record):super().__init__(frame,internal=True);self.record=record
        def stop(self):
            self.record.update(result=int(self.return_value),after=errors(),present_after=(directory/self.record['filename'][2:]).exists())
            self.record['after_raw']={}
            for name,expression in (('errno','errno'),('doserrno','recon_doserrno'),('last_error','last_error')):
                address=int(gdb.parse_and_eval('(unsigned)&'+expression))&0xffffffff
                self.record['after_raw'][name]={'address':address,'bytes':bytes(gdb.selected_inferior().read_memory(address,4)).hex()}
            (directory/'native-link-unlink-gdb.json').write_text(json.dumps(self.record,indent=2)+'\n');return False
    class Unlink(gdb.Breakpoint):
        def stop(self):
            frame=gdb.newest_frame();assert frame.older().name()=='delete_temporary'
            assert not(directory/'native-link-postchild-gdb.json').exists(),'extra LINK unlink'
            pointer=int(frame.read_var('name'))&0xffffffff;filename=string(pointer)
            local=int(frame.older().read_var('filename').address)&0xffffffff
            observe(directory,'link-postchild')
            record={'pid':gdb.selected_inferior().pid,'filename':filename,'argument_is_local':pointer==local,'argument_address':pointer,'caller_buffer_address':local,'filename_bytes':bytes(gdb.selected_inferior().read_memory(pointer,len(filename)+1)).hex(),'before':errors(),'present_before':(directory/filename[2:]).exists()}
            UnlinkReturn(frame,record);return False
    class Exit(gdb.Breakpoint):
        def stop(self):
            frame=gdb.newest_frame();assert frame.older().name()=='driver_exit'
            observe(directory,'link-exit',result=int(frame.read_var('code'))&0xffffffff)
            path=directory/'native-link-exit-gdb.json';record=json.loads(path.read_text());address=int(frame.read_var('code').address)&0xffffffff
            record['raw_context']['exit_status_address']=address;record['ranges'].append({'address':address,'data':bytes(gdb.selected_inferior().read_memory(address,4)).hex()});path.write_text(json.dumps(record,indent=2)+'\n');return False
    Spawn('recon_spawnvp',internal=True);Unlink('recon_unlink',internal=True);Exit('recon_exit',internal=True)
