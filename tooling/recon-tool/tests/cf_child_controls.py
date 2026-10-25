"""Run inside native GDB against the authored disposable gate child only."""
import json
from pathlib import Path
import struct
import time

import gdb
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "watcom"))
from gate import Gate, Hold, Span, Function, Frame


def run(receipt_path):
    inferior=gdb.selected_inferior()
    code=int(gdb.parse_and_eval("probe_code"))
    stack=int(gdb.parse_and_eval("probe_stack"))
    marker=int(gdb.parse_and_eval("&marker"))
    pointer=int(gdb.parse_and_eval("&probe_pointer"))
    Path(str(receipt_path)+".pid").write_text(str(inferior.pid))
    results=[]
    deadline=time.monotonic()+10

    def read(address,size):
        return bytes(inferior.read_memory(address,size))

    def put(address,value):
        inferior.write_memory(address,value)

    def reg(name,value):
        gdb.invalidate_cached_frames()
        gdb.newest_frame().select()
        gdb.execute("set $%s = %d"%(name,value),to_string=True)

    def context():
        gdb.invalidate_cached_frames()
        gdb.newest_frame().select()
        return {name:int(gdb.parse_and_eval("$"+name))&0xffffffff for name in
                ("eip","esp","eax","ebx","ecx","edx","esi","edi","ebp","eflags","cs","ss","ds","es")}

    def setup(raw,target=0x100,zf=False,counter=0):
        assert time.monotonic()<deadline
        put(code,b"\x90"*4096)
        put(code,raw)
        writer=b"\xc7\x05"+struct.pack("<I",marker)+b"\x01\0\0\0\xcc"
        put(code+target,writer)
        put(marker,b"\0"*4)
        put(pointer,struct.pack("<I",code+target))
        put(stack,b"\0"*4096)
        for name,value in dict(eip=code,esp=stack+2048,eax=0,ebx=0,ecx=counter,
                               edx=0,esi=0,edi=0,ebp=0,eflags=0x202|(0x40 if zf else 0)).items():
            reg(name,value)
        return writer

    def make(raw,frames=(),functions=None,readonly=False):
        c=context()
        stk=Span(stack,4096,"actual mmap stack",write=not readonly)
        return Gate(functions or [Function("authored_probe",code,{code:raw})],read,
                    [Span(code,4096,"actual mmap code"),stk,Span(marker,4,"read-only marker")],
                    stk,1,Span(code,4096,"actual probe code image"),frames=frames,
                    flat_segments={name:c[name]&0xffff for name in ("cs","ss","ds","es")},
                    max_steps=20,max_events=50,seconds=2)

    def bypass_to_marker():
        # Deliberate test mutation, after recording the production gate's STOP.
        # This is authored probe code only; no original/reconstruction is run.
        gdb.execute("continue",to_string=True)
        assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
        assert struct.unpack("<I",read(marker,4))[0]==1

    def negative(name,raw,target=0x100,zf=False,counter=0,prepare=None,reason=None):
        setup(raw,target,zf,counter)
        options=prepare() if prepare else {}
        gate=make(raw,**options)
        before=context()
        try:
            decision=gate.plan(before,1)
        except Hold as error:
            assert reason is None or reason in str(error),(name,str(error))
            outcome="HOLD: "+str(error)
        else:
            assert decision.action=="frontier" and decision.next_pc==code+target,(name,decision)
            outcome="frontier-before-transfer"
        assert context()==before and read(marker,4)==b"\0"*4
        bypass_to_marker()
        results.append({"name":name,"gate":outcome,"marker_before":0,
                        "marker_after_deliberate_gate_bypass":1})

    negative("direct-call",b"\xe8\xfb\0\0\0")
    negative("tail-jump",b"\xe9\xfb\0\0\0")
    negative("conditional-taken",b"\x74\x1e",target=0x20,zf=True)
    negative("conditional-not-taken",b"\x74\x1e",target=2,zf=False)
    negative("loop-taken",b"\xe2\x1e",target=0x20,counter=2)
    negative("loop-not-taken",b"\xe2\x1e",target=2,counter=1)
    negative("loop-wraparound",b"\xe2\x1e",target=0x20,counter=0)

    def indirect_register():
        reg("eax",code+0x100);return {}
    negative("indirect-register",b"\xff\xe0",prepare=indirect_register)
    def indirect_memory():
        reg("eax",pointer);return {}
    negative("indirect-memory-unowned-base",b"\xff\x20",prepare=indirect_memory,reason="unowned memory")
    def foreign_return():
        sp=context()["esp"];put(sp,struct.pack("<I",code+0x100))
        return {"frames":[Frame(code+0x200,sp)]}
    negative("foreign-return",b"\xc3",prepare=foreign_return,reason="foreign return")
    def readonly_stack():
        return {"readonly":True}
    negative("push-read-only-stack",b"\x50",target=1,prepare=readonly_stack,reason="readable/writable")
    def outside_stack():
        reg("esp",pointer);return {}
    negative("pop-outside-stack",b"\x58",target=1,prepare=outside_stack,reason="stack pointer outside")
    def popfd_flags():
        put(context()["esp"],struct.pack("<I",0x202));return {}
    negative("TF-clearing-popfd",b"\x9d",target=1,prepare=popfd_flags,reason="unapproved")

    raw=b"\xc7\x05"+struct.pack("<I",marker)+b"\x01\0\0\0"
    setup(raw,target=0x100);put(code+len(raw),b"\xcc")
    gate=make(raw);before=context()
    try:gate.plan(before,1)
    except Hold as error:assert "unowned memory" in str(error)
    else:raise AssertionError("unowned store executed")
    assert read(marker,4)==b"\0"*4;bypass_to_marker()
    results.append({"name":"unowned-data-write","marker_before":0,"marker_after_deliberate_gate_bypass":1})

    # Execute the genuine CALL with a real target INT3 already installed, then
    # observe TF at the callee's first byte and inspect its actual return slot.
    raw=b"\xe8\xfb\0\0\0";writer=setup(raw)
    gate=make(raw);before=context();decision=gate.plan(before,1)
    original=read(code+0x100,1);put(code+0x100,b"\xcc")
    # Native ptrace/x86 instruction-memory coherence is the probe transport's
    # flush contract; this does NOT claim Windows FlushInstructionCache coverage.
    gate.arm_frontier(decision,before,1,original,original,True,1,True)
    gdb.execute("stepi",to_string=True)
    after=context();entry=gate.observe_frontier(after,1)
    assert entry["context"]["eip"]==code+0x100 and read(marker,4)==b"\0"*4
    assert struct.unpack("<I",read(after["esp"],4))[0]==code+5
    put(code+0x100,original);bypass_to_marker()
    results.append({"name":"actual-guarded-call-entry","actual_stack_push":4,
                    "target_byte_not_executed":True,"marker_after_deliberate_gate_bypass":1})

    # Real near CALL/RET validates the shadow return slot; no fabricated push.
    raw=b"\xe8\xfb\0\0\0";writer=setup(raw,target=5);put(code+0x100,b"\xc3")
    caller=Function("caller",code,{code:raw,code+5:writer[:-1]})
    callee=Function("callee",code+0x100,{code+0x100:b"\xc3"})
    gate=make(raw,functions=[caller,callee])
    for _ in range(2):
        plan=gate.plan(context(),1);assert plan.action=="step"
        gdb.execute("stepi",to_string=True);gate.observe(context(),1)
    assert context()["eip"]==code+5 and read(marker,4)==b"\0"*4
    bypass_to_marker()
    results.append({"name":"actual-near-call-return","shadow_frames_remaining":len(gate.frames),"marker_before_return":0})

    # Real i386 RETF consumes eight bytes; the upper selector-slot WORD remains
    # raw padding, not a formal. Same actual CS returns to the owned continuation.
    raw=b"\xcb";writer=setup(raw);c=context()
    put(c["esp"],struct.pack("<II",code+0x100,0xabcd0000|(c["cs"]&0xffff)))
    f=Function("far_return_probe",code,{code:raw,code+0x100:writer[:-1]})
    gate=make(raw,functions=[f],frames=[Frame(code+0x100,c["esp"],selector=c["cs"]&0xffff)])
    plan=gate.plan(c,1);gdb.execute("stepi",to_string=True);gate.observe(context(),1)
    assert context()["esp"]==c["esp"]+8 and read(marker,4)==b"\0"*4
    bypass_to_marker()
    results.append({"name":"actual-far-return","actual_stack_pop":8,"raw_selector_slot":hex(0xabcd0000|(c["cs"]&0xffff))})

    receipt={"scope":"actual authored native i386 child only; Win32 transport still unproved",
             "controls":results,"control_count":len(results),"owned_pid":inferior.pid,
             "all_passed":True,"application_acceptance":False}
    Path(receipt_path).write_text(json.dumps(receipt,indent=2)+"\n")
