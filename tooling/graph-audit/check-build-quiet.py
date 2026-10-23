#!/usr/bin/env python3
"""Focused quiet-build contract tests; fake Cargo, no tool/worker builds."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parent
scratch=Path.home()/'.ai-tmp/graph-audit'
scratch.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='quiet-build-',dir=scratch)as temporary:
    work=Path(temporary)
    cargo=work/'fake cargo'
    cargo.write_text('#!/usr/bin/env python3\nimport json,os,sys\nfrom pathlib import Path\nPath(os.environ["FAKE_RECORD"]).write_text(json.dumps({"cwd":os.getcwd(),"argv":sys.argv[1:]}))\nprint("cargo stdout diagnostic")\nprint("warning: deliberately noisy compiler",file=sys.stderr)\nsys.exit(int(os.environ.get("FAKE_STATUS","0")))\n')
    cargo.chmod(0o755)
    env=dict(os.environ,CARGO=str(cargo),FAKE_RECORD=str(work/'record.json'),GRAPH_AUDIT_BUILD_LOG=str(work/'logs/build log.txt'))
    p=subprocess.run([str(root/'build-quiet.sh')],cwd=work,env=env,capture_output=True,text=True)
    assert p.returncode==0 and not p.stdout and not p.stderr,p
    log=Path(env['GRAPH_AUDIT_BUILD_LOG']).read_text()
    assert 'cargo stdout diagnostic' in log and 'warning: deliberately noisy compiler' in log
    record=json.loads((work/'record.json').read_text())
    assert record['cwd']==str(root)
    assert record['argv']==['build','--quiet','--release','--locked','--manifest-path',str(root/'Cargo.toml'),'--target-dir',str(root/'target')]
    env['FAKE_STATUS']='37'
    p=subprocess.run([str(root/'build-quiet.sh')],cwd=work,env=env,capture_output=True,text=True)
    assert p.returncode==37 and not p.stdout
    assert 'cargo stdout diagnostic' in p.stderr and 'warning: deliberately noisy compiler' in p.stderr and 'status 37' in p.stderr
    # Relative log paths retain the caller's working-directory interpretation.
    env.update(FAKE_STATUS='0',GRAPH_AUDIT_BUILD_LOG='relative/build.log')
    p=subprocess.run([str(root/'build-quiet.sh')],cwd=work,env=env,capture_output=True,text=True)
    assert p.returncode==0 and not p.stdout and not p.stderr
    assert (work/'relative/build.log').is_file()
    # GNU Make include resolves its own directory, independent of caller cwd.
    makefile=work/'Makefile'
    makefile.write_text(f'include {root}/graph-audit.mk\n.PHONY: show\nshow:\n\t@echo "$(GRAPH_AUDIT_BIN)"\n')
    p=subprocess.run(['make','--no-print-directory','show'],cwd=work,capture_output=True,text=True,check=True)
    assert p.stdout.strip()==str(root/'target/release/graph-audit') and not p.stderr
    default=subprocess.run(['make','--no-print-directory'],cwd=work,capture_output=True,text=True,check=True)
    assert default.stdout==p.stdout and not default.stderr,default
print('Quiet build: success warnings suppressed; failure streams/status retained; any-cwd paths and Make include passed.')
