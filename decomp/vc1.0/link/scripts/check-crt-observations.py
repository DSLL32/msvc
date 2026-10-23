"""Host-only nonempty inventory and mismatch checks; no original/native execution."""
from pathlib import Path
import importlib.util,json,copy,time,tempfile
BINARY=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('crt_observations',Path(__file__).with_name('crt-observations.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def main():
    start=time.perf_counter()
    record=json.loads((BINARY/'cases/editor-rebase-bind/capture/strtoul-boundary/receipt.json').read_text())
    expected=record['expected_inventory'];original=record['original'];native=record['native']
    assert module.validate(expected,original,native)==1
    tests=[]
    for name in ('both-sides-omission','original-omission','native-omission','empty-plan','wrong-result','wrong-end-pointer','wrong-errno'):
        plan=copy.deepcopy(expected);left=copy.deepcopy(original);right=copy.deepcopy(native)
        if name=='both-sides-omission':left=[];right=[]
        elif name=='original-omission':left=[]
        elif name=='native-omission':right=[]
        elif name=='empty-plan':plan=[]
        elif name=='wrong-result':right[0]['after']['result']+=1
        elif name=='wrong-end-pointer':right[0]['after']['end_points_into_input']=False
        else:right[0]['after']['errno']=34
        try:module.validate(plan,left,right)
        except AssertionError as error:tests.append(dict(test=name,status='rejected',error=str(error)))
        else:raise RuntimeError('Invalid CRT inventory accepted: '+name)
    directory=Path(tempfile.mkdtemp(prefix='crt-observations-',dir=Path.home()/'.ai-tmp/msvc2/vc1.0/link/native'))
    result=dict(status='pass',tests=tests,total_seconds=time.perf_counter()-start,processes_launched=0)
    (directory/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(directory/'receipt.json')
if __name__=='__main__':main()
