#!/usr/bin/env python3
"""One finite public strncmp EBP witness and its actual Rust necessity."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import import_evidence
from state import BINARY,sha


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();begin=time.monotonic();config=json.loads((a.original/'configuration.json').read_text());graph=json.loads((a.original/'graph-audit.json').read_text());positive=import_evidence.verify_strncmp_provider(config,graph);passed=[]
    with tempfile.TemporaryDirectory(prefix='strncmp-ebp-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        root=Path(folder);path=root/'cases/analysis/strncmp-ebp-contract.json';path.parent.mkdir(parents=True);original=(BINARY/'cases/analysis/strncmp-ebp-contract.json').read_bytes();path.write_bytes(original);import_evidence.BINARY=root
        try:
            assert import_evidence.verify_strncmp_provider(config,graph)
            for name,change in [('wrong provider body hash',lambda r:r.__setitem__('body_sha256','0'*64)),('wrong actual provider identity',lambda r:r.__setitem__('provider_sha256','0'*64)),('wrong provider entry',lambda r:r.__setitem__('entry',0x7801ae7e)),('wrong export ordinal',lambda r:r.__setitem__('ordinal',703)),('missing normal return',lambda r:r.__setitem__('normal_returns',[])),('omitted live cached call',lambda r:r.__setitem__('call_sites',r['call_sites'][:-1])),('invented helper call',lambda r:r.__setitem__('calls',[0x78001498])),('wrong public declaration',lambda r:r.__setitem__('declaration',r['declaration'].replace('__cdecl','__stdcall')))]:
                v=json.loads(original);before=copy.deepcopy(v);change(v);assert v!=before;raw=(json.dumps(v,indent=2)+'\n').encode();path.write_bytes(raw);trial=copy.deepcopy(config);row=next(r for r in trial['register_imports']['preservation_contracts']if r['evidence']['path']=='cases/analysis/strncmp-ebp-contract.json');row['evidence']['sha256']=sha(raw)
                try:import_evidence.verify_strncmp_provider(trial,graph)
                except AssertionError as error:passed.append({'control':name,'reason':str(error)})
                else:raise AssertionError('strncmp witness mutation accepted: '+name)
            path.write_bytes(original)
            trial=copy.deepcopy(config);next(r for r in trial['register_imports']['preservation_contracts']if r['target'].get('import',{}).get('name')=='strncmp')['preserves']=['esi']
            try:import_evidence.verify_strncmp_provider(trial,graph)
            except AssertionError:passed.append({'control':'wrong preserved register mask','reason':'only independently proved EBP contract permitted'})
            else:raise AssertionError('wrong preserved register mask accepted')
        finally:import_evidence.BINARY=BINARY
        metadata=json.loads((a.original/'graph-metadata.json').read_text());trial=copy.deepcopy(config)
        for value in [trial,metadata]:
            rows=value['register_imports']['preservation_contracts'];selected=[r for r in rows if r['target'].get('import',{}).get('name')=='strncmp'];assert len(selected)==1;rows.remove(selected[0])
        cfg=root/'without-strncmp.json';cfg.write_text(json.dumps(trial,indent=2)+'\n');meta=root/'without-strncmp-metadata.json';meta.write_text(json.dumps(metadata,indent=2)+'\n');receipt=json.loads((a.original/'audit-receipt.json').read_text());command=receipt['command'][:];assert sha(Path(command[0]).read_bytes())==receipt['tool_sha256']
        replacements={'--config':cfg,'--metadata':meta,'--rebuilt':a.original/'rebuilt.exe','--map':a.original/'rebuilt.map','--runtime':a.original/'runtime-original.json','--rebuilt-runtime':a.original/'runtime-rebuilt.json','--report':root/'negative-report.json'}
        for flag,value in replacements.items():command[command.index(flag)+1]=str(value)
        start=time.monotonic();result=subprocess.run(command,capture_output=True,timeout=30);seconds=time.monotonic()-start
        assert result.returncode!=0 and b'unresolved' in result.stderr.lower(),result.stderr.decode(errors='replace')
        passed.append({'control':'remove only new strncmp EBP preservation contract','reason':result.stderr.decode(errors='replace'),'seconds':seconds,'config_sha256':sha(cfg.read_bytes()),'metadata_sha256':sha(meta.read_bytes()),'tool_sha256':receipt['tool_sha256']})
    a.output.write_text(json.dumps({'positive':positive,'count':len(passed),'rejected':passed,'seconds':time.monotonic()-begin,'checker_sha256':sha(Path(__file__).read_bytes()),'predicate_sha256':sha((BINARY/'scripts/import_evidence.py').read_bytes()),'original_receipt_sha256':sha((a.original/'receipt.json').read_bytes()),'scope':'Newpublicstrncmp EBP only. Existing13EBP/8EDI/5ESI untouched; exact actual full84 Rust ablation rejects, no new targetexecution.'},indent=2)+'\n')

if __name__=='__main__':main()
