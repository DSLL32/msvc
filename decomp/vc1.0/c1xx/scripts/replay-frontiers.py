"""Ordinary C1XX histories use continuous replay; --fresh retains the full oracle."""
import sys
import subprocess
from pathlib import Path

if __name__ == '__main__':
    if '--fresh' in sys.argv:
        arguments = [a for a in sys.argv[1:] if a != '--fresh']
        raise SystemExit(subprocess.call(
            [sys.executable, str(Path(__file__).with_name('replay-fresh.py'))] + arguments))
    from fast_replay_adapter import main
    main()
