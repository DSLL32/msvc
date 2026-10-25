#!/usr/bin/env python3
"""Adaptive waits preserve bounded idle behavior; no child or Wine launch."""
from pathlib import Path
import sys
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'watcom'))
from ipc import reply_wait
for elapsed, expected in [(0.00001, 'spin'), (0.00006, 'yield'), (0.0019, 'yield'), (0.0021, 'sleep'), (1., 'sleep')]:
    with patch('ipc.time.monotonic', return_value=elapsed), patch('ipc.time.sleep') as sleep, patch('ipc.os.sched_yield') as yield_cpu:
        reply_wait(0.)
        assert sleep.call_count == (expected == 'sleep')
        assert yield_cpu.call_count == (expected == 'yield')
        if sleep.called:
            sleep.assert_called_once_with(0.001)
print('CF adaptive spin/yield/sleep thresholds PASS')
