#!/usr/bin/env python3
"""Failclosed support-mask scope: no arbitrary extra function/range/provider."""
import copy,importlib.util,json,time
from pathlib import Path


def main():
    spec=importlib.util.spec_from_file_location('support_prepare',Path(__file__).with_name('prepare-trial.py'));p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
    packet=json.loads((p.PROJECT/'cases/minimal/scaler-trials.json').read_text());frozen=Path(packet['trials'][0]['original'])
    candidate=json.loads((frozen/'candidate.json').read_text());cfg=json.loads((frozen/'functions.json').read_text());indexes={r['name']:r for r in cfg['driver']+cfg['imports']}
    assert candidate['trial']=='_calculate_arena_reservation' and p.compiler_support(candidate,indexes,cfg['accepted'])==['import:_ftol']
    passed=[];started=time.perf_counter()
    for case in ['extra-support-body','wrong-support','wrong-application-candidate','already-accepted-support','wrong-entry','unowned-extra-byte','wrong-dll','wrong-export']:
        c,i,a=copy.deepcopy(candidate),copy.deepcopy(indexes),list(cfg['accepted'])
        if case=='extra-support-body':c['support_imports'].append('_cleanup_compiler_pass')
        elif case=='wrong-support':c['support_imports']=['import:_initterm']
        elif case=='wrong-application-candidate':c['trial']='_main'
        elif case=='already-accepted-support':a.append('import:_ftol')
        elif case=='wrong-entry':i['import:_ftol']['start']-=1
        elif case=='unowned-extra-byte':i['import:_ftol']['end']+=1
        elif case=='wrong-dll':i['import:_ftol']['provider']['dll']='kernel32.dll'
        elif case=='wrong-export':i['import:_ftol']['provider']['name']='_ftol2'
        try:p.compiler_support(c,i,a)
        except ValueError:passed.append(case)
        else:raise ValueError('accepted malformed compiler support: '+case)
    print(json.dumps({'result':'PASS','count':len(passed),'seconds':time.perf_counter()-started,'scope':'bounded mask-selection policy only; strict actual original/rebuilt import catalogs checked separately','rejected':passed},indent=2))


if __name__=='__main__':main()
