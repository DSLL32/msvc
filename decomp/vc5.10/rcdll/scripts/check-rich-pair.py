#!/usr/bin/env python3
"""Validate a rich-case raw checkpoint and reject meaningful altered evidence."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from protocol import HERE,sha
from rich_protocol import release_entry,release_trial,prepare_trial,write_trial,detect_trial,unicode_trial,line_trial,prefix_trial,notify_trial,preprocess_trial,widen_trial,preprocessor_main_trial,next_argument_trial,scan_options_trial,apply_option_trial
p=argparse.ArgumentParser();p.add_argument('--original',required=True,type=Path);p.add_argument('--native',required=True,type=Path);p.add_argument('--output',type=Path);p.add_argument('--stage',choices=['entry','trial','prepare','write','detect','unicode','line','prefix','notify','preprocess','widen','preprocessor-main','next-argument','scan-options','apply-option'],default='trial')
a=p.parse_args();validate={'entry':release_entry,'trial':release_trial,'prepare':prepare_trial,'write':write_trial,'detect':detect_trial,'unicode':unicode_trial,'line':line_trial,'prefix':prefix_trial,'notify':notify_trial,'preprocess':preprocess_trial,'widen':widen_trial,'preprocessor-main':preprocessor_main_trial,'next-argument':next_argument_trial,'scan-options':scan_options_trial,'apply-option':apply_option_trial}[a.stage];proof=validate(a.original,a.native);controls=[]
with tempfile.TemporaryDirectory(prefix='rich-gate-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')as scratch:
    old=Path(scratch)/'original';new=Path(scratch)/'native'
    shutil.copytree(a.original,old);shutil.copytree(a.native,new)
    validate(old,new) # The unchanged copies MUST pass before rejection controls.
    def refresh(directory,name):
        p=directory/name;r=json.loads((directory/'receipt.json').read_text());r['files'][name]=dict(bytes=p.stat().st_size,sha256=sha(p));(directory/'receipt.json').write_text(json.dumps(r))
    def reject(name,side,filename,change):
        directory=old if side=='original'else new;path=directory/filename
        original=path.read_bytes();receipt=(directory/'receipt.json').read_bytes()
        change(path);refresh(directory,filename)
        try:validate(old,new)
        except (AssertionError,KeyError):controls.append(name)
        else:raise AssertionError('bad evidence passed: '+name)
        path.write_bytes(original);(directory/'receipt.json').write_bytes(receipt)
    def snapshot(change):
        def apply(path):
            value=json.loads(path.read_text());change(value);path.write_text(json.dumps(value))
        return apply
    reject('wrong-callee','native','snapshot.json',snapshot(lambda v:v.update(function='Handler')))
    reject('missing-native-int3','native','snapshot.json',snapshot(lambda v:v.update(executed_byte='90')))
    reject('changed-current-argv','native','snapshot.json',snapshot(lambda v:v['export_vector'][2].update(text='wrong')))
    reject('changed-original-storage','native','snapshot.json',snapshot(lambda v:v['export_vector'][3].update(bytes='00')))
    reject('changed-argv-alias','native','snapshot.json',snapshot(lambda v:v['export_vector'][3].update(pointer=v['export_vector'][3]['storage'])))
    reject('wrong-working-directory','native','snapshot.json',snapshot(lambda v:v['case_inputs'].update(host_cwd='/')))
    reject('wrong-INCLUDE','native','snapshot.json',snapshot(lambda v:v['case_inputs'].update(include='wrong')))
    reject('target-host-environment-leak','native','snapshot.json',snapshot(lambda v:v['case_inputs']['environment'].append('RECON_ACP=1252')))
    if a.stage=='entry':
        reject('wrong-live-buffer','native','snapshot.json',snapshot(lambda v:v['arguments'].update(new_payload='00'*528)))
    elif a.stage=='trial':
        reject('wrong-preprocessor-argv','native','snapshot.json',snapshot(lambda v:v['rich']['preprocessor_argv'].pop()))
        reject('wrong-input-path','native','snapshot.json',snapshot(lambda v:v['arguments'].update(path='wrong.rc')))
        reject('wrong-temporary','native','snapshot.json',snapshot(lambda v:v['rich']['temporaries'].update(rc_input_temporary='wrong')))
        reject('wrong-free-flags','native','snapshot.json',snapshot(lambda v:next(x for x in v['heap']if x['operation']=='free').update(flags=0)))
        reject('free-did-not-unlink','native','snapshot.json',snapshot(lambda v:next(x for x in v['heap']if x['operation']=='free')['live_after'].append(dict(payload=0,bytes=528,flags=9))))
    if a.stage=='prepare':
        reject('wrong-write-size','native','snapshot.json',snapshot(lambda v:v['arguments'].update(bytes=44)))
        reject('wrong-write-bytes','native','snapshot.json',snapshot(lambda v:v['arguments'].update(payload='00'*46)))
        reject('wrong-wide-filename','native','snapshot.json',snapshot(lambda v:v['arguments'].update(filename='00'*514)))
        reject('wrong-file-position','native','snapshot.json',snapshot(lambda v:v['arguments'].update(file_fdinfo='pos: 1\n')))
        reject('wrong-parent-frame','native','snapshot.json',snapshot(lambda v:v['arguments']['parent_frame'].update(caller='main')))
        reject('wrong-array-bounds','native','snapshot.json',snapshot(lambda v:v['arguments']['parent_frame']['objects'][0].update(address=0)))
        reject('wrong-array-layout','native','snapshot.json',snapshot(lambda v:v['arguments']['parent_frame']['objects'][1].update(bytes=1)))
        reject('wrong-array-write-pointer','native','snapshot.json',snapshot(lambda v:v['arguments'].update(data=0)))
        reject('wrong-compiler-input-size','original','events.txt',lambda p:p.write_text(p.read_text().replace('compiler-probe-entry eax=000042c4','compiler-probe-entry eax=000042c0')))
        reject('wrong-compiler-stack-return','original','events.txt',lambda p:p.write_text(p.read_text().replace('compiler-probe-return eax=71c28576','compiler-probe-return eax=00000000')))
    if a.stage=='write':
        reject('wrong-buffered-write','native','snapshot.json',snapshot(lambda v:v['arguments'].update(output_buffer='00'*46)))
        reject('wrong-write-return','native','snapshot.json',snapshot(lambda v:v['arguments'].update(write_returns=[45])))
        reject('wrong-input-file-state','native','snapshot.json',snapshot(lambda v:v['arguments'].update(input_state='00'*32)))
        reject('wrong-output-file-state','native','snapshot.json',snapshot(lambda v:v['arguments'].update(output_state='00'*32)))
        reject('wrong-original-write-result','original','events.txt',lambda p:p.write_text(p.read_text().replace('write-result 46','write-result 45')))
    if a.stage=='detect':
        reject('wrong-sample-size','native','snapshot.json',snapshot(lambda v:v['arguments'].update(bytes=248)))
        reject('wrong-sample-bytes','native','snapshot.json',snapshot(lambda v:v['arguments'].update(sample='00'*252)))
        reject('wrong-initial-flags','native','snapshot.json',snapshot(lambda v:v['arguments'].update(flags=0)))
        reject('wrong-original-position','native','snapshot.json',snapshot(lambda v:v['arguments'].update(position=1)))
        reject('wrong-file-buffer','native','snapshot.json',snapshot(lambda v:v['arguments'].update(file_buffer='00')))
        reject('wrong-input-file-state','native','snapshot.json',snapshot(lambda v:v['arguments'].update(file_state='00'*32)))
    if a.stage=='unicode':
        reject('wrong-line-capacity','native','snapshot.json',snapshot(lambda v:v['arguments'].update(capacity=4096)))
        reject('wrong-input-encoding','native','snapshot.json',snapshot(lambda v:v['arguments'].update(wide=1)))
        reject('wrong-classification','native','snapshot.json',snapshot(lambda v:v['arguments'].update(unicode_results=[dict(result=1,flags=8)])))
        reject('wrong-cpinfo-fields','native','snapshot.json',snapshot(lambda v:v['arguments']['cpinfo'][0].update(after='00'*20)))
        reject('wrong-reset-file-state','native','snapshot.json',snapshot(lambda v:v['arguments'].update(file_state='00'*32)))
        reject('wrong-original-classification','original','events.txt',lambda p:p.write_text(p.read_text().replace('text-test-result 0','text-test-result 1')))
        reject('wrong-original-cpinfo-error','original','events.txt',lambda p:p.write_text(p.read_text().replace('cpinfo-result 1 last-error=00000000','cpinfo-result 1 last-error=00000001')))
    if a.stage=='line':
        reject('wrong-prefix','native','snapshot.json',snapshot(lambda v:v['arguments'].update(prefix='0000')))
        reject('wrong-first-line','native','snapshot.json',snapshot(lambda v:v['arguments'].update(text='0000')))
        reject('wrong-line-number','native','snapshot.json',snapshot(lambda v:v['arguments'].update(current_line=1)))
        reject('wrong-local-ownership','native','snapshot.json',snapshot(lambda v:v['arguments']['locals'][1].update(pointer=0)))
        reject('wrong-local-zeroing','native','snapshot.json',snapshot(lambda v:v['arguments']['locals'][0].update(contents='5a'*4177)))
        reject('wrong-line-buffer','native','snapshot.json',snapshot(lambda v:v['arguments']['locals'][1].update(contents='00'*4177)))
        reject('local-free-not-unlinked','native','snapshot.json',snapshot(lambda v:v['arguments']['locals'][1]['live'].append(dict(pointer=1,bytes=4177,flags=64))))
        reject('wrong-ctype-result','native','snapshot.json',snapshot(lambda v:v['arguments']['ctype'][0].update(result=8)))
        reject('wrong-file-position','native','snapshot.json',snapshot(lambda v:v['arguments'].update(file_state='00'*32)))
        reject('wrong-unread-buffer','native','snapshot.json',snapshot(lambda v:v['arguments'].update(file_buffer='00')))
    if a.stage=='prefix':
        reject('wrong-progress-kind','native','snapshot.json',snapshot(lambda v:v['arguments'].update(kind=2)))
        reject('wrong-progress-count','native','snapshot.json',snapshot(lambda v:v['arguments'].update(value=63)))
        reject('wrong-current-line','native','snapshot.json',snapshot(lambda v:v['arguments'].update(current_line=0)))
        reject('wrong-source-line','native','snapshot.json',snapshot(lambda v:v['arguments'].update(line='0000')))
        reject('wrong-case-mapping','native','snapshot.json',snapshot(lambda v:v['arguments']['towupper'][0].update(result=114)))
        reject('wrong-prefix-return','native','snapshot.json',snapshot(lambda v:v['arguments'].update(prefix_results=[-1])))
        reject('wrong-format-count','native','snapshot.json',snapshot(lambda v:v['arguments']['line_format'][0].update(result=10)))
        reject('wrong-format-number','native','snapshot.json',snapshot(lambda v:v['arguments']['line_format'][0].update(number=2)))
        reject('wrong-pending-output','native','snapshot.json',snapshot(lambda v:v['arguments'].update(output_buffer='00')))
        reject('wrong-pending-input','native','snapshot.json',snapshot(lambda v:v['arguments'].update(input_buffer='00')))
    if a.stage=='notify':
        reject('wrong-preprocessor-count','native','snapshot.json',snapshot(lambda v:v['arguments'].update(count=19)))
        reject('wrong-preprocessor-unused','native','snapshot.json',snapshot(lambda v:v['arguments'].update(unused=1)))
        reject('wrong-preprocessor-argv','native','snapshot.json',snapshot(lambda v:v['arguments']['argv'].pop()))
        reject('wrong-prepared-total','native','snapshot.json',snapshot(lambda v:v['arguments'].update(prepared_lines=125)))
        reject('wrong-current-line','native','snapshot.json',snapshot(lambda v:v['arguments'].update(current_line=126)))
        reject('wrong-filename','native','snapshot.json',snapshot(lambda v:v['arguments'].update(filename='00'*514)))
        reject('input-not-closed','native','snapshot.json',snapshot(lambda v:v['arguments'].update(closed_input='00'*32)))
        reject('unproved-window-callback','native','snapshot.json',snapshot(lambda v:v['globals'].update(rc_window='01000000')))
    if a.stage=='preprocess':
        reject('wrong-forwarded-count','native','snapshot.json',snapshot(lambda v:v['arguments'].update(count=19)))
        reject('wrong-forwarded-argv','native','snapshot.json',snapshot(lambda v:v['arguments']['argv'].pop()))
        reject('wrong-prepared-total','native','snapshot.json',snapshot(lambda v:v['arguments'].update(prepared_lines=125)))
        reject('wrong-current-line','native','snapshot.json',snapshot(lambda v:v['arguments'].update(current_line=126)))
        reject('wrong-filename','native','snapshot.json',snapshot(lambda v:v['arguments'].update(filename='00'*514)))
        reject('input-not-closed','native','snapshot.json',snapshot(lambda v:v['arguments'].update(closed_input='00'*32)))
        reject('unproved-window-callback','native','snapshot.json',snapshot(lambda v:v['globals'].update(rc_window='01000000')))
    if a.stage=='widen':
        reject('wrong-wide-count','native','snapshot.json',snapshot(lambda v:v['arguments'].update(count=19)))
        reject('wrong-wide-text','native','snapshot.json',snapshot(lambda v:v['arguments']['wide_vector'][0].update(bytes='0000')))
        reject('wrong-packed-alias','native','snapshot.json',snapshot(lambda v:v['arguments']['wide_vector'][0].update(pointer=0)))
        reject('wrong-wide-terminator','native','snapshot.json',snapshot(lambda v:v['arguments'].update(wide_terminator=1)))
        reject('wrong-vector-base','native','snapshot.json',snapshot(lambda v:v['arguments'].update(vector_pointer=0)))
        reject('wrong-converted-argv','native','snapshot.json',snapshot(lambda v:v['arguments']['argv'].pop()))
        reject('wrong-new-allocation-size','native','snapshot.json',snapshot(lambda v:v['heap_identity']['blocks'][0].update(bytes=1)))
        reject('wrong-initial-zeroes','native','snapshot.json',snapshot(lambda v:next(x for x in reversed(v['heap'])if x['operation']=='allocate').update(payload='5a')))
    if a.stage in ['preprocessor-main','next-argument','scan-options','apply-option']:
        reject('wrong-preprocessor-remaining','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['globals']['rc_cpp_remaining'].update(bytes='00000000')))
        reject('wrong-preprocessor-vector','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['globals']['rc_cpp_arguments'].update(bytes='00000000')))
        reject('wrong-option-pattern','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['tables']['options']['rows'][0].update(pattern='0000')))
        reject('wrong-option-destination','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['tables']['options']['rows'][0].update(destination=0)))
        reject('wrong-option-action','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['tables']['options']['rows'][0].update(action=1)))
        reject('wrong-letter-action','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['tables']['letters']['rows'][0].update(action=1)))
        reject('wrong-packing-default','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['globals']['rc_cpp_pack'].update(bytes='01000000')))
        reject('wrong-wide-owned-data','native','snapshot.json',snapshot(lambda v:v['arguments']['wide_vector'][0].update(bytes='0000')))
    if a.stage=='next-argument':
        reject('wrong-scanner-getter','native','snapshot.json',snapshot(lambda v:v['arguments']['scan'].update(next=0)))
        reject('wrong-scanner-alias','native','snapshot.json',snapshot(lambda v:v['arguments']['scan'].update(argument=v['arguments']['wide_vector'][0]['pointer'])))
        reject('wrong-scanner-argument','native','snapshot.json',snapshot(lambda v:v['arguments']['scan'].update(argument_bytes='0000')))
        reject('wrong-scanner-table','native','snapshot.json',snapshot(lambda v:v['arguments']['scan'].update(options=0)))
        reject('wrong-scanner-duplicate','native','snapshot.json',snapshot(lambda v:v['arguments']['scan'].update(duplicate=1)))
    if a.stage=='scan-options':
        reject('wrong-applied-descriptor','native','snapshot.json',snapshot(lambda v:v['arguments']['apply'].update(option=0)))
        reject('wrong-operand-alias','native','snapshot.json',snapshot(lambda v:v['arguments']['apply'].update(operand=v['arguments']['wide_vector'][1]['pointer'])))
        reject('wrong-operand-text','native','snapshot.json',snapshot(lambda v:v['arguments']['apply'].update(operand_bytes='0000')))
        reject('wrong-operand-duplicate','native','snapshot.json',snapshot(lambda v:v['arguments']['apply'].update(duplicate=1)))
        reject('wrong-current-option','native','snapshot.json',snapshot(lambda v:v['arguments']['preprocessor']['globals']['rc_cpp_option_argument'].update(bytes='00000000')))
    if a.stage=='apply-option':
        reject('wrong-number-text','native','snapshot.json',snapshot(lambda v:v['arguments']['number'].update(operand_bytes='0000')))
        reject('wrong-number-alias','native','snapshot.json',snapshot(lambda v:v['arguments']['number'].update(operand=0)))
        reject('wrong-parent-descriptor','native','snapshot.json',snapshot(lambda v:v['arguments']['number']['parent'].update(option=0)))
        reject('wrong-parent-duplicate','native','snapshot.json',snapshot(lambda v:v['arguments']['number']['parent'].update(duplicate=1)))
    reject('wrong-allocation-owner','native','snapshot.json',snapshot(lambda v:v['heap_identity']['blocks'][0].update(owner=0)))
    reject('wrong-registered-module','native','snapshot.json',snapshot(lambda v:v['module'].update(sha256='0'*64)))
    reject('wrong-codepage','native','snapshot.json',snapshot(lambda v:v['globals'].update(rc_codepage='00000000')))
    reject('wrong-loaded-target','original','loaded-target.bin',lambda p:p.write_bytes(b'wrong target'))
    reject('wrong-loaded-CRT','original','loaded-crt.bin',lambda p:p.write_bytes(b'wrong CRT'))
    reject('stale-trial','native','functions.json',snapshot(lambda v:v.update(trial_function='Handler')))
    reject('wrong-loaded-driver','original','loaded-driver.bin',lambda p:p.write_bytes(b'wrong driver'))
    reject('extra-original-unmask','original','masked-text.bin',lambda p:p.write_bytes(p.read_bytes()[:0x1000]+b'\x90'+p.read_bytes()[0x1001:]))
    reject('missing-original-frontier','original','events.txt',lambda p:p.write_text('\n'.join(l for l in p.read_text().splitlines()if not l.startswith('frontier '))+'\n'))
    validate(old,new)
proof['negative_controls']=controls
if a.output:a.output.write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps(dict(matched_frontier=proof['matched_frontier'],application_trace=proof['application_trace'],negative_controls=controls),indent=2))
