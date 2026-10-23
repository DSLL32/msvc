#!/usr/bin/env python3
"""Read-only first-candidate state checks and meaningful rejection mutations."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from protocol import LOGICAL_NAME_ALIASES,entry_trial_pair,crt_init_pair,dll_main_pair,rc_entry_pair,rc_main_pair,rc_initialize_pair,rc_allocate_pair,rc_is_option_pair,rc_emit_pair,rc_message_pair,rc_format_pair,sha,HERE
p=argparse.ArgumentParser();p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path)
p.add_argument('--frozen-trial',choices=['rc_format']);a=p.parse_args();trial=a.frozen_trial or json.loads((HERE/'functions.json').read_text())['trial_function'];validate={'_DllMainCRTStartup':entry_trial_pair,'_CRT_INIT':crt_init_pair,'DllMain':dll_main_pair,'RC':rc_entry_pair,'main':rc_main_pair,'rc_initialize':rc_initialize_pair,'rc_allocate':rc_allocate_pair,'rc_is_option':rc_is_option_pair,'rc_emit':rc_emit_pair,'rc_message':rc_message_pair,'rc_format':rc_format_pair}[trial];proof=validate(a.original,a.native)
checks=[]
with tempfile.TemporaryDirectory(prefix='trial-negatives-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')as scratch:
    def rejected(name,mutate):
        old=Path(scratch)/(name+'-old');new=Path(scratch)/(name+'-new')
        shutil.copytree(a.original,old);shutil.copytree(a.native,new)
        mutate(old,new)
        try:validate(old,new)
        except (AssertionError,KeyError):checks.append(name)
        else:raise AssertionError('invalid trial passed: '+name)
    def refresh(directory,name):
        path=directory/name;receipt=json.loads((directory/'receipt.json').read_text());receipt['files'][name]=dict(bytes=path.stat().st_size,sha256=sha(path));(directory/'receipt.json').write_text(json.dumps(receipt))
    def native_field(field,value):
        def change(old,new):
            path=new/'snapshot.json';raw=json.loads(path.read_text());raw['arguments'][field]=value;path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
        return change
    if trial not in ['DllMain','RC','main','rc_initialize','rc_allocate','rc_is_option','rc_emit','rc_message','rc_format']:
        rejected('changed-reason',native_field('reason',0))
        rejected('changed-module',native_field('instance',0))
        rejected('zero-startup-context',native_field('reserved',0))
    elif trial=='rc_format':
        def exit_status(old,new):
            path=new/'exit.json';path.write_text('{"exit_code":1}');refresh(new,'exit.json')
        rejected('nonzero-native-exit',exit_status)
        if a.frozen_trial:
            def elf(old,new):
                path=new/'captured-native.elf';raw=bytearray(path.read_bytes());raw[0]=0;path.write_bytes(raw);refresh(new,'captured-native.elf')
            rejected('wrong-native-elf',elf)
        def trace(old,new):
            path=new/'trace.txt';path.write_text(path.read_text().replace('RC\n',''));refresh(new,'trace.txt')
        rejected('missing-native-entry',trace)
        def detach(old,new):
            path=old/'events.txt';path.write_text(path.read_text().replace('lifecycle-return 0 1','lifecycle-return 0 0'));refresh(old,'events.txt')
        rejected('failed-original-detach',detach)
    elif trial=='rc_message':
        rejected('wrong-format-build',native_field('build',1473))
        rejected('wrong-format-capacity',native_field('capacity',4096))
        rejected('wrong-format-version',native_field('version','5.00.0000.0'))
    elif trial=='rc_emit':
        rejected('wrong-message-id',native_field('id',20001))
        def effect(old,new):
            path=new/'events.txt';path.write_text(path.read_text().replace('after 2 0a00','after 2 0000'));refresh(new,'events.txt')
        rejected('changed-callback-poststate',effect)
    elif trial=='rc_is_option':
        rejected('wrong-first-message',native_field('message','0000'))
    elif trial=='rc_allocate':
        rejected('wrong-option-character',native_field('value',45))
        def owner(old,new):
            path=new/'snapshot.json';raw=json.loads(path.read_text());raw['heap_identity']['blocks'][0]['owner']=0;path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
        rejected('wrong-allocation-owner',owner)
    elif trial=='rc_initialize':
        rejected('wrong-allocation-request',native_field('bytes',519))
        def heap(old,new):
            path=new/'snapshot.json';raw=json.loads(path.read_text());raw['heap'][0]['flags']=0;path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
        rejected('wrong-heap-create-flags',heap)
    elif trial=='main':
        def jump(old,new):
            path=new/'snapshot.json';raw=json.loads(path.read_text());raw['jump']['opaque_setjmp3']=0;path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
        rejected('ordinary-context-instead-of-setjmp3',jump)
    else:
        if trial=='DllMain':
            rejected('unproved-resource-callback',native_field('resource',1))
            rejected('wrong-diagnostic-callback',native_field('diagnostic',0))
        else:
            rejected('wrong-diagnostic-callback',native_field('host_callback',0))
        rejected('wrong-export-count',native_field('count',1))
        rejected('wrong-export-argv',native_field('argv',['wrong','/help']))
        def disabled(old,new):
            path=new/'snapshot.json';raw=json.loads(path.read_text());raw['module']['thread_calls_disabled']=False;path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
        rejected('thread-notifications-enabled',disabled)
    def state(old,new):
        path=new/'snapshot.json';raw=json.loads(path.read_text());raw['globals']['dll_attach_count']='02000000';path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
    rejected('changed-encountered-global',state)
    def raw_hook(old,new):
        path=old/('completion-globals.bin' if trial=='rc_format' else 'globals.bin');raw=bytearray(path.read_bytes());raw[0x15468:0x1546c]=b'\x01\0\0\0';path.write_bytes(raw);refresh(old,path.name)
    rejected('unproved-raw-callback',raw_hook)
    def mask(old,new):
        path=old/'masked-text.bin';raw=bytearray(path.read_bytes());raw[0x1000]=0x90;path.write_bytes(raw);refresh(old,'masked-text.bin')
    rejected('extra-unmasked-code',mask)
    def stale(old,new):
        path=new/'functions.json';raw=json.loads(path.read_text());raw['trial_function']='RC';path.write_text(json.dumps(raw))
    rejected('wrong-trial-config',stale)
    def missing(old,new):
        path=old/'events.txt';path.write_text('\n'.join(line for line in path.read_text().splitlines()if not line.startswith('exit ' if trial=='rc_format' else 'frontier '))+'\n');refresh(old,'events.txt')
    rejected('missing-raw-frontier',missing)
    def wrong_module(name):
        def change(old,new):
            (old/name).write_bytes(b'wrong module');refresh(old,name)
        return change
    rejected('wrong-loaded-target',wrong_module('loaded-target.bin'))
    rejected('wrong-loaded-crt',wrong_module('loaded-crt.bin'))
    if trial=='_CRT_INIT':
        def allocation(field,value):
            def change(old,new):
                path=new/'snapshot.json';raw=json.loads(path.read_text());raw['allocations'][0][field]=value;path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
            return change
        rejected('wrong-allocation-size',allocation('bytes',124))
        rejected('wrong-allocation-result',allocation('result',0))
        def exit_field(field,value):
            def change(old,new):
                path=new/'snapshot.json';raw=json.loads(path.read_text());raw['exit_array'][field]=value;path.write_text(json.dumps(raw));refresh(new,'snapshot.json')
            return change
        rejected('wrong-exit-vector-cursor',exit_field('end',0))
        rejected('wrong-first-callback',exit_field('first','01000000'))
proof['negative_checks']=checks
if a.frozen_trial:print(json.dumps(dict(logical_name_aliases=LOGICAL_NAME_ALIASES,completion=proof['matched_completion'],application_trace=proof['application_trace'],negative_checks=checks),indent=2))
else:print(json.dumps(proof,indent=2))
