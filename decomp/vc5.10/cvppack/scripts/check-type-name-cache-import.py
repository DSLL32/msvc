#!/usr/bin/env python3
"""The cache-reset loop needs exactly the observed cached free/EBP fact."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import time
from state import BINARY, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--audit',type=Path,default=BINARY/'cases/hello-debug-dll/evidence/0052-reset-type-name-cache/original/audit.json')
    parser.add_argument('--output',type=Path,default=BINARY/'cases/hello-debug-dll/type-name-cache-import-check.json')
    args=parser.parse_args();start=time.perf_counter()
    audit=json.loads(args.audit.read_text())
    report=audit['graph']['functions']['_reset_type_name_cache']['register_imports']
    identity={'dll':'msvcrt.dll','name':'free'}
    assert report['require_all_register_calls_resolved']
    assert report['original']['resolved_register_calls']=={str(0x40c783):identity}
    assert report['original']['unresolved_register_calls']==report['original']['other_unresolved_calls']==[]
    assert list(report['rebuilt']['direct_import_calls'].values())==[identity]
    assert report['rebuilt']['resolved_register_calls']=={}
    assert report['rebuilt']['unresolved_register_calls']==report['rebuilt']['other_unresolved_calls']==[]
    path=args.audit.parent/'graph-metadata.json';metadata=json.loads(path.read_text())
    matches=[row for row in metadata['register_imports']['preservation_contracts'] if row['target']=={'import':identity}]
    assert len(matches)==1 and matches[0]['preserves']==['esi','ebp']
    matches[0]['preserves'].remove('ebp')
    command=audit['graph_command'][:]
    assert sha(Path(command[0]))==audit['graph_tool_sha256']
    with tempfile.TemporaryDirectory(prefix='cvpack-free-ebp-',dir=Path.home()/'.ai-tmp') as temporary:
        changed=Path(temporary)/'metadata.json';changed.write_text(json.dumps(metadata))
        command[command.index('--metadata')+1]=str(changed)
        result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic=result.stderr.decode()
        assert result.returncode and 'unresolved register import calls' in diagnostic and '_reset_type_name_cache' in diagnostic, diagnostic
        mutated_metadata_sha256=sha(changed)
    receipt=dict(schema=1,native_only=True,positive_original_site=0x40c783,positive_qualified_import=identity,
        ebp_removal_rejected=True,diagnostic=diagnostic,audit_sha256=sha(args.audit),
        metadata_sha256=sha(path),mutated_metadata_sha256=mutated_metadata_sha256,
        graph_tool_sha256=audit['graph_tool_sha256'],checker_sha256=sha(Path(__file__)),
        evidence_sha256=sha(BINARY/'cases/hello-debug-dll/type-name-cache-import.json'),seconds=time.perf_counter()-start)
    args.output.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))


if __name__=='__main__':main()
