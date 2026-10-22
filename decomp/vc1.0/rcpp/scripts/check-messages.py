#!/usr/bin/env python3
"""Native-linked catalog text/pointer identities against the original digest."""
import hashlib,json,subprocess,tempfile,time
from pathlib import Path
from importlib.machinery import SourceFileLoader
HERE=Path(__file__).resolve().parents[1]
checker=SourceFileLoader('rcpp_native_abi_checker',str(HERE/'scripts/check-native.py')).load_module()
checker.require_elf(HERE/'build/native-trace/message-check')
reference=json.loads((HERE/'cases/native-abi.json').read_text())['message']
start=time.perf_counter()
with tempfile.TemporaryDirectory(dir=Path.home()/'.ai-tmp/vc1.0-rcpp')as name:
    output=Path(name)/'messages.bin'
    result=subprocess.run([str(HERE/'build/native-trace/message-check'),str(output)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
    assert result.returncode==0 and not result.stdout and not result.stderr,(result.returncode,result.stdout,result.stderr)
    digest=hashlib.sha256(output.read_bytes()).hexdigest()
    assert digest==reference['result_sha256'],(digest,reference['result_sha256'])
(HERE/'cases/native-message-validation.json').write_text(json.dumps(dict(status='passed',calls=reference['calls'],result_sha256=digest,execution_seconds=time.perf_counter()-start,capture_performed=False),indent=2)+'\n')
print('Native message ABI/text/identity match cached original for 131080 calls.')
