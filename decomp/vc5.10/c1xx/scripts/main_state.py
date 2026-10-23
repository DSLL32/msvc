"""Independent main entry, typed initialized defaults/restore, and first callee."""
import json
import re
import struct

DEFAULT_SYSTEM=[0,4096,0,0,0,0,0,65536,0,0]
INITIALIZED=('compiler_phase','option_c9il','compiler_flags','system_info')
COMMAND=('break_into_debugger','command_argc','command_argv')


def expected(owned,which):
    if which=='saved':return [23,29,0x55667788,[0x11110007,8192,owned+4,owned+8,3,2,586,131072,6,1234]]
    if which=='live':return [17,19,0x11223344,[0x22220003,12288,owned+12,owned+16,7,4,486,196608,5,4321]]
    return [0,0,0,DEFAULT_SYSTEM]


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    def raw(suffix):
        blob=(original/('main-fields-'+suffix+'.bin')).read_bytes();s.require(len(blob)==60,'incomplete actual main-field projection');words=list(struct.unpack('<3I8I2H3I',blob));return words[:3]+[words[3:13]],words[13:]
    def values(snapshot):return [snapshot['frontend'][name] for name in INITIALIZED]
    def command(snapshot):return [snapshot['frontend'][name] for name in COMMAND]
    def saved(snapshot):return [snapshot['frontend']['saved_'+name] for name in INITIALIZED]
    old_owned=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0];new_owned=pre['frontend']['saved_state']
    s.require(raw('000')==(expected(0,'default'),[0,0,0]),'actual original main-field defaults differ')
    s.require(raw('006')==(expected(old_owned,'live' if seed else 'default'),[17,23,old_owned+12] if seed else [0,0,0]) and values(pre)==expected(new_owned,'live' if seed else 'default') and command(pre)==([17,23,new_owned+12] if seed else [0,0,0]),'nonzero live/main command input differs')
    s.require(saved(pre)==(expected(new_owned,'saved') if seed else [None]*4),'saved typed main input differs before restore')
    if seed:
        blob=(original/'frontend-snapshot-006.bin').read_bytes();old_saved=[struct.unpack_from('<I',blob,offset)[0] for offset in (0x15c0,0x1488,0xf7c)]+[list(struct.unpack_from('<8I2H',blob,0x17b8))]
        s.require(old_saved==expected(old_owned,'saved'),'original saved typed main input differs before restore')
    for suffix,snapshot in [('phase',phase)]:
        owned=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0];now=snapshot['frontend']['saved_state']
        s.require(raw(suffix)==(expected(owned,'saved' if seed else 'default'),[0,0,0]) and values(snapshot)==expected(now,'saved' if seed else 'default') and command(snapshot)==[0,0,0],'new initialized fields escaped typed restore or command BSS reset')
        blob=(original/('frontend-snapshot-'+suffix+'.bin')).read_bytes();old_saved=[struct.unpack_from('<I',blob,offset)[0] for offset in (0x15c0,0x1488,0xf7c)]+[list(struct.unpack_from('<8I2H',blob,0x17b8))]
        s.require(old_saved==expected(owned,'saved' if seed else 'default') and saved(snapshot)==expected(now,'saved' if seed else 'default'),'saved typed main-field projection differs')
    entry=json.loads((native/'main-entry.json').read_text());rows=[r for r in traps if r[1]=='00038bfd'];s.require(len(rows)==1,'missing independently observed main entry')
    suffix='%03d'%int(rows[0][0]);old_entry,_=raw(suffix);owner=entry['frontend']['saved_state'];old_owner=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0]
    s.require(old_entry==expected(old_owner,'saved' if seed else 'default') and values(entry)==expected(owner,'saved' if seed else 'default') and raw(suffix)[1]==command(entry)==[0,0,0],'main entry did not preserve actual initialized/restored state')
    # The raw vector's address is the first word; full vector identity is checked by caller.
    pointer=struct.unpack_from('<I',(original/'invoke-entry-argv.bin').read_bytes())[0];native_pointer=entry['invocation']['argv_address'];argc=26
    start,size=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] c1xx_main$',symbols,re.M).groups()]
    invoke,isize=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] InvokeCompilerPass$',symbols,re.M).groups()]
    abi=entry['raw_entry_abi'];raw_args=[int(v,16) for v in rows[0][2].split(',')]
    s.require(entry['symbol']=='c1xx_main' and entry['pc']==start and entry['args']=={'argc':argc,'argv':native_pointer} and abi['stack_arguments']==[argc,native_pointer] and abi['esp']==entry['esp'] and invoke<abi['return_pc']<invoke+isize and raw_args[:3]==[0x1043851f,argc,pointer],'actual main entry cdecl(argc,argv) or real caller differs')
    s.require(entry['target_state']['compiler_mode']==1 and struct.unpack_from('<I',(original/('state-'+suffix+'.bin')).read_bytes(),8)[0]==1,'Invoke mode assignment before main differs')
    helper_rows=[r for r in traps if r[1]=='00038c86'];s.require(len(helper_rows)==1,'missing actual first work entry')
    helper_row=helper_rows[0];helper_suffix='%03d'%int(helper_row[0]);helper=state if state['symbol']=='initialize_work_buffers' else json.loads((native/'work-entry.json').read_text())
    before=(original/('frontend-data-'+suffix+'.bin')).read_bytes();after=(original/('frontend-data-'+helper_suffix+'.bin')).read_bytes();phase_offset=0x15c0
    s.require(len(before)==len(after)==8520 and before[:phase_offset]==after[:phase_offset] and before[phase_offset+4:]==after[phase_offset+4:] and struct.unpack_from('<I',after,phase_offset)[0]==1,'main changed unknown initialized bytes or failed phase write')
    old_bss=(original/'main-bss-entry.bin').read_bytes();new_bss=(original/'main-bss-frontier.bin').read_bytes();s.require(len(old_bss)==len(new_bss)==0xf69c and old_bss==new_bss,'main changed unrelated original BSS before first helper')
    old=dict(entry['frontend']);new=dict(helper['frontend']);old.pop('compiler_phase');new.pop('compiler_phase');s.require(old==new and helper['frontend']['compiler_phase']==1,'main changed other native modeled state before helper')
    old_final,command_final=raw(helper_suffix);want=expected(old_owner,'saved' if seed else 'default');want[0]=1;s.require(old_final==want and command_final==[0,0,0],'main helper frontier field projection differs')
    s.require(helper_row[1]=='00038c86' and int(helper_row[2].split(',')[0],16)==0x10438c0e and helper['symbol']=='initialize_work_buffers' and helper['args']=={} and start<helper['caller_return_pc']<start+size,'genuine first helper/cdecl(void) boundary differs')
    for snapshot in (entry,helper):s.require(snapshot['target_state']['last_error']==203 and snapshot['control_word']==0x27f,'main changed caller state before helper')
