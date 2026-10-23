"""Source/build policy binding shared by capture and acceptance, no execution."""
from pathlib import Path
import hashlib
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
def fingerprints():
    files=[BINARY/'Makefile']
    for parent in [BINARY/'src',BINARY/'scripts',ROOT/'lib/recon/src',ROOT/'lib/recon/include',ROOT/'lib/recon/native']:
        files.extend(p for p in parent.rglob('*') if p.is_file() and p.suffix in ('.c','.h','.py','.rc'))
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(set(files))}
