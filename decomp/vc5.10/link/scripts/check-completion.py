#!/usr/bin/env python3
"""Affected final-status, trace and final-generation rejection controls; no Wine."""
import argparse,copy,hashlib,json,shutil,struct,tempfile,time
from pathlib import Path
from accept import verify_completion,bind_capture,trial_for
from completion_cleanup import verify as verify_cleanup

parser=argparse.ArgumentParser(description=__doc__)
for name in ['original','native','cleanup','config']:parser.add_argument('--'+name,type=Path,required=True)
a=parser.parse_args();started=time.perf_counter();configuration=json.loads(a.config.read_text())
for path,kind in [(a.original,'original'),(a.native,'native-complete'),(a.cleanup,'native')]:bind_capture(path,configuration,kind)
master=verify_completion(a.original,a.native,configuration);proof=verify_cleanup(a.original,a.cleanup)
state=json.loads((a.native/'state.json').read_text());cleanup=json.loads((a.cleanup/'state.json').read_text())
events=[json.loads(line)for line in (a.original/'events.jsonl').read_text().splitlines()];rejected=[]
def deny(name,call):
    try:call()
    except (AssertionError,KeyError,ValueError,struct.error):rejected.append(name);return
    raise AssertionError('accepted completion mutation: '+name)
for key,value in [('logical_status',76),('posix_status',1100),('native_pid',state['native_pid']+1),('entries',state['entries'][:-1]),('entries',state['entries'][::-1]),('entries',state['entries']+['_free_buffer12']),('terminal_record_hex',state['terminal_record_hex'][:-2])]:
    changed=copy.deepcopy(state);changed[key]=value
    deny('terminal/trace '+key+' '+str(value)[:40],lambda s=changed:verify_completion(a.original,a.native,configuration,s))
for index,value in [(0,0),(1,2),(2,state['native_pid']+1),(3,0),(4,76),(5,1)]:
    changed=copy.deepcopy(state);words=list(struct.unpack('<6I',bytes.fromhex(changed['terminal_record_hex'])));words[index]=value;changed['terminal_record_hex']=struct.pack('<6I',*words).hex()
    deny('terminal word '+str(index),lambda s=changed:verify_completion(a.original,a.native,configuration,s))
for label,mutate in [
    ('missing final delete',lambda s:s['delete_calls'].pop()),
    ('wrong cleanup application trace',lambda s:s['entries'].pop()),
    ('reordered deletes',lambda s:s['delete_calls'].reverse()),
    ('final generation live',lambda s:s['cleanup_ownership'][-1].update(live=1)),
    ('wrong final generation',lambda s:s['delete_calls'][-1]['after'].update(generation=999)),
    ('wrong final size',lambda s:s['new_ownership'][0].update(requested=4096)),
    ('changed retained descriptor',lambda s:s['cleanup_descriptors'].update(buffer12='00'*12)),
    ('missing normal return',lambda s:s['delete_calls'][-1].update(normal_return=False)),
    ('unexpected terminal trap',lambda s:s.update(signal_number=5)),
    ('duplicate ownership generation',lambda s:s['cleanup_ownership'][0].update(generation=s['cleanup_ownership'][-1]['generation']))]:
    changed=copy.deepcopy(cleanup);mutate(changed);deny(label,lambda s=changed:verify_cleanup(a.original,a.cleanup,s))
for label,field,value in [('wrong original final argument','argument',0),('wrong original return stack','stack_pointer',0),('wrong original return site','site',0),('changed original descriptor','descriptor',[0,1024,1024])]:
    changed=copy.deepcopy(events);next(e for e in changed if e.get('name')=='observe:delete_return_12')[field]=value
    deny(label,lambda e=changed:verify_cleanup(a.original,a.cleanup,events=e))
# Exercise the current binding controller, rather than reusing startup-only controls.
def clone(source):
    destination=Path(tempfile.mkdtemp(prefix='completion-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/link'))
    for path in source.iterdir():
        if path.name=='receipt.json':shutil.copyfile(path,destination/path.name)
        else:(destination/path.name).symlink_to(path,target_is_directory=path.is_dir())
    return destination
wrong=copy.deepcopy(configuration);wrong['accepted'].remove('_free_buffer12')
for source,kind in [(a.original,'original'),(a.native,'native-complete')]:
    deny('wrong candidate-inclusive '+kind,lambda p=source,k=kind:bind_capture(p,wrong,k))
for field in ['native_sha256','rebuilt_sha256']:
    directory=clone(a.native);receipt=json.loads((directory/'receipt.json').read_text());receipt[field]='0'*64;(directory/'receipt.json').write_text(json.dumps(receipt))
    deny('stale '+field,lambda p=directory:bind_capture(p,configuration,'native-complete'))
for field in ['actual_loaded_crt_verified','actual_loaded_target_hfile_verified']:
    directory=clone(a.original);receipt=json.loads((directory/'receipt.json').read_text());receipt[field]=False;(directory/'receipt.json').write_text(json.dumps(receipt))
    deny('unverified '+field,lambda p=directory:bind_capture(p,configuration,'original'))
for source,kind in [(a.original,'original'),(a.native,'native-complete')]:
    directory=clone(source);changed=copy.deepcopy(configuration);changed['accepted'].remove('_free_buffer12');data=(json.dumps(changed,indent=2)+'\n').encode()
    (directory/'functions.json').unlink();(directory/'functions.json').write_bytes(data)
    receipt=json.loads((directory/'receipt.json').read_text());receipt['configuration_sha256']=hashlib.sha256(data).hexdigest();receipt['files']['functions.json']=dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest());(directory/'receipt.json').write_text(json.dumps(receipt))
    deny('mutually matching stale '+kind,lambda p=directory,k=kind:bind_capture(p,configuration,k))
for filename in ['stdout.bin','stderr.bin']:
    directory=clone(a.native);(directory/filename).unlink();(directory/filename).write_bytes(b'changed')
    deny('changed stream '+filename,lambda p=directory:verify_completion(a.original,p,configuration))
deny('unknown candidate',lambda:trial_for(configuration,'_invented'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
print(json.dumps(dict(status='passed',fresh_negative_count=len(rejected),rejected=rejected,cleanup=proof,
    input_sha256={str(p):sha(p)for p in [a.config,a.original/'receipt.json',a.native/'receipt.json',a.cleanup/'receipt.json',Path(__file__),Path(__file__).with_name('completion_cleanup.py'),Path(__file__).with_name('accept.py'),Path(__file__).with_name('check-native.py')]},
    elapsed_seconds=time.perf_counter()-started),indent=2))
