#!/usr/bin/env python3
"""Offline gate controls only; not an owned-child/debug-transport proof."""
import hashlib
import json
from pathlib import Path
import struct
import time

import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"watcom"))

import capstone
from gate import Gate, Hold, Span, Function, Frame


class Fixture:
    def __init__(self, instructions, entries=None, frames=(), **options):
        self.bytes = bytearray(0x10000)
        for pc, raw in instructions.items():
            self.bytes[pc:pc + len(raw)] = raw
        self.stack = Span(0x8000, 0x1000, "observed live thread stack", write=True)
        self.context = dict(eip=0x1000, esp=0x8800, eax=0, ebx=0, ecx=0,
                            edx=0, esi=0, edi=0, ebp=0, eflags=2,
                            cs=0x23, ss=0x2b, ds=0x2b, es=0x2b)
        functions = entries or [Function("probe", 0x1000, instructions)]
        self.gate = Gate(functions, lambda a, n: self.bytes[a:a+n],
                         [Span(0x1000, 0x1000, "code"),
                          Span(0x3000, 0x100, "owned global", write=True), self.stack],
                         self.stack, 7, Span(0x1000,0x3000,"application image"), frames=frames,
                         flat_segments={"cs": 0x23, "ss": 0x2b, "ds": 0x2b, "es": 0x2b},
                         **options)

    def plan(self):
        return self.gate.plan(self.context, 7)

    def observe(self, plan):
        result = dict(self.context, eip=plan.next_pc, esp=plan.next_sp, cs=plan.next_cs)
        self.gate.observe(result, 7)
        self.context = result


def main():
    results = []

    def positive(name, callback):
        callback()
        results.append({"name": name, "result": "pass"})

    def reject(name, callback, fragment):
        try:
            callback()
        except Hold as error:
            assert fragment in str(error), (name, str(error), fragment)
            results.append({"name": name, "result": "rejected", "reason": str(error)})
        else:
            raise AssertionError("control did not reject: " + name)

    def frontier(name, fixture, target):
        before = bytes(fixture.bytes), dict(fixture.context)
        result = fixture.plan()
        assert result.action == "frontier" and result.next_pc == target, (name, result)
        assert before == (bytes(fixture.bytes), fixture.context)
        assert fixture.gate.pending is None
        results.append({"name": name, "result": "frontier-before-transfer"})

    f = Fixture({0x1000: b"\x90", 0x1001: b"\x90"})
    positive("linear-step-observed", lambda: f.observe(f.plan()))
    frontier("unclassified-entry", Fixture({0x1100: b"\x90"}), None)
    frontier("direct-call-missing-target", Fixture({0x1000: b"\xe8\xfb\x0f\0\0"}), 0x2000)
    frontier("escaped-fallthrough", Fixture({0x1000: b"\x90"}), 0x1001)
    frontier("tail-jump-missing-target", Fixture({0x1000: b"\xe9\xfb\x0f\0\0"}), 0x2000)
    f = Fixture({0x1000: b"\xff\xe0"}); f.context["eax"] = 0x2000
    frontier("indirect-register-missing-target", f, 0x2000)
    f = Fixture({0x1000:b"\xff\xe0"});f.context["eax"]=0x70000000
    reject("external-service-is-not-application-frontier", f.plan, "proved service boundary")
    f = Fixture({0x1000: b"\xff\x20"}); f.context["eax"] = 0x3100
    reject("indirect-memory-wrong-base-owner", f.plan, "unowned memory")

    for flag, target in ((0x42, 0x1012), (2, 0x1002)):
        f = Fixture({0x1000: b"\x74\x10"}); f.context["eflags"] = flag
        frontier("conditional-" + ("taken" if flag & 0x40 else "not-taken"), f, target)
    for counter, target in ((2, 0x1012), (1, 0x1002), (0, 0x1012)):
        f = Fixture({0x1000: b"\xe2\x10"}); f.context["ecx"] = counter
        frontier("loop-counter-%d" % counter, f, target)

    call = {0x1000: b"\xe8\xfb\0\0\0", 0x1005: b"\x90", 0x1100: b"\xc3"}
    f = Fixture(call, entries=[Function("caller", 0x1000, {p:b for p,b in call.items() if p < 0x1100}),
                              Function("callee", 0x1100, {0x1100:b"\xc3"})])
    p = f.plan(); assert p.action == "step" and p.next_pc == 0x1100
    struct.pack_into("<I", f.bytes, p.next_sp, 0x1005); f.observe(p)
    positive("near-call-live-return-frame", lambda: f.observe(f.plan()))
    assert not f.gate.frames

    f = Fixture({0x1000:b"\xc3", 0x1005:b"\x90"}, frames=[Frame(0x1005,0x8800)])
    struct.pack_into("<I", f.bytes, 0x8800, 0x1006)
    reject("foreign-return", f.plan, "foreign return")
    f = Fixture({0x1000:b"\xc2\x04\0", 0x1005:b"\x90"}, frames=[Frame(0x1005,0x8800)])
    struct.pack_into("<I", f.bytes, 0x8800, 0x1005)
    reject("return-wrong-cleanup", f.plan, "cleanup mismatch")
    f = Fixture({0x1000:b"\xc3"})
    reject("return-without-call", f.plan, "without a live call")
    f = Fixture({0x1000:b"\x50",0x1001:b"\x90"})
    f.gate.stack = Span(0x8000,0x1000,"read-only stack",write=False)
    reject("push-read-only-stack", f.plan, "readable/writable owned stack")
    f = Fixture(call, entries=[Function("caller",0x1000,{0x1000:call[0x1000],0x1005:b"\x90"}),Function("callee",0x1100,{0x1100:b"\xc3"})])
    f.gate.spans = [s for s in f.gate.spans if s is not f.stack]
    f.gate.spans.append(Span(0x8800,0x800,"stack gap excludes implicit call write",write=True))
    reject("call-slot-unowned", f.plan, "unowned memory")
    f = Fixture({0x1000:b"\x50",0x1001:b"\x90"})
    f.gate.stack = Span(0x1000,0x8000,"bad stack overlaps code",write=True)
    f.gate.spans.append(f.gate.stack);f.context["esp"]=0x1004
    reject("push-overlaps-code", f.plan, "overlaps owned instruction")
    f = Fixture({0x1000:b"\x58",0x1001:b"\x90"});f.context["esp"]=0x3000
    reject("pop-from-owned-global-outside-stack", f.plan, "live stack pointer outside")

    f = Fixture({0x1000:b"\xcb", 0x1005:b"\x90"}, frames=[Frame(0x1005,0x8800,selector=0x23)])
    struct.pack_into("<II", f.bytes, 0x8800, 0x1005, 0xabcd0023)
    positive("far-return-selector-padding-unread", lambda: f.observe(f.plan()))
    f = Fixture({0x1000:b"\xcb", 0x1005:b"\x90"}, frames=[Frame(0x1005,0x8800,selector=0x23)])
    struct.pack_into("<II", f.bytes, 0x8800, 0x1005, 0x33)
    reject("foreign-far-selector", f.plan, "foreign far return selector")

    for code, name, fragment in ((b"\x9d", "TF-clearing-popfd", "unapproved"),
                                  (b"\x8e\xd0", "TF-suppressing-SS-load", "segment change"),
                                  (b"\xff\x1d\x00\x30\0\0", "unproved-far-service-call", "unapproved")):
        reject(name, Fixture({0x1000:code}).plan, fragment)
    f = Fixture({0x1000:b"\x90",0x1001:b"\x90"}); f.bytes[0x1000] = 0xcc
    reject("changed-relocated-instruction", f.plan, "bytes changed")
    f = Fixture({0x1000:b"\xc7\x05\x00\x31\0\0\x01\0\0\0",0x100a:b"\x90"})
    reject("unowned-data-store", f.plan, "unowned memory")
    f = Fixture({0x1000:b"\xc7\x05\x00\x10\0\0\x01\0\0\0",0x100a:b"\x90"})
    f.gate.spans.append(Span(0x1000,0x1000,"overbroad writable span",write=True))
    reject("self-modifying-code-store", f.plan, "overlaps owned instruction")
    f = Fixture({0x1000:b"\x65\xa1\x00\x30\0\0",0x1006:b"\x90"})
    reject("unproved-GS-base", f.plan, "unproved segment")
    f = Fixture({0x1000:b"\x64\xa1\x00\x30\0\0",0x1006:b"\x90"})
    f.gate.flat_segments["fs"] = 0x53;f.context["fs"]=0x53
    reject("FS-selector-does-not-prove-flat-base", f.plan, "unproved segment")
    f = Fixture({0x1000:b"\x90",0x1001:b"\x90"}); f.context["ds"] = 0x33
    # A scalar instruction need not use DS; a real memory operand must reject it.
    f = Fixture({0x1000:b"\xa1\x00\x30\0\0",0x1005:b"\x90"});f.context["ds"] = 0x33
    reject("changed-flat-selector", f.plan, "unproved segment")
    f = Fixture({0x1000:b"\x90",0x1001:b"\x90"})
    reject("async-thread", lambda:f.gate.event("step",8), "unapproved thread")
    reject("callback-debug-event", lambda:f.gate.event("callback",7), "unapproved thread")
    reject("unexpected-exception", lambda:f.gate.event("exception",7), "unapproved thread")
    f = Fixture({0x1000:b"\x90",0x1001:b"\x90"})
    f.plan()
    reject("missed-single-step-event", lambda:f.gate.observe(dict(f.context,eip=0x1002),7), "differs from plan")
    f = Fixture({0x1000:b"\x90",0x1001:b"\x90"}, max_steps=1)
    f.observe(f.plan())
    reject("instruction-budget", f.plan, "instruction budget")
    ticks = [0.0]
    f = Fixture({0x1000:b"\x90"}, clock=lambda:ticks[0], seconds=1)
    ticks[0] = 2
    reject("deadline-every-plan", f.plan, "deadline budget")
    f = Fixture({0x1000:b"\x90",0x1001:b"\x90"}, max_events=1)
    p=f.plan()
    reject("event-budget-every-observation", lambda:f.observe(p), "event/deadline budget")
    reject("missing-observation", lambda:(lambda x:(x.plan(),x.plan()))(Fixture({0x1000:b"\x90",0x1001:b"\x90"})), "no observed result")

    def armed():
        f=Fixture({0x1000:b"\xe8\xfb\x0f\0\0"})
        d=f.plan();f.bytes[0x2000]=0xcc
        f.gate.arm_frontier(d,f.context,7,b"\x90",b"\x90",True,1,True)
        after=dict(f.context,eip=0x2000,esp=0x87fc)
        struct.pack_into("<I",f.bytes,0x87fc,0x1005)
        return f,d,after

    f,d,after=armed()
    positive("guarded-frontier-actual-entry-shape",lambda:f.gate.observe_frontier(after,7))
    reject("cannot-run-unknown-frontier-body",lambda:f.gate.plan(after,7),"no function body")
    f=Fixture({0x1000:b"\xe8\xfb\x0f\0\0"});d=f.plan()
    reject("missing-real-target-trap",lambda:f.gate.arm_frontier(d,f.context,7,b"\x90",b"\x90",True,1,True),"not actually installed")
    f=Fixture({0x1000:b"\xe8\xfb\x0f\0\0"});d=f.plan();f.bytes[0x2000]=0xcc
    reject("frontier-wrong-prebyte",lambda:f.gate.arm_frontier(d,f.context,7,b"\x90",b"\x91",True,1,True),"prebytes/write/flush")
    reject("frontier-failed-write",lambda:f.gate.arm_frontier(d,f.context,7,b"\x90",b"\x90",False,1,True),"prebytes/write/flush")
    reject("frontier-failed-flush",lambda:f.gate.arm_frontier(d,f.context,7,b"\x90",b"\x90",True,1,False),"prebytes/write/flush")
    f,d,after=armed();after["esp"]+=4
    reject("frontier-no-real-call-push",lambda:f.gate.observe_frontier(after,7),"target/SP/CS differs")
    f,d,after=armed();struct.pack_into("<I",f.bytes,0x87fc,0x1006)
    reject("frontier-foreign-call-return",lambda:f.gate.observe_frontier(after,7),"CALL return slot differs")
    f,d,after=armed();after["cs"]=0x33
    reject("frontier-wrong-CS",lambda:f.gate.observe_frontier(after,7),"target/SP/CS differs")
    f,d,after=armed()
    reject("frontier-unexpected-breakpoint-instead-of-step",lambda:f.gate.observe_frontier(dict(after,eip=0x2001),7,"breakpoint"),"unapproved thread")
    f=Fixture({0x1000:b"\xc3"},frames=[Frame(0x2000,0x8800)])
    struct.pack_into("<I",f.bytes,0x8800,0x2000)
    reject("unknown-return-is-HOLD-not-new-function",f.plan,"not proved code")

    receipt = {"scope":"offline gate controls only; no debugger/child/app acceptance",
               "capstone":capstone.__version__,"controls":results,
               "gate_sha256":hashlib.sha256((Path(__file__).resolve().parents[1]/'watcom/gate.py').read_bytes()).hexdigest(),
               "checker_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "control_count":len(results)}
    print(json.dumps(receipt,indent=2))


if __name__ == "__main__":
    main()
