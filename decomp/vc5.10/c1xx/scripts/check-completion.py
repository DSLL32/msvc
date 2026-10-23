"""Require actual attach/export-presence/detach completion and typed return state."""
import importlib.util
import json
from pathlib import Path
import re

spec=importlib.util.spec_from_file_location('support',Path(__file__).with_name('trial-support.py'))
s=importlib.util.module_from_spec(spec)
spec.loader.exec_module(s)


def native_states(records):
    s.require(len(records)==2,'missing native lifecycle return')
    first,last=records
    for reason,record in zip((1,0),records):
        s.require(record['reason']==reason and record['status']==1,'native lifecycle return differs')
        s.require(record['module']!=0 and record['module']==record['dll_module']==first['module'],
                  'native target module identity changed')
        s.require(record['registration']==first['registration'],'native registered provenance changed')
        s.require(record['process_attached']==reason and record['raw_dll_entry']==record['dll_adjust_fdiv']==0,
                  'native CRT scalar cleanup differs')
        s.require(record['initializer_words']==[0,0] and record['onexit_first']==0,
                  'native initializer/onexit content differs')
        s.require(record['thread_calls_disabled']==1 and record['last_error']==203 and
                  record['floating_control']==0x9001f,'native loader notification/LastError/x87 state differs')
    s.require(first['onexit_begin']!=0 and first['onexit_end']==first['onexit_begin'] and
              last['onexit_begin']==0 and last['onexit_end']==first['onexit_begin'],
              'native freed-onexit begin/end identity differs')
    return {'returns':[1,1],'attach_count':[1,0],'module':'registered DLL',
            'onexit':['allocated empty 128-byte table','begin NULL; end retains freed allocation identity'],
            'initializer_words':[0,0],'raw_callback':None,'adjust_fdiv':0,
            'last_error':203,'x87_abstract_control':0x9001f,'thread_calls_disabled':True}


def check(original,native,require_audit=True,audit_path=None):
    o,n,trial,text,state=s.pair(original,native)
    s.require(trial['trial']=='_DllMain@12' and o['exit']==n['exit']==0 and
              n['kind']=='native-completion','not actual completion on both sides')
    s.require('frontier ' not in text and 'exception ' not in text and
              re.findall(r'^exit=.*$',text,re.M)==['exit=0 target=1 crt=1 initialized=1'],
              'original masked run did not finish successfully')
    traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    expected=[('0','0002e3c6'),('1','0002e46a'),('2','000384ce'),('3','0002e52a'),
              ('5','0002e3c6'),('6','0002e52a'),('7','0002e46a')]
    s.require([row[:2] for row in traps]==expected,'original attach/detach call order changed or export invoked')
    returns=re.findall(r'^crt-return seq=(\d+) reason=(\d+) status=(\d+) pc=([0-9a-f]{8})$',text,re.M)
    s.require(len(returns)==2 and [row[:3] for row in returns]==[('4','1','1'),('8','0','1')] and
              returns[0][3]==returns[1][3] and int(returns[0][3],16)!=0,
              'actual loader caller-return observations differ')
    for sequence,rva,stacktext in traps:
        stack=[int(word,16) for word in stacktext.split(',')]
        expected_args=[0x104cb010,0x104cb014] if rva=='000384ce' else [0x10400000,int(sequence)<5,0]
        s.require(stack[1:1+len(expected_args)]==expected_args,'raw original boundary arguments changed')
    old=[s.original_state(original,text,seq) for seq in range(9)]
    for seq,record in enumerate(old):
        s.require(record['raw_dll_entry']==record['dll_adjust_fdiv']==0 and record['initializer_words']==[0,0],
                  'original callback/initializer/imported scalar state changed')
        s.require(record['dll_module']==(0 if seq<=3 else 0x10400000),'original target module store differs')
        expected_count=0 if seq in (0,1,2,8) else 1
        s.require(record['process_attached']==expected_count,'original CRT attach count differs')
        if seq in (0,1,8):
            s.require(record['onexit_begin']==0,'original allocation lifetime differs')
        else:
            s.require(record['onexit_begin']==old[2]['onexit_begin']!=0 and record['onexit_first']==0 and
                      record['onexit_end']==record['onexit_begin'],'original initialized onexit allocation differs')
    s.require(old[0]['onexit_end']==old[1]['onexit_end']==0 and
              old[8]['onexit_end']==old[2]['onexit_begin'],'original dangling end identity changed')
    s.require(re.findall(r'^last-error=(\d+)$',text,re.M)==['203']*9,'original loader LastError changed')
    words=[int(word,16)&0xffff for word in re.findall(r'^control-word=([0-9a-f]+)$',text,re.M)]
    s.require(words==[0x27f]*9,'original x87 caller state changed')
    providers=re.findall(r'^provider iat=([0-9a-f]{8}) resolved=([0-9a-f]{8})$',text,re.M)
    s.require(providers==[('000c9114','78001715'),('000c90a8','78001498'),('000c9140','7800138a'),
                          ('000c9118','7803f928')] and 'provider-adjust-fdiv=0\n' in text,
              'canonical provider identity changed')
    records=[json.loads(line) for line in (native/'lifecycle-state.jsonl').read_text().splitlines()]
    s.require(state['checkpoints']==records and state['registered_token']==records[0]['module'] and
              state['registration']==records[0]['registration'],'derived native state differs from raw host observations')
    normalized=native_states(records)
    trace=[]
    for _,rva,_ in traps:
        if rva=='0002e52a' and 'DllMain' not in trace: trace.append('DllMain')
    s.require((native/'trace.txt').read_text()==''.join(name+'\n' for name in trace)=='DllMain\n',
              'native application entry trace differs from original')
    s.require((original/'stdout.bin').read_bytes()==b'attach=1\r\nexports=1\r\ndetach=1\r\n' and
              (original/'stderr.bin').read_bytes()==b'','case concrete output differs')
    if require_audit:s.audit(trial,audit_path)
    return {'trial':trial['trial'],'frontier':'completed lifecycle','completion':True,'typed_state':normalized,
            'application_trace':trace,'whole_mask_bytes_checked':751216,'audit_checked':require_audit,
            'original_reused':o['native_sha256']!=n['native_sha256'],
            'original_seconds':o['seconds'],'native_seconds':n['seconds']}
