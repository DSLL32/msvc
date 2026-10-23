#!/usr/bin/env python3
"""Native/read-only rejection tests for the actual fresh initial capture pair."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
from common import HERE,ROOT,read,sha,verify_compiled_sources

spec=importlib.util.spec_from_file_location('verify_pair',HERE/'scripts/verify-pair.py')
v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
spec=importlib.util.spec_from_file_location('rc_audit',HERE/'scripts/audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

def seal(directory,side,name):
    place=directory/side;p=place/'receipt.json';j=read(p);j['files'][name]={'bytes':(place/name).stat().st_size,'sha256':sha(place/name)};p.write_text(json.dumps(j))

def run(initial):
    base_config=initial/'candidate.json'
    v.validate(initial,base_config)
    accepted=[]
    complete=v.validate(initial,base_config)['completion']
    # An unchanged captured ELF remains valid when shared worktree text drifts.
    compiled=read(initial/'native/compiled-sources.json')
    original_read=Path.read_text
    def without_live_shared(path,*args,**kwargs):
        assert not path.is_relative_to(ROOT/'lib/recon'),'Read live shared worktree instead of frozen compiled content'
        return original_read(path,*args,**kwargs)
    try:
        Path.read_text=without_live_shared
        verify_compiled_sources(compiled)
    finally:
        Path.read_text=original_read
    with tempfile.TemporaryDirectory(prefix='rc-rejections-',dir=str(Path.home()/'.ai-tmp')) as temp:
        temp=Path(temp)
        def check(name,change):
            place=temp/name;shutil.copytree(initial,place);config=place/'trial.json';config.write_bytes(base_config.read_bytes())
            change(place,config)
            try:v.validate(place,config)
            except (AssertionError,ValueError,KeyError,FileNotFoundError,StopIteration):accepted.append(name)
            else:raise AssertionError('Accepted negative: '+name)
        check('stale-config',lambda d,c:c.write_text(c.read_text()+' '))
        check('changed-raw-original',lambda d,c:(d/'original/events.txt').write_text('exit 0\n'))
        def nofrontier(d,c):
            p=d/'original/events.txt';p.write_text('\n'.join(l for l in p.read_text().splitlines() if not l.startswith('exit ' if complete else 'frontier '))+'\n');seal(d,'original','events.txt')
        check('resealed-missing-frontier',nofrontier)
        def empty_native(d,c):
            (d/'native/gdb.stdout').write_text('No actual result\n');seal(d,'native','gdb.stdout')
        check('resealed-missing-native-state',empty_native)
        def wrong_native(d,c):
            p=d/'native/gdb.stdout';p.write_text(__import__('re').sub(r'RC_RAW_STATE=([^\n]+)',lambda m:'RC_RAW_STATE='+json.dumps(dict(json.loads(m[1]),frontier='_rc_main',completion=False,status=124)),p.read_text()));seal(d,'native','gdb.stdout')
        check('resealed-wrong-native-trap',wrong_native)
        def native_state(d,mutate):
            import re
            p=d/'native/gdb.stdout'
            def edit(m):
                j=json.loads(m[1]);mutate(j);return 'RC_RAW_STATE='+json.dumps(j)
            p.write_text(re.sub(r'RC_RAW_STATE=([^\n]+)',edit,p.read_text()));seal(d,'native','gdb.stdout')
        if not complete:
            check('resealed-wrong-PC',lambda d,c:native_state(d,lambda j:j['pc_check'].update(pc=j['pc_check']['pc']+1,offset=j['pc_check']['offset']+1)))
            check('resealed-wrong-signal',lambda d,c:native_state(d,lambda j:j['pc_check'].update(signal=11)))
            check('resealed-wrong-typed-state',lambda d,c:native_state(d,lambda j:j['crt_state'].update(onexit_begin=0)))
        else:
            check('resealed-wrong-native-exit',lambda d,c:native_state(d,lambda j:j.update(status=124)))
            check('resealed-missing-completion',lambda d,c:native_state(d,lambda j:j.update(completion=False)))
            def no_exit_event(d,c):
                p=d/'native/gdb.stdout';p.write_text('\n'.join(line for line in p.read_text().splitlines() if 'exited normally]' not in line)+'\n');seal(d,'native','gdb.stdout')
            check('resealed-missing-GDB-exit',no_exit_event)
        def wrong_snapshot(d,c):
            p=d/'original/compiled-sources.json';j=read(p);j['src/startup.c']+=' ';p.write_text(json.dumps(j))
        check('wrong-immutable-source',wrong_snapshot)
        def wrong_native_snapshot(d,c):
            p=d/'native/compiled-sources.json';j=read(p)
            name=next(name for name in j['compiled_sources'] if name.startswith('lib/recon/'))
            j['compiled_sources'][name]+=' '
            p.write_text(json.dumps(j,indent=2)+'\n');seal(d,'native','compiled-sources.json')
        check('resealed-wrong-native-compiled-content',wrong_native_snapshot)
        def wrong_projection(d,c):
            p=d/'audit.json';j=read(p);j['runtime_projection']['original'][0]['end']+=1;p.write_text(json.dumps(j))
        check('wrong-runtime-projection',wrong_projection)
        def wrong_audit_map(d,c):
            p=d/'audit.json';j=read(p);j['input_bindings']['map']='0'*64;p.write_text(json.dumps(j))
        check('wrong-audit-MAP-pin',wrong_audit_map)

        def wrongmodule(d,c):
            p=d/'original/loaded-target.bin';p.write_bytes(p.read_bytes()+b'X');seal(d,'original','loaded-target.bin')
        check('resealed-wrong-loaded-target',wrongmodule)
        def wrongstream(d,c):
            (d/'native/stdout.bin').write_bytes(b'X');seal(d,'native','stdout.bin')
        check('resealed-stream-mismatch',wrongstream)
        for label,mutate in [('unknown-owner',lambda j:j['accepted'].append('_unknown')),('overlap',lambda j:j['driver'][0].update(start=j['driver'][1]['start'])),('accepted-trap-stub',lambda j:j['accepted'].append('_math_error'))]:
            p=temp/(label+'.json');j=read(base_config);mutate(j);p.write_text(json.dumps(j))
            try:audit.audit(p)
            except AssertionError:accepted.append(label)
            else:raise AssertionError('Audit accepted '+label)
    return accepted

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('initial',type=Path);a=p.parse_args();print(json.dumps({'rejected':run(a.initial)},indent=2))
