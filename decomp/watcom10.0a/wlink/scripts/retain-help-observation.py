#!/usr/bin/env python3
"""Derive public evidence from the SINGLE existing private help observation.
Never launches an executable or changes the raw evidence.
"""
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = Path('/root/.ai-tmp/msvc2/watcom10.0a/wlink/help-observer-01')
DEST = ROOT / 'cases/help/observer-capture-01'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    receipt = json.loads((CAPTURE / 'receipt.json').read_text())
    raw_path = CAPTURE / 'events.private.txt'
    raw = raw_path.read_bytes()
    assert sha(raw) == receipt['raw_events_private']['sha256']
    assert raw_path.stat().st_mode & 0o777 == 0o600
    lines = raw.decode('ascii').splitlines(keepends=True)
    derived = []
    redactions = []
    calls = Counter()
    paths = []
    for number, line in enumerate(lines, 1):
        if line.startswith('bytes kind=actual-Windows-environment-private '):
            prefix, payload = line.rstrip('\n').split('hex=', 1)
            block = bytes.fromhex(payload)
            redactions.append({'raw_line': number, 'bytes': len(block), 'sha256': sha(block)})
            line = prefix + 'hex=REDACTED_PRIVATE_ENVIRONMENT sha256=' + sha(block) + '\n'
        if line.startswith('api-enter '):
            calls[line.split('name=', 1)[1].split()[0]] += 1
        if line.startswith('bytes kind=actual-path '):
            paths.append(bytes.fromhex(line.split('hex=', 1)[1]).rstrip(b'\0').decode('latin1'))
        derived.append(line)
    assert len(redactions) == 1
    assert lines[-1].startswith('observer-complete API-events=1109 ')
    DEST.mkdir(exist_ok=False)
    public = ('DERIVED REDACTED VIEW; not raw evidence. Only the explicitly named '
              'actual Windows environment hex block is replaced by its SHA256.\n'
              'Raw private file/hash/permissions are retained in receipt.json.\n' + ''.join(derived)).encode('ascii')
    compressed = gzip.compress(public, mtime=0)
    (DEST / 'events.derived-redacted.txt.gz').write_bytes(compressed)
    for name in ('stdout.bin', 'stderr.bin', 'launcher.stdout.bin', 'launcher.stderr.bin'):
        data = (CAPTURE / name).read_bytes()
        assert sha(data) == receipt['artifacts'][name]['sha256']
        shutil.copyfile(CAPTURE / name, DEST / name)
    receipt['derived_view'] = {
        'file': 'events.derived-redacted.txt.gz', 'raw_private_file_unchanged': True,
        'uncompressed_bytes': len(public), 'uncompressed_sha256': sha(public),
        'stored_bytes': len(compressed), 'stored_sha256': sha(compressed),
        'redactions': redactions,
        'policy': 'Clearly derived redacted view; no claim that it is the raw log.'}
    receipt['observations'] = {
        'main': next(s.strip() for s in lines if s.startswith('actual-main ')),
        'api_entries': dict(calls), 'api_total': sum(calls.values()),
        'paths_passed_to_CreateFileA': paths,
        'debugger_completion': lines[-1].strip(),
        'input': 'Win32 NUL character handle 0x34; one successful ReadFile(1 byte) returned a defined zero count at pagination; GetConsoleMode and both SetConsoleMode calls failed, raw output-mode bytes remain uncredited.',
        'files': 'Own patched executable was opened/read for appended resources. Five wlink.hlp attempts failed. No WLINK.LNK/WLSYSTEM.LNK CreateFileA calls were observed in this help invocation; imported-API observations are not a proof excluding arbitrary direct/native DLL dependencies.',
        'comparison': 'stdout equals the earlier baseline by SHA256; argv/cwd/environment/handle transport are explicitly a new observation, not relabeled old input evidence.',
        'acceptance': 'Unmasked diagnostic only, not an application-function acceptance or masked replay.'}
    (DEST / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'retained': str(DEST), 'api_entries': sum(calls.values()),
                      'redacted_blocks': len(redactions), 'compressed_bytes': len(compressed)}, indent=2))

if __name__ == '__main__':
    main()
