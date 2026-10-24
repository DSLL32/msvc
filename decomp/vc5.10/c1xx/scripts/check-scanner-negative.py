#!/usr/bin/env python3
"""Affected scanner projection/ABI/public-API denials without target execution."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import re
import tempfile
import time

from scanner_state import check as check_scanner


def load(name,filename):
    spec=importlib.util.spec_from_file_location(name,Path(__file__).with_name(filename));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    original,native=args.original.resolve(),args.native.resolve();decoder=load('scanner_negative_decoder','check-callback.py');history=load('scanner_negative_history','check-history.py')
    bindings={side+'_receipt_sha256':decoder.s.sha(path/'receipt.json')for side,path in (('original',original),('native',native))};audit=history.configure(decoder,original,native,saved_record=bindings);decoder.check(original,native,audit_path=audit)
    trial=json.loads((native/'trial.json').read_text());state=json.loads((native/'state.json').read_text());pre=json.loads((native/'pre.json').read_text());phase=json.loads((native/'phase.json').read_text());text=(original/'events.txt').read_text();traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    cases=('entry-PC','entry-formal','entry-array','entry-alias','native-RET','native-nonvolatile','next-PC','next-formal','output-array','output-neighbor','input-live','restored-array','saved-lifetime','serialized-result','public-argument','public-formal','public-result','public-RET','public-FP','public-nonvolatile','MB-gate','MB-codepage','MB-inventory','LC-handle','MB-table','old-table','result-identity','private-vector')
    if trial.get('test_seed'):cases=('input-live','restored-array','saved-lifetime','entry-array')
    results=[];started=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='scanner-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as temporary:
        work=Path(temporary)
        for path in native.iterdir():(work/path.name).symlink_to(path,target_is_directory=path.is_dir())
        for case in cases:
            current_state,current_pre,current_phase=copy.deepcopy(state),copy.deepcopy(pre),copy.deepcopy(phase);changed=[]
            def write(name,blob):
                path=work/name;path.unlink();path.write_bytes(blob);changed.append(path)
            def js(name,change):
                value=json.loads((work/('scanner-'+name+'.json')).read_text());change(value);write('scanner-'+name+'.json',json.dumps(value).encode())
            def flip(row,key,offset):
                blob=bytearray.fromhex(row[key]);blob[offset]^=1;row[key]=blob.hex()
            if case=='entry-PC':js('entries',lambda v:v[0].update(pc=v[0]['pc']+1))
            elif case=='entry-formal':js('entries',lambda v:v[0]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='entry-array':js('entries',lambda v:flip(v[0],'scanner',511))
            elif case=='entry-alias':current_state['frontend']['storage']['scanner.flags'][0]+=4
            elif case=='native-RET':js('returns',lambda v:v[0].update(esp=v[0]['esp']+4))
            elif case=='native-nonvolatile':js('returns',lambda v:v[0].update(ebx=v[0]['ebx']^4))
            elif case=='next-PC':js('next-entries',lambda v:v[0].update(pc=v[0]['pc']+1))
            elif case=='next-formal':js('next-entries',lambda v:v[0]['formal_inventory'].append(dict(name='invented',type='int')))
            elif case=='output-array':js('returns',lambda v:flip(v[0],'scanner',511))
            elif case=='output-neighbor':js('returns',lambda v:flip(v[0]['guard'],'data',0))
            elif case=='input-live':current_pre['frontend']['scanner']['classes'][255]^=1
            elif case=='restored-array':current_phase['frontend']['scanner']['classes'][255]^=1
            elif case=='saved-lifetime':current_state['frontend']['saved_scanner']['flags'][255]^=1
            elif case=='serialized-result':current_state['frontend']['scanner']['flags'][255]^=1
            elif case=='public-argument':js('api-entries',lambda v:v[0]['args'].__setitem__(0,-3))
            elif case=='public-formal':js('api-entries',lambda v:v[0]['formal_inventory'][0].update(type='unsigned int'))
            elif case=='public-result':js('api-returns',lambda v:v[0].update(eax=1))
            elif case=='public-RET':js('api-returns',lambda v:v[0].update(esp=v[0]['esp']+4))
            elif case=='public-FP':js('api-returns',lambda v:v[0]['fp'].update(cw=0x37f))
            elif case=='public-nonvolatile':js('api-returns',lambda v:v[0].update(esi=v[0]['esi']^4))
            elif case=='MB-gate':js('api-returns',lambda v:v[0]['locale'].update(mb_gate=1))
            elif case=='MB-codepage':js('api-returns',lambda v:v[0]['locale'].update(mb_codepage=0))
            elif case=='MB-inventory':js('api-returns',lambda v:v[0]['locale']['selection'].update(mb_calls=2))
            elif case=='LC-handle':js('api-returns',lambda v:v[0]['locale']['lc_handle'].__setitem__(1,0x409))
            elif case=='MB-table':js('api-returns',lambda v:flip(v[0]['locale'],'mbctype',256))
            elif case=='old-table':js('api-returns',lambda v:flip(v[0]['locale'],'old_ctype',513))
            elif case=='result-identity':js('api-returns',lambda v:v[0]['locale'].update(result_pointer=v[0]['locale']['result_pointer']+1))
            elif case=='private-vector':
                name='locale-native-mb-after-CRT-environment.bin';blob=bytearray((work/name).read_bytes());blob[-2]^=1;write(name,blob)
            else:raise RuntimeError(case)
            try:check_scanner(decoder.s,decoder.d,original,work,trial,text,current_pre,current_phase,current_state,trial.get('test_seed'),'%03d'%int(traps[-1][0]),traps)
            except(ValueError,AssertionError,KeyError)as error:results.append(dict(case=case,rejection=str(error)))
            else:raise RuntimeError('scanner mutant accepted: '+case)
            finally:
                for path in changed:path.unlink();path.symlink_to(native/path.name)
    assert not args.output.exists();result=dict(kind='fresh affected scanner/new-projection/public-API semantic controls after full paired strict positive; no original/native execution',bindings=bindings,original_directory=str(original),native_directory=str(native),count=len(results),cases=results,seconds=time.perf_counter()-started,checker_sha256=decoder.s.sha(Path(__file__)),semantic_checker_sha256=decoder.s.sha(Path(__file__).with_name('scanner_state.py')));args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'])))
