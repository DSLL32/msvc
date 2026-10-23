"""Two encountered byte workspaces and genuine first reserve arguments."""
import json
import re
import struct


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    def raw(suffix):
        blob=(original/('work-buffers-'+suffix+'.bin')).read_bytes();s.require(len(blob)==16,'missing four actual work pointers');return list(struct.unpack('<4I',blob))
    def values(snapshot):return sum(snapshot['frontend']['work_buffers'],[])
    old_owned=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0];new_owned=pre['frontend']['saved_state'];offsets=(100,200,300,400)
    s.require(raw('000')==[0]*4,'work buffer loaded defaults differ')
    s.require(raw('006')==([old_owned+i for i in offsets] if seed else [0]*4) and values(pre)==([new_owned+i for i in offsets] if seed else [0]*4),'work pointer nonzero BSS input identities differ')
    s.require(raw('phase')==values(phase)==[0]*4,'work pointers escaped BSS reset')
    entry=json.loads((native/'work-entry.json').read_text());rows=[r for r in traps if r[1]=='00038c86'];s.require(len(rows)==1,'missing actual work entry')
    suffix='%03d'%int(rows[0][0]);start,size=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_work_buffers$',symbols,re.M).groups()]
    s.require(entry['symbol']=='initialize_work_buffers' and entry['pc']==start and entry['args']=={} and raw(suffix)==values(entry)==[0]*4,'not actual cdecl(void) work entry or pointer reset differs')
    reserve_row=next(row for row in traps if row[1]=='00038cc9');reserve_suffix='%03d'%int(reserve_row[0]);reserve=state if '_reserve_work_buffer' not in trial['accepted'] else json.loads((native/'reserve-entry-01.json').read_text())
    s.require(reserve_row[1]=='00038cc9' and [int(v,16) for v in reserve_row[2].split(',')][:3]==[0x10438c95,0x500000,0x4000] and reserve['symbol']=='reserve_work_buffer' and reserve['args']=={'reserve_bytes':0x500000,'commit_bytes':0x4000},'reserve cdecl(reserve,commit) arguments or actual caller differs')
    s.require(raw(reserve_suffix)==values(reserve)==[0]*4 and entry['frontend']==reserve['frontend'],'work initializer stored state before first real reserve call')
    before=(original/'main-bss-frontier.bin').read_bytes();after=(original/'work-bss-first-reserve.bin').read_bytes();s.require(len(before)==len(after)==0xf69c and before==after,'work initializer changed original BSS before allocation')
    s.require((original/('frontend-data-'+suffix+'.bin')).read_bytes()==(original/('frontend-data-'+reserve_suffix+'.bin')).read_bytes(),'work initializer changed original initialized storage')
    for snapshot in (entry,reserve):s.require(snapshot['target_state']['last_error']==203 and snapshot['target_state']['compiler_mode']==1 and snapshot['control_word']==0x27f,'work initializer changed caller state')
