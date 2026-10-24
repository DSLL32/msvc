#!/usr/bin/env python3
"""Four affected original-boundary semantic denials after authenticated proof."""
import argparse,json,struct,tempfile,time
from pathlib import Path
from path_fold_original import check,sha


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();original=args.original.resolve();base=check(original);results=[];start=time.perf_counter()
    cases={'public-input':('path-fold-000-entry.bin',44,256),'cached-EBP':('path-fold-000-return.bin',36,0),'extended-unsigned-result':('path-fold-192-return.bin',20,65536),'unknown-neighbor':('locale-fold-complete-target-BSS.bin',0,1)}
    with tempfile.TemporaryDirectory(prefix='fold-original-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        work=Path(tmp)
        for p in original.iterdir():(work/p.name).symlink_to(p,target_is_directory=p.is_dir())
        for label,(name,offset,value)in cases.items():
            p=work/name;raw=bytearray(p.read_bytes());struct.pack_into('<I',raw,offset,value);p.unlink();p.write_bytes(raw)
            receipt=json.loads((original/'receipt.json').read_text());receipt['artifacts'][name]={'size':len(raw),'sha256':sha(p)};r=work/'receipt.json';r.unlink();r.write_text(json.dumps(receipt,indent=2)+'\n')
            try:check(work)
            except(ValueError,AssertionError)as error:
                need=str(error);assert 'artifact 'not in need,(label,need);results.append(dict(case=label,rejection=need))
            else:raise RuntimeError('original semantic mutant accepted: '+label)
            finally:p.unlink();p.symlink_to(original/name);r.unlink();r.symlink_to(original/'receipt.json')
    result=dict(kind='four original semantic controls; temporary mutated artifacts with explicitly rebound transport hashes, no target execution',original_receipt_sha256=base['original_receipt_sha256'],count=len(results),cases=results,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),semantic_checker_sha256=sha(Path(__file__).with_name('path_fold_original.py')));assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'])))
