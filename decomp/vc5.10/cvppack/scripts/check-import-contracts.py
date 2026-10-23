#!/usr/bin/env python3
"""Remove each reviewed contract/register fact and require rejection."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time
from state import BINARY, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--audit',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    audit=json.loads(args.audit.read_text())
    metadata=args.audit.parent/'graph-metadata.json'
    original=json.loads(metadata.read_text())
    spec=importlib.util.spec_from_file_location('import_sites',BINARY/'scripts/switch-data.py')
    checker=importlib.util.module_from_spec(spec);spec.loader.exec_module(checker)
    checker.check_import_sites(audit['graph'])
    command=audit['graph_command'][:]
    assert audit['graph_tool_sha256']==sha(Path(command[0]))
    tests=[];start=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='cvpack-contract-removal-',dir=Path.home()/'.ai-tmp') as temporary:
        path=Path(temporary)/'metadata.json'
        command[command.index('--metadata')+1]=str(path)
        def reject(name,modified):
            path.write_text(json.dumps(modified,indent=2)+'\n')
            begin=time.perf_counter()
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
            common_accepts=result.returncode==0
            site_complete=False
            diagnostic=result.stderr.decode()
            if common_accepts:
                try:checker.check_import_sites(json.loads(result.stdout))
                except AssertionError as error:diagnostic=str(error)
                else:site_complete=True
            assert not(common_accepts and site_complete), 'unnecessary preservation fact: '+name
            tests.append(dict(name=name,common_graph_accepts=common_accepts,site_complete=site_complete,
                metadata_sha256=sha(path),diagnostic=diagnostic,seconds=time.perf_counter()-begin))
        contracts=original['register_imports']['preservation_contracts']
        for index,row in enumerate(contracts):
            target=json.dumps(row['target'],sort_keys=True)
            changed=copy.deepcopy(original)
            changed['register_imports']['preservation_contracts'].pop(index)
            reject('contract '+target,changed)
            for register in row['preserves']:
                changed=copy.deepcopy(original)
                remaining=changed['register_imports']['preservation_contracts'][index]['preserves']
                remaining.remove(register)
                if not remaining:changed['register_imports']['preservation_contracts'].pop(index)
                reject('register '+target+' '+register,changed)
    report=dict(schema=1,native_only=True,audit_sha256=sha(args.audit),metadata_sha256=sha(metadata),
        graph_tool_sha256=audit['graph_tool_sha256'],checker_sha256=sha(Path(__file__)),
        site_checker_sha256=sha(BINARY/'scripts/switch-data.py'),
        contracts=len(contracts),register_facts=sum(len(row['preserves']) for row in contracts),
        tests=tests,seconds=time.perf_counter()-start)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(contracts=report['contracts'],register_facts=report['register_facts'],
        rejected=len(tests),seconds=report['seconds']),indent=2))


if __name__=='__main__':main()
