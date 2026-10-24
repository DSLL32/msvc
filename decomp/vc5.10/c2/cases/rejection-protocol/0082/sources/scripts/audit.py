#!/usr/bin/env python3
"""Invoke shared Rust discovery against explicit trial metadata and Win32 bytes."""
import argparse
import json
from pathlib import Path
import subprocess
import shutil
import time
from state import BINARY, ROOT, TARGET, sha, graph_audit_binary

def run(config, report):
    executable=graph_audit_binary()
    args=[str(executable),'--profile','cl','--repo',str(ROOT),'--project',str(BINARY),
          '--original',str(TARGET),'--rebuilt',str(BINARY/'build/c2.exe'),
          '--map',str(BINARY/'build/c2.map'),'--config',str(config),
          '--runtime',str(BINARY/'runtime.json'),'--rebuilt-runtime',str(BINARY/'build/runtime.json'),
          '--metadata',str(BINARY/'build/graph-metadata.json'),'--discovery','cfg','--check-ret','--check-imports','--check-references','--check-accepted-extents','--check-ownership','--check-extents','--report',str(report)]
    begin=time.monotonic();result=subprocess.run(args,capture_output=True,timeout=30)
    assert result.returncode==0,result.stderr.decode(errors='replace')
    provenance=json.loads((BINARY/'build/original/input-provenance.json').read_text())
    assert provenance['binary_sha256']==sha((BINARY/'build/c2.exe').read_bytes()),'Win32 build provenance differs'
    source_file=report.parent/'win32-inputs.json';source_file.write_text(json.dumps(provenance,indent=2)+'\n')
    destination=report.parent/'win32-inputs'
    if destination.exists():
        for name,digest in provenance['inputs'].items():
            assert sha((destination/name).read_bytes())==digest,'immutable Win32 source snapshot differs'
    else:shutil.copytree(provenance['snapshot'],destination)
    parsed=json.loads(result.stdout)
    from data_evidence import verify as verify_data
    data_receipt=report.parent/'data-evidence.json'
    data_receipt.write_text(json.dumps(verify_data(json.loads(config.read_text())),indent=2)+'\n')
    from import_evidence import verify
    residual=report.parent/'import-residuals.json'
    residual.write_text(json.dumps(verify(json.loads(config.read_text()),parsed),indent=2)+'\n')
    from callback_evidence import verify as verify_callbacks
    callback_file=report.parent/'callback-evidence.json'
    callback_file.write_text(json.dumps(verify_callbacks(json.loads(config.read_text()),parsed),indent=2)+'\n')
    from writer_evidence import verify as verify_writer
    writer_file=report.parent/'writer-evidence.json'
    writer_file.write_text(json.dumps(verify_writer(json.loads(config.read_text()),parsed),indent=2)+'\n')
    assert parsed==json.loads(report.read_text()),'audit report differs'
    return dict(command=args,seconds=time.monotonic()-begin,configuration_sha256=sha(config.read_bytes()),
                tool_sha256=sha(executable.read_bytes()),writer_evidence_sha256=sha(writer_file.read_bytes()),callback_evidence_sha256=sha(callback_file.read_bytes()),import_residuals_sha256=sha(residual.read_bytes()),data_evidence_sha256=sha(data_receipt.read_bytes()),report_sha256=sha(report.read_bytes()),
                target_sha256=sha(TARGET.read_bytes()),rebuilt_sha256=sha((BINARY/'build/c2.exe').read_bytes()),
                rebuilt_map_sha256=sha((BINARY/'build/c2.map').read_bytes()),
                provenance_sha256=sha(source_file.read_bytes()),metadata_sha256=sha((BINARY/'build/graph-metadata.json').read_bytes()),runtime_sha256=sha((BINARY/'runtime.json').read_bytes()),
                rebuilt_runtime_sha256=sha((BINARY/'build/runtime.json').read_bytes()))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--config',type=Path,default=BINARY/'functions.json');p.add_argument('--report',type=Path,default=BINARY/'build/graph-audit.json');a=p.parse_args()
    print(json.dumps(run(a.config.resolve(),a.report.resolve()),indent=2))
