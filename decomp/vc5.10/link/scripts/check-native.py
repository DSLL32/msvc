#!/usr/bin/env python3
"""Native-only fixture. An incomplete first case must fail closed."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
config = json.loads((root / 'functions.json').read_text())
if not config.get('cases', {}).get('help', {}).get('complete'):
    raise SystemExit('Help is not accepted: named traps remain; no fixture success claimed.')
raise SystemExit('Completed-help comparator is not installed; refusing success.')
