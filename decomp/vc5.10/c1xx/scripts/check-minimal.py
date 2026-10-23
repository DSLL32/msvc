"""Typed first real Invoke trial, preserving the captured CL DLL ABI."""
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess

spec=importlib.util.spec_from_file_location('support',Path(__file__).with_name('trial-support.py'))
s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)


def vector(path):
    raw=path.read_bytes()
    if len(raw)<8:raise ValueError('missing vector header')
    address,count=struct.unpack_from('<II',raw);cursor=8;strings=[]
    s.require(address!=0 and count<=512,'invalid vector pointer/count')
    for _ in range(count):
        pointer,length=struct.unpack_from('<II',raw,cursor);cursor+=8
        value=raw[cursor:cursor+length];cursor+=length
        s.require(pointer and 0<length<=32768 and len(value)==length and value[-1:]==b'\0' and
                  b'\0' not in value[:-1],'invalid raw argv element')
        strings.append(value[:-1].decode('latin1'))
    s.require(cursor==len(raw),'extra/truncated vector bytes')
    return address,strings


def check(original,native,require_audit=True,audit_path=None):
    o,n,trial,text,state=s.pair(original,native)
    s.require(trial['case']=='minimal' and trial['trial']=='_InvokeCompilerPass@12' and
              o['exit']==125 and n['kind']=='native-gdb-frontier','wrong real Invoke trial')
    contract=json.loads((native/'contract.json').read_text())
    s.require(contract['argc']==26 and contract['mode']==1 and contract['return']==0 and
              contract['target_sha256']==trial['target']['sha256'],'not the captured successful CL contract')
    traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    s.require([row[:2] for row in traps]==[('0','0002e3c6'),('1','0002e46a'),('2','000384ce'),
              ('3','0002e52a'),('5','000384d4'),('6','00038550')] and
              re.findall(r'^frontier rva=(\w+)$',text,re.M)==['00038550'],
              'raw Invoke did not advance exactly to initializer')
    pointer,argv=vector(original/'invoke-entry-argv.bin')
    native_pointer,native_argv=vector(native/'native-argv.bin')
    _,seed_argv=vector(native/'invoke-argv.bin')
    s.require(argv==native_argv==seed_argv==contract['original_argv'],'actual DLL argument strings differ')
    stack=[int(word,16) for word in traps[-2][2].split(',')]
    s.require(stack[1:4]==[len(argv),pointer,1],'original raw stdcall argc/argv/BOOL differs')
    s.require(state['invocation']=={'argc':len(argv),'mode':1,'argv_address':native_pointer,'argv':argv},
              'native actual Invoke frame/vector differs')
    context=(original/'invoke-entry-context.bin').read_bytes()
    s.require(len(context)==204 and struct.unpack_from('<I',context)[0]&8 and
              struct.unpack_from('<I',context,184)[0]==0x104384d5 and
              struct.unpack_from('<I',context,196)[0]==int(re.search(r'^trap seq=5 .*esp=(\w+)',text,re.M)[1],16),
              'raw original entry CONTEXT differs')
    old=s.original_state(original,text,6);current=state['target_state']
    s.require(struct.unpack_from('<I',(original/'state-006.bin').read_bytes(),8)[0]==0 and
              current['compiler_mode']==0,'Invoke stored mode before the initializer')
    for values in (old,current):
        s.require(values['process_attached']==1 and values['raw_dll_entry']==values['dll_adjust_fdiv']==0 and
                  values['initializer_words']==[0,0] and values['onexit_begin']!=0 and
                  values['onexit_end']==values['onexit_begin'] and values['onexit_first']==0,
                  'typed lifecycle state differs at initializer')
    s.require(old['dll_module']==0x10400000 and current['dll_module']==state['registered_token'],
              'registered module identity differs')
    s.require(re.findall(r'^last-error=(\d+)$',text,re.M)==['203']*7 and current['last_error']==203 and
              [int(word,16)&0xffff for word in re.findall(r'^control-word=(\w+)$',text,re.M)]==[0x27f]*7 and
              state['control_word']==0x27f,'caller LastError/x87 state differs')
    s.require(state['symbol']=='initialize_compiler_pass' and state['args']=={} and
              'Program received signal SIGTRAP' in (native/'gdb.txt').read_text(),
              'native frontier not a real cdecl initializer trap')
    elf=native/'native-executable.elf'
    s.require(s.sha(elf)==n['native_sha256'],'captured ELF differs from execution binding')
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(elf)],text=True)
    match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_compiler_pass$',symbols,re.M)
    s.require(match is not None,'no real initializer ELF symbol')
    first,size=[int(word,16) for word in match.groups()]
    s.require(first<state['pc']<=first+size,'native PC outside initializer')
    code=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),
                                 '--stop-address='+str(state['pc']),str(elf)],text=True)
    s.require('int3' in code,'native initializer frontier is not an INT3')
    s.require((native/'trace.txt').read_text()=='DllMain\nInvokeCompilerPass\ninitialize_compiler_pass\n',
              'native first-encounter application trace differs')
    if require_audit:s.audit(trial,audit_path)
    return {'trial':trial['trial'],'frontier':'_initialize_compiler_pass','raw_frontier_rva':0x38550,
            'argc':26,'mode':1,'compiler_mode_before_initializer':0,'last_error':203,'control_word':0x27f,
            'whole_mask_bytes_checked':751216,'audit_checked':require_audit,
            'original_seconds':o['seconds'],'native_seconds':n['seconds']}
