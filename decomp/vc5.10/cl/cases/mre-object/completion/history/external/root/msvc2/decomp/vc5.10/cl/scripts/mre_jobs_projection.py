"""Logical cross-image projection after independent complete ABI/state decoding."""
from case_inputs import load


def fields(packet):
    assert packet['normal_completion']is True and packet['application_credit']==0
    original=packet['target_objects']['jobs-before'];origins={};options=[]
    for row in original['options']:
        pointer=row['origin']
        if pointer not in origins:origins[pointer]=len(origins)
        options.append({**{n:row[n]for n in('flags','matching','name','value')},'origin_role':origins[pointer]if pointer else None})
    return {'case_inputs':load('mre-object'),'status':0,'normal_outcome':packet['normal_outcome'],
        'jobs':{'formal':None,'result':None,'caller':'prepare_jobs','pending':'five NULL pointers','ABI':'cdecl4, ordinary nonvolatiles and full FP preserved'},
        'MRE':{'provider_sha256':packet['provider_sha256'],'calls':[{'name':r['name'],'ordinal':r['ordinal'],'stdcall_bytes':r['stdcall_bytes'],'errors_before':r['before']['errors'],'errors_after':r['after']['errors']}for r in packet['calls']],
            'mandatory_inventory':[1]*6,'first_dependency_scan':1,'Close_commit':1,'roles':'opaque engine/driver+4; no contents or extent claim',
            'IDB':{'initial':{k:packet['intermediate_files'][0][k]for k in('bytes','sha256')},'final':packet['lifecycle']['final_idb'],'descriptor':3,'initial_flags':0x11,'initial_position':1024,'access':'readwrite','Release_preserves':True,'Close_writes_and_closes':True},'FILE_CRT_Winstd_aliases_preserved_through_Close':True,'full_FP_preserved':True,'CRT_profile':'provided MSVCRT510 CP0/defaultC/static tables'},
        'driver_input':{'options':options,'include':original['include_input']['text'],'use_standard_include':original['include_input']['use_standard_include'],'deferred':'empty; tail aliases Driver.deferred_inputs'},
        'lowercase':{k:packet['lowercase_call'][k]for k in('before_hex','after_hex','initialized_bytes','errors')},
        'comparator':{'values_hex':packet['comparator_call']['values_hex'],'result':1,'observed_returns':1,'qualification':packet['comparator_call']['input_qualification']},
        'qualifications':['Original full public state ends at post-Close; actual normal status/streams/trace/final IDB separately retained.','Private provider lock/heap/thread/object internals and allocation tails unobserved/unclaimed.']}
