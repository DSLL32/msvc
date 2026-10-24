"""Paired typed state for the one supported fresh MRE signature candidate."""
import hashlib,json
from pathlib import Path
from case_inputs import load
from mre_signature_packet import decode
from mre_state import verify

PROJECT=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()


def fields(packet):
    objects=packet['target_objects']['mre-frontier'];options=objects['options']
    assert options and packet['signature_frontier']['option']==options[0]['address']
    origins={};rows=[]
    for option in options:
        pointer=option['origin']
        if pointer not in origins:origins[pointer]=len(origins)
        rows.append({**{name:option[name]for name in('flags','matching','name','value')},'origin_role':origins[pointer]if pointer else None})
    frontier=packet['signature_frontier']['full_state']
    return {'frontier':'option_matching_selector','case_inputs':load('mre-object'),
        'parameters':{'option':rows[0],'selectors':'Mm','option_alias':'Driver.options head'},
        'driver_input':{'options':rows,'include':objects['include_input']['text'],
            'use_standard_include':objects['include_input']['use_standard_include'],'deferred_queue':'empty, tail aliases Driver.deferred_inputs'},
        'MRE':{'provider_sha256':packet['provider_sha256'],'calls':[{'name':c['name'],'ordinal':c['ordinal'],'stdcall_bytes':c['stdcall_bytes']}for c in packet['calls']],
            'IDB':{'bytes':packet['intermediate_files'][0]['bytes'],'sha256':packet['intermediate_files'][0]['sha256'],
                'descriptor':3,'position':1024,'binary':True,'noinherit':True,'access':'readwrite','lifetime':'open through frontier; later calls denied'},
            'errors':frontier['errors'],'CW':frontier['x87_control_word'],'CRT_profile':{'fmode':frontier['fmode'],'commode':frontier['commode'],
                'mb_cur_max':frontier['mb_cur_max'],'mb_codepage':frontier['mb_codepage'],'lc_codepage':frontier['lc_codepage'],'lc_handles':frontier['lc_handles']},
            'all_captured_CRT_OS_FILE_handle_aliases_preserved':True}}


def project(platform,directory):
    directory=Path(directory)
    if platform=='original':
        packet=decode(directory)
        capture=json.loads((directory/'capture.json').read_bytes())
        names=['capture.json',*capture['files']]
    else:
        assert platform=='native'
        execution=json.loads((directory/'native-execution.json').read_bytes())
        assert execution['signature_candidate']is True
        original=Path(execution['original_capture']['path']).parent
        assert sha(original/'capture.json')==execution['original_capture']['sha256']
        packet=decode(original)
        assert packet==json.loads((directory/'mre-response.json').read_bytes())
        assert sha(directory/'config.json')==packet['config_sha256']==execution['config_sha256']
        verify(original,directory,signature_candidate=True)
        capture=json.loads((directory/'capture.json').read_bytes())
        assert capture['controller_error']is None and capture['mre_signature_response']is True
        assert capture['native_sha256']==execution['native_sha256']and capture['rebuilt_sha256']==execution['rebuilt_sha256']
        assert sha(directory/'native-build.json')==execution['native_build_sha256']
        names=['capture.json',*capture['files']]
    assert capture['case']=='mre-object'and capture['arguments']==load('mre-object')['arguments']
    for name,item in capture['files'].items():
        value=(directory/name).read_bytes()
        assert item=={'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()},'changed actual MRE raw capture '+name
    return {'projection':'cl-option-signature-frontier-v1','fields':fields(packet),
        'decoder_sha256':sha(Path(__file__)),'raw_inputs':{name:sha(directory/name)for name in names}}
