#!/usr/bin/env python3
"""Scoped graph/import checks and strict RET checks on every real body."""
import json
from accept import BINARY,audit_trial
report=audit_trial(BINARY/'functions.json')
(BINARY/'build/graph-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(discovery='cfg',full_vertices=len(report['full_report']['functions']),
    strict_ret_bodies=len(report['strict_report']['functions']),
    elapsed_seconds=report['elapsed_seconds']),indent=2))
