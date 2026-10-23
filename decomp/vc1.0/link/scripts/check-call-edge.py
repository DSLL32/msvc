"""Host-only rejection checks for caller-qualified native API aliases."""
from pathlib import Path
import copy,importlib.util,json,tempfile,time
from binary import PE
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
spec=importlib.util.spec_from_file_location('call_edge',Path(__file__).with_name('call-edge.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def main():
    start=time.perf_counter()
    proof=json.loads((BINARY/'cases/editor-rebase-bind/capture/map-view-caller/receipt.json').read_text())
    config=json.loads((BINARY/'functions.json').read_text())
    caller=next(r for r in config['driver'] if r['name']=='_map_rebase_image')
    context=proof['native_context'];callee=proof['caller_edge']['callee'];native='map_rebase_image'
    original=PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE')
    assert module.validate(context,callee,caller,original,native)['original_return']==context['stack'][0]
    tests=[]
    for name in ('both-empty','missing-native','missing-return','wrong-callee','GPA-only','stale-caller-extent','wrong-original-caller'):
        sample=copy.deepcopy(context);record=copy.deepcopy(caller);observed=native;target=callee
        if name=='both-empty':sample={};observed=''
        elif name=='missing-native':observed=''
        elif name=='missing-return':sample['stack']=[]
        elif name=='wrong-callee':target+=6
        elif name=='GPA-only':observed='map_file'
        elif name=='stale-caller-extent':record['end']=record['start']+1
        else:sample['stack'][0]=record['end']+5
        try:module.validate(sample,target,record,original,observed)
        except AssertionError as error:tests.append(dict(test=name,status='rejected',error=str(error)))
        else:raise RuntimeError('Invalid caller observation accepted: '+name)
    directory=Path(tempfile.mkdtemp(prefix='caller-edge-',dir=Path.home()/'.ai-tmp/msvc2/vc1.0/link/native'))
    result=dict(status='pass',tests=tests,total_seconds=time.perf_counter()-start,processes_launched=0)
    (directory/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(directory/'receipt.json')
if __name__=='__main__':main()
