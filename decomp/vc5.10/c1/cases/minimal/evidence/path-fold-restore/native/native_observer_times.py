"""Coarse GDB collector accounting; no target or observation changes."""
import json,time
import gdb

began=time.perf_counter()
phases={}
running=None
stopped=None
first_run=None
original_dumps=json.dumps

def measured(label):
 def decorate(function):
  def call(*args,**kwargs):
   started=time.perf_counter()
   try:return function(*args,**kwargs)
   finally:
    row=phases.setdefault(label,dict(seconds=0.0,calls=0))
    row['seconds']+=time.perf_counter()-started;row['calls']+=1
  return call
 return decorate

json.dumps=measured('json_serialization')(json.dumps)

def continued(event):
 global running,stopped,first_run
 now=time.perf_counter()
 if first_run is None:first_run=now
 if stopped is not None:phases.setdefault('stopped_collection_restore',dict(seconds=0.0,calls=0))['seconds']+=now-stopped
 running=now;stopped=None

def stopped_event(event):
 global running,stopped
 now=time.perf_counter()
 if running is not None:
  row=phases.setdefault('run_including_callbacks',dict(seconds=0.0,calls=0));row['seconds']+=now-running;row['calls']+=1
 running=None;stopped=now

gdb.events.cont.connect(continued)
gdb.events.stop.connect(stopped_event)

def finish(path,launch):
 now=time.perf_counter()
 if stopped is not None:phases.setdefault('stopped_collection_restore',dict(seconds=0.0,calls=0))['seconds']+=now-stopped
 value=dict(gdb_startup_seconds=began-launch,source_loading_seconds=(first_run or now)-began,
  phases=phases,seconds_since_first_python=now-began,
  scope='Run includes breakpoint callbacks. Callback/state/x87/serialization totals overlap and must not be added. Stopped time includes explicit restore/read/serialization commands. No collector optimization.')
 with open(path,'w')as output:output.write(original_dumps(value,indent=2)+'\n')
