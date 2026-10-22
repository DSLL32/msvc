#!/usr/bin/env python3
"""Native-linked classifiers against the existing independently captured original."""
import hashlib,json,os,subprocess,time
from pathlib import Path
from importlib.machinery import SourceFileLoader
HERE=Path(__file__).resolve().parents[1]
checker=SourceFileLoader('rcpp_native_abi_checker',str(HERE/'scripts/check-native.py')).load_module()
checker.require_elf(HERE/'build/native-trace/classifier-check')
reference=json.loads((HERE/'cases/native-abi.json').read_text())['classifier']
start=time.perf_counter()
result=subprocess.run([str(HERE/'build/native-trace/classifier-check')],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
assert result.returncode==0 and not result.stderr,(result.returncode,result.stderr)
assert result.stdout==reference['stdout_latin1'].encode('latin1')
assert hashlib.sha256(result.stdout).hexdigest()==reference['stdout_sha256']
(HERE/'cases/native-classifier-validation.json').write_text(json.dumps(dict(status='passed',calls=reference['calls'],execution_seconds=time.perf_counter()-start,capture_performed=False),indent=2)+'\n')
print('Native classifier ABI results match cached original for 40048 calls.')
