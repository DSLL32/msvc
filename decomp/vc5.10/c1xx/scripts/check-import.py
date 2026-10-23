"""Decode the executed canonical initializer import and real DllMain frontier."""
import importlib.util
from pathlib import Path
import re
import subprocess

spec = importlib.util.spec_from_file_location('support', Path(__file__).with_name('trial-support.py'))
s = importlib.util.module_from_spec(spec)
spec.loader.exec_module(s)


def check(original, native, require_audit=True, audit_path=None):
    o,n,trial,text,state = s.pair(original,native)
    s.require(trial['trial']=='import:_initterm' and o['exit']==125, 'wrong import frontier trial')
    traps = re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    s.require([row[:2] for row in traps]==[('0','0002e3c6'),('1','0002e46a'),('2','000384ce'),('3','0002e52a')]
              and re.findall(r'^frontier rva=(\w+)$',text,re.M)==['0002e52a'], 'raw DllMain frontier missing')
    old = s.original_state(original,text,3)
    current = state['target_state']
    s.require(old['dll_module']==0, 'application module store ran before frontier')
    for values in (old,current):
        s.require(values['process_attached']==1 and values['raw_dll_entry']==values['dll_adjust_fdiv']==0,
                  'CRT initialization state differs at DllMain')
        s.require(values['initializer_words']==[0,0] and values['onexit_begin']!=0 and
                  values['onexit_end']==values['onexit_begin'] and values['onexit_first']==0,
                  'CRT heap/callback identity differs at DllMain')
    stack=[int(word,16) for word in traps[-1][2].split(',')]
    s.require(stack[1:4]==[0x10400000,1,0] and state['symbol']=='DllMain' and state['args']=={
        'module':state['registered_token'],'reason':1,'reserved':0}, 'stdcall DllMain inputs differ')
    s.require(re.findall(r'^last-error=(\d+)$',text,re.M)==['203']*4 and current['last_error']==203,
              'loader LastError changed')
    words=[int(word,16)&0xffff for word in re.findall(r'^control-word=([0-9a-f]+)$',text,re.M)]
    s.require(words==[0x27f]*4 and state['control_word']==0x27f,'caller x87 context changed')
    transcript=(native/'gdb.txt').read_text()
    s.require('Program received signal SIGTRAP' in transcript and 'in DllMain (' in transcript,
              'not a real native application frontier')
    binary=s.PROJECT/'build/native/c1xx-host'
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
    match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] DllMain$',symbols,re.M)
    s.require(match is not None,'missing ELF DllMain')
    first,size=[int(word,16) for word in match.groups()]
    s.require(first<state['pc']<=first+size,'native frontier outside DllMain')
    code=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),
                                 '--stop-address='+str(state['pc']),str(binary)],text=True)
    s.require('int3' in code,'native frontier not INT3')
    if require_audit: s.audit(trial,audit_path)
    return {'trial':trial['trial'],'frontier':'_DllMain@12','raw_frontier_rva':0x2e52a,
            'classification':'genuine linker-generated import support',
            'typed_state':{'attach_count':1,'raw_callback':None,'onexit':'allocated empty 128-byte table',
                           'initializer_words':[0,0],'adjust_fdiv':0,'last_error':203,'control_word':0x27f},
            'whole_mask_bytes_checked':751216,'audit_checked':require_audit,
            'original_reused':o['native_sha256']!=n['native_sha256']}
