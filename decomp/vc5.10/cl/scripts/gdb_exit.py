"""Observe the actual qualified CRT exit call; never manufacture a frontier."""
import json
from pathlib import Path
import gdb
from completion_state import observe


def install(directory):
    directory=Path(directory)
    config=json.loads((directory/'config.json').read_text())
    if '_driver_exit'not in config['accepted']:return
    class Entry(gdb.Breakpoint):
        def stop(self):
            frame=gdb.newest_frame()
            assert frame.older().name()=='driver_exit'
            assert not (directory/'native-exit-gdb.json').exists(),'extra exit call'
            observe(directory,gdb,'exit',int(frame.read_var('code'))&0xffffffff)
            return False
    Entry('recon_exit',internal=True)
