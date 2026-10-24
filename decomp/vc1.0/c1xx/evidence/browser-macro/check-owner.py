"""Focused pre-promotion owner checks; no original execution or promotion."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

BINARY = Path(__file__).resolve().parents[2]
PROOF = BINARY/'cases/argument-observations/member_registry_browser/1266-browser_macro-4/validation.json'
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    started = time.perf_counter()
    results = []
    with tempfile.TemporaryDirectory(prefix='browser-macro-owner-', dir=Path.home()/'.ai-tmp') as scratch:
        for test in ('control', 'missing-anchor', 'wrong-caller', 'wrong-native-owner'):
            directory = Path(scratch)/test
            shutil.copytree(PROOF.parent, directory)
            proof = json.loads((directory/'validation.json').read_text())
            if test == 'missing-anchor':
                p = directory/'original-receipt.json'
                receipt = json.loads(p.read_text())
                receipt['ownership_register_observations'] = []
                p.write_text(json.dumps(receipt)+'\n')
                proof['evidence_sha256'][p.name] = sha(p)
            elif test == 'wrong-caller':
                proof['formals'][0]['binding']['original_caller_rva'] += 1
            elif test == 'wrong-native-owner':
                p = directory/'native-checkpoint.json'
                checkpoint = json.loads(p.read_text())
                checkpoint['caller_locals']['caller_definition']['value'] += 4
                p.write_text(json.dumps(checkpoint)+'\n')
                proof['evidence_sha256'][p.name] = sha(p)
            (directory/'validation.json').write_text(json.dumps(proof)+'\n')
            run = subprocess.run([sys.executable, 'scripts/advance-frontier.py', '--case', 'member_registry_browser', '--candidate', '_browser_macro@4', '--argument-proof', str(directory/'validation.json'), '--check-arguments-only'], cwd=BINARY, text=True, capture_output=True)
            assert (run.returncode == 0) == (test == 'control'), (test, run.stdout, run.stderr)
            results.append(dict(test=test, result='verified' if run.returncode == 0 else 'held', output=(run.stdout+run.stderr).strip().splitlines()[-1]))
    output = Path(__file__).with_name('owner-negative-checks.json')
    output.write_text(json.dumps(dict(scope=__doc__, proof=str(PROOF.relative_to(BINARY)), proof_sha256=sha(PROOF), checks=results, elapsed_seconds=time.perf_counter()-started), indent=2)+'\n')
    print(output)
if __name__ == '__main__': main()
