"""Use the shared Rust CFG auditor for C1XX canonical and candidate audits.

Data defaults/aliases, exact callback slot/sentinel policy, grammars and reached
CRT identity/behavior remain separate Python checks. This module contains no
instruction decoder and never invokes Wine or Cargo.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from case_paths import BINARY,ROOT,CAPTURE_ROOT
from graph_metadata import metadata

FLAGS=['--discovery','cfg','--check-ret','--check-references','--check-callbacks',
       '--check-ownership','--check-extents']
TOOL=Path(os.environ.get('GRAPH_AUDIT_BIN',ROOT/'tooling/graph-audit/target/release/graph-audit'))


def command(config=None, graph_metadata=None):
    assert TOOL.is_file(),'Build the shared host auditor explicitly with make graph-audit-tool'
    result=[str(TOOL),'--repo',str(ROOT),'--project',str(BINARY),'--profile','c1xx',*FLAGS,
            '--metadata',str(graph_metadata or BINARY/'build/graph-metadata.json')]
    if config is not None:result+=['--config',str(config)]
    return result


def verify_candidate(name, config_path=None):
    """Check the complete current graph plus an already reconstructed candidate."""
    config=json.loads(Path(config_path or BINARY/'functions.json').read_text())
    assert name not in config['accepted'],'candidate already accepted'
    record=next(r for r in config['driver'] if r['name']==name)
    assert record['end']>record['start']+1,'candidate needs a complete original extent'
    config['accepted'].append(name)
    CAPTURE_ROOT.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='graph-candidate-',dir=CAPTURE_ROOT) as scratch:
        directory=Path(scratch)
        config_path=directory/'functions.json';config_path.write_text(json.dumps(config)+'\n')
        metadata_path=directory/'metadata.json';metadata_path.write_text(json.dumps(metadata(config))+'\n')
        invocation=command(config_path,metadata_path)
        # Keep the tool's actual diagnostics on stderr. JSON is consumed as a
        # verification result; graph failures never fall back to another engine.
        result=subprocess.run(invocation,cwd=BINARY,stdout=subprocess.PIPE,check=True)
        report=json.loads(result.stdout)
    assert report['discovery']=='cfg' and name in report['functions']
    return dict(report=report,raw=result.stdout,tool_sha256=hashlib.sha256(TOOL.read_bytes()).hexdigest(),flags=FLAGS)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate')
    args=parser.parse_args()
    if args.candidate:
        import sys
        sys.stdout.buffer.write(verify_candidate(args.candidate)['raw'])
    else:
        os.execv(str(TOOL),command())


if __name__=='__main__':main()
