#!/usr/bin/env python3
"""Affected matcher/borrowed-pointer/public-call controls, without execution."""
import argparse,copy,importlib.util,json,struct,tempfile,time
from pathlib import Path
from argument_state import check as argument
from matcher_state import check as matcher


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();started=time.perf_counter()
    sp=importlib.util.spec_from_file_location('matcher_control_decoder',Path(__file__).with_name('check-callback.py'));m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
    o,n=a.original.resolve(),a.native.resolve();trial=json.loads((n/'trial.json').read_text());state=json.loads((n/'state.json').read_text());entry=json.loads((n/'argument-entry-state.json').read_text());stage=json.loads((n/'matcher-entry-state.json').read_text());text=(o/'events.txt').read_text();seed=bool(trial.get('test_seed'))
    def check(oo,nn,tr,tx,st,en,mid):
        prefix=argument(m.s,m.d,oo,nn,tr,tx,mid,en,{})
        return matcher(m.s,m.d,oo,nn,tr,tx,st,prefix)
    positive=check(o,n,trial,text,state,entry,stage)
    cases=['current-default','current-phase','current-final','current-owner','callback-return-pointer','callback-return-stack','callback-return-register','callback-return-byte','callback-return-owner','callback-return-FP','action-rule','action-value','action-copy','action-formal','action-caller','action-PC','action-owner','action-neighbor','argument-owner-all-matcher-phases','final-cursor','final-count','table-range','table-entry','table-byte','original-action-argument']
    cases+=['seed-owned-pointer','native-seed-owned-pointer','native-phase-pointer']if seed else['API-result','API-return-stack','API-return-nonvolatile','API-return-private-slot','API-return-input-slot','API-input-count','API-input-seed','API-owner','API-FP','API-body','API-table','API-table-after','API-bridge-owner']
    result=[]
    with tempfile.TemporaryDirectory(prefix='matcher-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        root=Path(tmp);oo,nn=root/'original',root/'native';oo.mkdir();nn.mkdir()
        for directory,source in((oo,o),(nn,n)):
            for f in source.iterdir():(directory/f.name).symlink_to(f,target_is_directory=f.is_dir())
        for case in cases:
            changed=[];tr=copy.deepcopy(trial);st=copy.deepcopy(state);en=copy.deepcopy(entry);mid=copy.deepcopy(stage)
            def write(directory,name,b):
                path=directory/name;path.unlink();path.write_bytes(b);changed.append((path,(o if directory==oo else n)/name))
            def js(name,fn):
                rows=json.loads((n/name).read_text());fn(rows[0]);write(nn,name,json.dumps(rows).encode())
            def object_js(name,fn):
                row=json.loads((n/name).read_text());fn(row);write(nn,name,json.dumps(row).encode())
            def flip(directory,name,at):
                b=bytearray(((o if directory==oo else n)/name).read_bytes());b[at]^=1;write(directory,name,b)
            def owner(row):row['locale']['pctype']+=2
            if case=='current-default':flip(oo,'current-argument-000.bin',0)
            elif case=='current-phase':flip(oo,'current-argument-phase.bin',0)
            elif case=='current-final':st['frontend']['current_argument']+=1
            elif case=='current-owner':
                js('matcher-next-entries.json',lambda r:r['guard'].update(bss=r['guard']['bss'][:-8]+'00000000'))
            elif case.startswith('callback-return-'):
                field=case[16:]
                if field=='pointer':js('matcher-next-returns.json',lambda r:r.update(eax=r['eax']+1))
                elif field=='stack':js('matcher-next-returns.json',lambda r:r.update(esp=r['esp']+4))
                elif field=='register':js('matcher-next-returns.json',lambda r:r.update(ebx=r['ebx']^1))
                elif field=='byte':js('matcher-next-returns.json',lambda r:r.update(value='00'))
                elif field=='owner':js('matcher-next-returns.json',owner)
                elif field=='FP':js('matcher-next-returns.json',lambda r:r['fp'].update(sw=r['fp']['sw']^1))
                else:raise ValueError(field)
            elif case.startswith('action-'):
                field=case[7:]
                if field in('rule','value','copy'):js('matcher-actions.json',lambda r:r['args'].__setitem__(['rule','value','copy'].index(field),r['args'][['rule','value','copy'].index(field)]^1))
                elif field=='formal':js('matcher-actions.json',lambda r:r['formal_inventory'][2].update(type='void *'))
                elif field=='caller':js('matcher-actions.json',lambda r:r.update(caller='process_compiler_options'))
                elif field=='PC':js('matcher-actions.json',lambda r:r.update(pc=r['pc']+1))
                elif field=='owner':js('matcher-actions.json',owner)
                elif field=='neighbor':js('matcher-actions.json',lambda r:r['guard'].update(bss='01'+r['guard']['bss'][2:]))
                else:raise ValueError(field)
            elif case=='argument-owner-all-matcher-phases':
                for f in('argument-matchers.json','matcher-next-entries.json','matcher-next-returns.json','matcher-actions.json'):js(f,owner)
            elif case=='final-cursor':st['frontend']['command_argv']+=4
            elif case=='final-count':st['frontend']['command_argc']-=1
            elif case=='table-range':tr['ranges'][-1]['end_rva']+=1
            elif case=='table-entry':tr['ranges'][-1]['entry']=1
            elif case=='table-byte':flip(oo,'relocated-text.bin',0x395e8-0x1000)
            elif case=='original-action-argument':flip(oo,'matcher-action-registers.bin',12*4)
            elif case=='seed-owned-pointer':flip(oo,'current-argument-006.bin',0)
            elif case=='native-seed-owned-pointer':object_js('pre.json',lambda r:r['frontend'].update(current_argument=r['frontend']['current_argument']+4))
            elif case=='native-phase-pointer':object_js('phase.json',lambda r:r['frontend'].update(current_argument=1))
            elif case.startswith('API-'):
                field=case[4:]
                if field=='result':js('signature-returns.json',lambda r:r.update(eax=r['eax']^1))
                elif field=='return-stack':js('signature-returns.json',lambda r:r.update(esp=r['esp']+4))
                elif field=='return-nonvolatile':js('signature-returns.json',lambda r:r.update(edi=r['edi']^1))
                elif field=='return-private-slot':js('signature-returns.json',lambda r:r['argument_slots_after'].__setitem__(2,0xffffffff))
                elif field=='return-input-slot':js('signature-returns.json',lambda r:r['argument_slots_after'].__setitem__(0,r['argument_slots_after'][0]+1))
                elif field=='input-count':js('vector-public-entries.json',lambda r:r['args'].__setitem__(1,2))
                elif field=='input-seed':js('vector-public-entries.json',lambda r:r['args'].__setitem__(2,0))
                elif field=='owner':js('signature-returns.json',owner)
                elif field=='FP':js('signature-returns.json',lambda r:r['fp'].update(sw=r['fp']['sw']^1))
                elif field=='body':flip(oo,'signature-provider-body.bin',1)
                elif field=='table':flip(oo,'signature-provider-table.bin',40)
                elif field=='table-after':flip(oo,'signature-provider-table-after.bin',40)
                elif field=='bridge-owner':
                    js('vector-public-entries.json',owner);js('signature-returns.json',owner)
                else:raise ValueError(field)
            else:raise ValueError(case)
            try:check(oo,nn,tr,text,st,en,mid)
            except(ValueError,AssertionError,KeyError)as e:result.append(dict(case=case,rejection=str(e)))
            else:raise ValueError('mutation accepted: '+case)
            finally:
                for f,source in changed:f.unlink();f.symlink_to(source)
    out=dict(kind='fresh affected matcher/current-pointer/publicCPP controls; no executable or original invocation',original=str(o),native=str(n),original_receipt_sha256=m.s.sha(o/'receipt.json'),native_receipt_sha256=m.s.sha(n/'receipt.json'),positive=positive,count=len(result),cases=result,seconds=time.perf_counter()-started,checkers={name:m.s.sha(Path(__file__).with_name(name))for name in('argument_state.py','matcher_state.py','signature_call_state.py','check-matcher-negative.py')})
    if a.output.exists():raise ValueError('refuse overwrite')
    a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(dict(count=out['count'],seconds=out['seconds'])))
if __name__=='__main__':main()
