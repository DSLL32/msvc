"""Read-only native jobs return/lifecycle completion proof from addressed data."""
import hashlib,json
from pathlib import Path
from clock_state import compare_fp
from mre_state import state


def verify_tail(provider,directory,layout,pid,observations,callback_call):
    directory=Path(directory);original=provider.original;packet=provider.packet
    baseline=json.loads((directory/'native-mre-4-after-gdb.json').read_bytes());before_state=state(baseline)[0]
    records=[]
    for phase in('jobs-before','jobs-after'):
        r=json.loads((directory/('native-'+phase+'-gdb.json')).read_bytes());assert r['phase']==phase and r['pid']==pid
        layout.bind(r);derived,m=state(r);a=r['arguments'];g=r['general'];stack=a['stack']
        assert derived==before_state,'jobs modified actual target/provider state'
        assert m.words(stack,2)==[a['return'],0]and a['values']==[0]
        layout.call(a['return'],'jobs_from_deferred','prepare_jobs')
        assert m.read(a['pending'],20)==bytes(20)and m.word(r['symbols']['mre_call_count'])==4
        assert r['descriptor3']==baseline['descriptor3']and r['x87']==baseline['x87']
        compare_fp((original/('observer/'+phase+'-context.bin')).read_bytes()[28:140].hex(),r['x87'])
        if phase=='jobs-before':assert g['eip']==layout.bias+layout.symbol('jobs_from_deferred')and g['esp']==stack
        else:
            prior=records[0];assert a==prior['arguments']
            assert g['eip']==a['return']and g['esp']==stack+4 and g['eax']==0
            assert all(g[n]==prior['general'][n]for n in('ebx','esi','edi','ebp'))
        streams=json.loads((directory/('streams-'+phase+'.json')).read_bytes())
        assert streams==json.loads((directory/'streams-mre-4-after.json').read_bytes())
        records.append(r)
    # The six external ABI/state observations were independently decoded in
    # mre_state.verify; this final native point proves no cached role is used
    # again and the original's actual complete status/streams/artifact agree.
    r=json.loads((directory/'native-driver-exit-gdb.json').read_bytes());assert r['phase']=='driver-exit'and r['pid']==pid
    layout.bind(r);derived,m=state(r);a=r['arguments'];g=r['general'];sp=a['stack']
    assert g['eip']==layout.bias+layout.symbol('driver_exit')and g['esp']==sp
    assert m.words(sp,2)==[a['return'],0]and a['values']==[0]
    layout.call(a['return'],'driver_exit','main')
    assert m.word(r['symbols']['mre_call_count'])==6
    assert derived['descriptors']==[0,1,2]and derived['fd3_flags']==0 and r['descriptor3']is None
    assert m.word(r['data_addresses']['recon_osfhnd']+12)==0xffffffff
    last=json.loads((directory/'native-mre-6-after-gdb.json').read_bytes());last_state=state(last)[0]
    assert derived==last_state,'native public/app state changed after Close before actual exit'
    assert r['x87']==last['x87']
    streams=json.loads((directory/'streams-driver-exit.json').read_bytes())
    prior=json.loads((directory/'streams-mre-6-after.json').read_bytes())
    for family in('crt','win32'):
        for i in range(3):
            expected=dict(prior[family][i])
            if expected['role']in('stdout','stderr'):
                expected['position']=len((original/(expected['role']+'.bin')).read_bytes())
            assert streams[family][i]==expected,'changed native output handle identity/alias/position'
    assert streams['osfile']==prior['osfile']and streams['errors']==prior['errors']and streams['mre_calls']==6
    from native_status import read as terminal_status
    assert terminal_status(directory,0)==0
    for name in('stdout.bin','stderr.bin','trace.txt','case.idb'):assert(directory/name).read_bytes()==(original/name).read_bytes(),'different actual complete outcome '+name
    return {'case':'mre-object','scope':'Fresh Jobs candidate; full six-call external contract and actual normal outcome','calls':[1]*6,'jobs_formal':[None],'jobs_return':None,'lowercase_calls':1,'observed_comparator_returns':1,'native_pid':pid,'actual_native_ABI_state_and_file_match':True,'callback_call':callback_call,'status_DWORD':0,'final_idb_sha256':hashlib.sha256((directory/'case.idb').read_bytes()).hexdigest(),'file_lifetime':'Real fd3 owned by provider, open through Release, written/closed by Close; other descriptors/FILE/standard-handle aliases retained.','exit_state_qualification':'Original last full public state is post-Close. Native exit additionally validates its own same-native lifetime/alias/state relations; no unobserved original exit snapshot claimed.'}
