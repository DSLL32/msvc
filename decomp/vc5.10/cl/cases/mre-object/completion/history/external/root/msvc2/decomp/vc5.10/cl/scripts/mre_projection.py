"""Paired typed state for the one supported fresh MRE signature candidate."""
import hashlib,json
from pathlib import Path
from case_inputs import load
from mre_signature_packet import decode as signature_response
from mre_selector_packet import decode as selector_response
from mre_sort_packet import decode as sort_response
from mre_jobs_packet import decode as jobs_response
from mre_state import verify

PROJECT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def fields(packet):
    if packet['normal_completion']:
        from mre_jobs_projection import fields as jobs_fields
        return jobs_fields(packet)
    objects=packet['target_objects']['mre-frontier'];options=objects['options']
    selector=packet['signature_frontier']['symbol']=='compare_option_strings'
    sort=packet['signature_frontier']['symbol']=='jobs_from_deferred'
    assert options
    if not(selector or sort):assert packet['signature_frontier']['option']==options[0]['address']
    origins={};rows=[]
    for option in options:
        pointer=option['origin']
        if pointer not in origins:origins[pointer]=len(origins)
        rows.append({**{name:option[name]for name in('flags','matching','name','value')},'origin_role':origins[pointer]if pointer else None})
    frontier=packet['signature_frontier']['full_state']
    parameters=({'left':packet['signature_frontier']['values'][0],'right':packet['signature_frontier']['values'][1],
        'positions':packet['signature_frontier']['positions'],'items':packet['signature_frontier']['items'],
        'count':packet['signature_frontier']['count'],'caller':'qualified qsort callback'}if selector else
        {'inputs':None,'caller':'prepare_jobs','pending':'five NULL pointers'}if sort else
        {'option':rows[0],'selectors':'Mm','option_alias':'Driver.options head'})
    result={'frontier':packet['signature_frontier']['symbol'],'case_inputs':load('mre-object'),
        'parameters':parameters,
        'driver_input':{'options':rows,'include':objects['include_input']['text'],
            'use_standard_include':objects['include_input']['use_standard_include'],'deferred_queue':'empty, tail aliases Driver.deferred_inputs'},
        'MRE':{'provider_sha256':packet['provider_sha256'],'calls':[{'name':c['name'],'ordinal':c['ordinal'],'stdcall_bytes':c['stdcall_bytes']}for c in packet['calls']],
            'IDB':{'bytes':packet['intermediate_files'][0]['bytes'],'sha256':packet['intermediate_files'][0]['sha256'],
                'descriptor':3,'position':1024,'binary':True,'noinherit':True,'access':'readwrite','lifetime':'open through frontier; later calls denied'},
            'errors':frontier['errors'],'CW':frontier['x87_control_word'],'CRT_profile':{'fmode':frontier['fmode'],'commode':frontier['commode'],
                'mb_cur_max':frontier['mb_cur_max'],'mb_codepage':frontier['mb_codepage'],'lc_codepage':frontier['lc_codepage'],'lc_handles':frontier['lc_handles']},
            'all_captured_CRT_OS_FILE_handle_aliases_preserved':True}}
    if selector or sort:
        lower=packet['lowercase_call']
        result['lowercase']={k:lower[k]for k in('before_hex','after_hex','initialized_bytes','errors')}
        result['lowercase'].update(calls=1,result_alias=True,qualified='MSVCRT!_mbslwr',profile='CP0/default-C',full_FP_preserved=True)
        result['formal_slot_qualification']='Original low byte reused for finalselector0; native formal remains actual Driver*, authenticated separately'
    if sort:
        callback=packet['comparator_call']
        result['comparator']={'values_hex':callback['values_hex'],'result':callback['result'],'observed_returns':1,
            'qualified':'MSVCRT!strcmp','cdecl_bytes':8,'errors':callback['public_before']['errors'],'full_FP_preserved':True,
            'qualification':callback['input_qualification']}
    return result


def project(platform,directory):
    directory=Path(directory)
    if platform=='original':
        jobs=(directory/'mre-observer.txt').read_bytes()==b'CL_MRE_JOBS_RESPONSE_V1\n'
        selector=(directory/'mre-observer.txt').read_bytes()==b'CL_MRE_SELECTOR_RESPONSE_V1\n'
        sort=(directory/'mre-observer.txt').read_bytes()==b'CL_MRE_SORT_RESPONSE_V1\n'
        packet=(jobs_response if jobs else sort_response if sort else selector_response if selector else signature_response)(directory)
        capture=json.loads((directory/'capture.json').read_bytes())
        names=['capture.json',*capture['files']]
    else:
        assert platform=='native'
        execution=json.loads((directory/'native-execution.json').read_bytes())
        jobs=execution.get('jobs_candidate',False)
        if jobs:
            assert execution.get('script_snapshots'),'missing immutable native script provenance'
            for name,item in execution['script_snapshots'].items():
                path=directory/'source-snapshots'/name
                assert Path(item['path']).name==name and sha(path)==item['sha256']
        selector=execution.get('selector_candidate',False)
        sort=execution.get('sort_candidate',False)
        assert all(type(v)is bool for v in(jobs,selector,sort,execution['signature_candidate']))and sum((jobs,selector,sort,execution['signature_candidate']))==1
        original=Path(execution['original_capture']['path']).parent
        assert sha(original/'capture.json')==execution['original_capture']['sha256']
        packet=(jobs_response if jobs else sort_response if sort else selector_response if selector else signature_response)(original)
        assert packet==json.loads((directory/'mre-response.json').read_bytes())
        assert sha(directory/'config.json')==packet['config_sha256']==execution['config_sha256']
        verify(original,directory,signature_candidate=not(jobs or selector or sort),selector_candidate=selector,sort_candidate=sort,jobs_candidate=jobs)
        capture=json.loads((directory/'capture.json').read_bytes())
        assert capture['controller_error']is None and capture.get('mre_jobs_response'if jobs else'mre_sort_response'if sort else'mre_selector_response'if selector else'mre_signature_response')is True
        assert capture['native_sha256']==execution['native_sha256']and capture['rebuilt_sha256']==execution['rebuilt_sha256']
        assert sha(directory/'native-build.json')==execution['native_build_sha256']
        names=['capture.json',*capture['files']]
    assert capture['case']=='mre-object'and capture['arguments']==load('mre-object')['arguments']
    for name,item in capture['files'].items():
        value=(directory/name).read_bytes()
        assert item=={'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()},'changed actual MRE raw capture '+name
    decoders=('mre_jobs_packet.py','mre_jobs_lifecycle.py','mre_jobs_outcome.py','mre_jobs_controls.py','mre_jobs_projection.py','jobs_native_state.py','mbslwr_sort_packet.py','sort_native_state.py','native_status.py')if jobs else('mre_sort_packet.py','mbslwr_sort_packet.py','sort_native_state.py')if sort else('mre_selector_packet.py','mbslwr_packet.py','selector_native_state.py')if selector else('mre_signature_packet.py',)
    decoders+=('mre_projection.py','mre_native.py','mre_state.py','mre_elf.py','dependency_response.py','mktemp_state.py')
    return {'projection':'cl-jobs-mre-completion-v1'if jobs else'cl-option-comparator-frontier-v1'if sort else'cl-option-selector-frontier-v1'if selector else'cl-option-signature-frontier-v1','fields':fields(packet),
        'decoder_sha256':sha(Path(__file__)),'derivation_sources':{name:sha(PROJECT/'scripts'/name)for name in decoders},
        'raw_inputs':{name:sha(directory/name)for name in names}}
