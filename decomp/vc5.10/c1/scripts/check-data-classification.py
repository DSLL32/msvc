#!/usr/bin/env python3
"""Prove the accepted parser correction changes only code/data graph roles."""
import argparse,copy,importlib.util,json,shutil,tempfile,time
from pathlib import Path
from audit_roles import BASE,TABLE,NAME,current_config,expected_code,sha
s=importlib.util.spec_from_file_location('trial',Path(__file__).with_name('check-trial.py'));m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
def union(config):
 return sorted((a,b) for r in config['driver'] for a,b in [[r['start'],r['end']]]+r.get('code_chunks',[])+r.get('shared',[]))
def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);a=p.parse_args();started=time.perf_counter();positive=m.check(a.original,a.native)
 candidate=json.loads((a.original/'candidate.json').read_text());old=json.loads((a.original/'functions.json').read_text());new=json.loads((m.PROJECT/candidate['audit_config']).read_text())
 assert current_config(old)==new and old!=new
 assert union(old)==union(new)
 entry=[(r['start_rva'],r['end_rva'],r['entry']) for r in candidate['ranges']]
 assert sorted((BASE+x,BASE+y)for x,y,_ in entry)==sorted(union(old)+[(BASE+i['start_rva'],BASE+i['end_rva'])for i in candidate['imports']])
 assert next(e for e in entry if e[:2]==tuple(TABLE))[2]==0
 assert positive['rust_owned_code_bytes']==2862 and positive['rust_owned_text_data_bytes']==16 and positive['rust_owned_original_bytes']==2878
 tests=['table-start','table-end','table-missing','extra-shared','extra-code','cold-missing','hot-end','name','accepted','candidate-role','range-entry','helper-identity','config-identity']
 for test in tests:
  with tempfile.TemporaryDirectory(prefix='classification-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1')as tmp:
   root=Path(tmp);o=root/'original';n=root/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n)
   if test in ('range-entry','candidate-role'):
    f=o/'candidate.json';d=json.loads(f.read_text())
    if test=='range-entry':next(r for r in d['ranges']if r['start_rva']==TABLE[0])['entry']=1
    else:next(r for r in d['functions']if r['name']==NAME)['role']='runtime-support'
   elif test=='helper-identity':f=o/'receipt.json';d=json.loads(f.read_text());d['helper_sha256']='0'*64
   elif test=='config-identity':f=o/'receipt.json';d=json.loads(f.read_text());d['functions_sha256']='0'*64
   else:
    f=o/'functions.json';d=json.loads(f.read_text());r=next(r for r in d['driver']if r['name']==NAME)
    if test=='table-start':r['code_chunks'][-1][0]+=1
    elif test=='table-end':r['code_chunks'][-1][1]+=1
    elif test=='table-missing':r['code_chunks'].pop()
    elif test=='extra-shared':r['shared']=[[BASE+0x12984,BASE+0x12988]]
    elif test=='extra-code':r['code_chunks'].append([BASE+0x12984,BASE+0x12988])
    elif test=='cold-missing':r['code_chunks'].pop(0)
    elif test=='hot-end':r['end']+=1
    elif test=='name':r['name']='invented'
    else:d['accepted'].remove(NAME)
   f.write_text(json.dumps(d))
   if f.name!='receipt.json':
    receipt=o/'receipt.json';v=json.loads(receipt.read_text());v['artifacts'][f.name]=dict(sha256=sha(f),size=f.stat().st_size)
    if f.name=='functions.json':v['functions_sha256']=sha(f)
    if f.name=='candidate.json':v['candidate_sha256']=sha(f)
    receipt.write_text(json.dumps(v))
   try:m.check(o,n)
   except(AssertionError,ValueError,KeyError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(status='PASS',classification_only=True,restored_union=union(new),entry_plan=entry,original_receipt_sha256=sha(a.original/'receipt.json'),native_receipt_sha256=sha(a.native/'receipt.json'),historical_config_sha256=sha(a.original/'functions.json'),corrected_config_sha256=sha(m.PROJECT/candidate['audit_config']),helper_sha256=sha(m.PROJECT/'build/dll-capture.exe'),old_audit_sha256=sha(a.original/'audit.json'),current_audit_sha256=sha(m.PROJECT/'build/trial-audit.json'),fresh_controls=len(tests),rejected=tests,seconds=time.perf_counter()-started),indent=2))
if __name__=='__main__':main()
