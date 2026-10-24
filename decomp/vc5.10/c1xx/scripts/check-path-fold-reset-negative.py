#!/usr/bin/env python3
"""Five fail-closed reset derivation controls; synthetic metadata only."""
import argparse,copy,json,tempfile,time
from pathlib import Path
from unittest.mock import patch
from path_fold_reset_refinement import check,sha


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();original,native=args.original.resolve(),args.native.resolve();positive=check(original,native);rulepath=Path(positive['rule_path']);rule=json.loads(rulepath.read_text());results=[];start=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='fold-reset-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx')as tmp:
        for case in ('trial-core','extra-bound-input','compiler-command','another-source','wrong-reset-value'):
            work=Path(tmp)/case;work.mkdir()
            for p in native.iterdir():(work/p.name).symlink_to(p,target_is_directory=p.is_dir())
            def js(name,mutate):
                p=work/name;d=json.loads(p.read_text());mutate(d);p.unlink();p.write_text(json.dumps(d,indent=2)+'\n')
            active=copy.deepcopy(rule)
            if case=='trial-core':js('trial.json',lambda d:d['target'].update(base=0))
            elif case=='extra-bound-input':js('trial.json',lambda d:d['bound_inputs'].update({'graph-metadata.json':'0'*64}))
            elif case=='compiler-command':js('build-provenance.json',lambda d:d['command'].append('-O2'))
            else:
                manifest=json.loads((work/'build-provenance.json').read_text());key=rule['source_key']if case=='wrong-reset-value'else'decomp/vc5.10/c1xx/src/scanner.h';old=manifest['source_content'][key];data=(work/'source-content'/old).read_bytes()
                data=data.replace(b'frontend_bss.filename_case_map[i] = 0;',b'frontend_bss.filename_case_map[i] = 1;')if case=='wrong-reset-value'else data+b'\n#define UNRELATED_SOURCE_DELTA 1\n'
                content=work/'source-content';content.unlink();content.mkdir()
                for p in (native/'source-content').iterdir():(content/p.name).symlink_to(p)
                import hashlib
                digest=hashlib.sha256(data).hexdigest();(content/digest).write_bytes(data);js('build-provenance.json',lambda d:d['source_content'].update({key:digest}))
                if case=='wrong-reset-value':active['source_after_sha256']=digest
            read_text=Path.read_text
            def controlled(self,*a,**kw):return json.dumps(active)if self==rulepath else read_text(self,*a,**kw)
            try:
                with patch.object(Path,'read_text',controlled):check(original,work)
            except(ValueError,AssertionError)as error:results.append(dict(case=case,rejection=str(error)))
            else:raise RuntimeError('reset derivation mutant accepted '+case)
    result=dict(kind='five explicitly synthetic derivation metadata/source controls; no claimed compiled execution/no original/native run',original_receipt_sha256=sha(original/'receipt.json'),native_receipt_sha256=sha(native/'receipt.json'),count=len(results),cases=results,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),derivation_checker_sha256=sha(Path(__file__).with_name('path_fold_reset_refinement.py')));assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=len(results),seconds=result['seconds'])))
