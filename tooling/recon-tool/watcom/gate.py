"""Fail-closed runtime x86 instruction gate; not a static image walker.

The transport must call plan BEFORE execution and observe after exactly one
instruction. No result here authorizes free-running an OS/CRT operation.
"""
from dataclasses import dataclass
import bisect
import math
import struct
import time

from capstone import __version__ as CAPSTONE_VERSION
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM, X86_OP_REG


class Hold(RuntimeError):
    """Execution must remain stopped; this is not application acceptance."""


@dataclass(frozen=True)
class Span:
    start: int
    size: int
    name: str
    read: bool = True
    write: bool = False

    def contains(self, address, size):
        return (0 <= address <= 0xffffffff and size > 0 and
                address + size <= 0x100000000 and
                self.start <= address and address + size <= self.start + self.size)


@dataclass(frozen=True)
class Function:
    name: str
    entry: int
    instructions: dict  # actual relocated address -> exact instruction bytes
    cleanup: int | None = 0


@dataclass(frozen=True)
class Frame:
    return_pc: int
    stack_pointer: int
    cleanup: int = 0
    selector: int | None = None  # None is near; low WORD meaningful for far.


@dataclass(frozen=True)
class Decision:
    action: str
    pc: int
    next_pc: int | None
    next_sp: int | None
    reason: str
    next_cs: int | None = None
    transfer: str = "linear"
    call_return: int | None = None
    writes: tuple = ()
    frame: Frame | None = None
    pop_frame: bool = False
    register_checks: tuple = ()


class Gate:
    def __init__(self, functions, memory, spans, stack, thread_id, application_image,
                 frames=(), flat_segments=None, max_steps=100000, max_events=110000,
                 seconds=30.0, clock=time.monotonic, startup=False, zero_span=None):
        if CAPSTONE_VERSION != '5.0.7':
            raise Hold("instruction admission requires qualified Capstone 5.0.7")
        self.startup = startup
        self.zero_span = zero_span
        self.functions = list(functions)
        self.memory = memory
        self.spans = list(spans)
        self.stack = stack
        self.application_image = application_image
        self.thread_id = thread_id
        self.flat_segments = dict(flat_segments or {})
        if (not self.flat_segments or set(self.flat_segments) - {"cs", "ss", "ds", "es"} or
                any(not 0 <= v <= 0xffff for v in self.flat_segments.values())):
            raise Hold("actual flat segment selectors must be supplied")
        self.frames = list(frames)
        self.clock = clock
        self.deadline = clock() + seconds
        if not (0 < max_steps <= 1000000 and 0 < max_events <= 2000000 and
                math.isfinite(seconds) and 0 < seconds <= 300):
            raise Hold("invalid capture budgets")
        self.max_steps, self.max_events = max_steps, max_events
        self.steps = self.events = 0
        self.pending = None
        self.frontier = None
        self.stopped_at_frontier = False
        self.decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        self.decoder.detail = True
        self.code = {}
        self.entries = {}
        for function in self.functions:
            if function.entry in self.entries:
                raise Hold("duplicate function entry")
            self.entries[function.entry] = function
            for pc, raw in function.instructions.items():
                if pc in self.code or not raw or len(raw) > 15:
                    raise Hold("ambiguous or invalid instruction ownership")
                self.code[pc] = (function, bytes(raw))
        self.starts = sorted(self.code)
        end = 0
        for pc in self.starts:
            if pc < end or pc + len(self.code[pc][1]) > 0x100000000:
                raise Hold("overlapping or overflowing instruction ownership")
            end = pc + len(self.code[pc][1])

    def event(self, kind, thread_id):
        self.events += 1
        if self.events > self.max_events or self.clock() >= self.deadline:
            raise Hold("capture event/deadline budget exhausted")
        if thread_id != self.thread_id or kind not in {"step", "initial-stop"}:
            raise Hold("unapproved thread, callback, exception or debug event")

    def access(self, address, size, write=False):
        if self.clock() >= self.deadline:
            raise Hold("capture deadline exhausted during memory operation")
        if not any(s.contains(address, size) and s.read and (not write or s.write)
                   for s in self.spans):
            raise Hold("unowned memory read/write at %#x" % address)
        nearby = self.starts[max(0, bisect.bisect_right(self.starts, address) - 1):
                             bisect.bisect_left(self.starts, address + size)]
        if write and any(address < p + len(self.code[p][1]) and p < address + size
                         for p in nearby):
            raise Hold("application store overlaps owned instruction bytes")

    def read(self, address, size, write=False):
        self.access(address, size, write)
        try:
            result = bytes(self.memory(address, size))
        except Exception as error:
            raise Hold("transport memory read failed") from error
        if len(result) != size:
            raise Hold("short memory read")
        if self.clock() >= self.deadline:
            raise Hold("capture deadline exhausted after memory operation")
        return result

    def stack_access(self, address, size, write=False):
        if not self.stack.read or not self.stack.contains(address, size) or (write and not self.stack.write):
            raise Hold("implicit access outside readable/writable owned stack")
        self.access(address, size, write)

    def unclassified(self, context, pc, target, next_sp, reason, transfer="linear", call_return=None, writes=()):
        if target is not None:
            if not self.application_image.contains(target, 1):
                raise Hold("external transfer requires a proved service boundary")
            prior = bisect.bisect_right(self.starts, target) - 1
            if prior >= 0 and target < self.starts[prior] + len(self.code[self.starts[prior]][1]):
                raise Hold("transfer targets middle of an owned instruction")
        result = Decision("frontier", pc, target, next_sp, reason,
                          next_cs=self.reg(context, "cs") & 0xffff,
                          transfer=transfer, call_return=call_return,writes=tuple(writes))
        self.frontier = (result, dict(context))
        return result

    @staticmethod
    def reg(context, name):
        if name not in context or not 0 <= context[name] <= 0xffffffff:
            raise Hold("missing/non-DWORD register: " + name)
        return context[name]

    def address(self, instruction, operand, context):
        value = operand.mem.disp
        for register, scale in ((operand.mem.base, 1), (operand.mem.index, operand.mem.scale)):
            if register:
                value += self.reg(context, instruction.reg_name(register)) * scale
        segment = (instruction.reg_name(operand.mem.segment) if operand.mem.segment else
                   ("ss" if instruction.reg_name(operand.mem.base) in {"esp", "ebp"} else "ds"))
        if (segment not in {"cs", "ss", "ds", "es"} or segment not in self.flat_segments or
                (self.reg(context, segment) & 0xffff) != self.flat_segments[segment]):
            raise Hold("unproved segment/TLS memory base")
        return value & 0xffffffff

    def operand(self, instruction, operand, context):
        if operand.type == X86_OP_IMM:
            return operand.imm & 0xffffffff
        if operand.type == X86_OP_REG:
            return self.reg(context, instruction.reg_name(operand.reg))
        if operand.type == X86_OP_MEM and operand.size == 4:
            return struct.unpack("<I", self.read(self.address(instruction, operand, context), 4))[0]
        raise Hold("unsupported transfer operand")

    @staticmethod
    def condition(name, flags, ecx):
        cf, pf = bool(flags & 1), bool(flags & 4)
        zf, sf, of = bool(flags & 0x40), bool(flags & 0x80), bool(flags & 0x800)
        conditions = {"jo": of, "jno": not of, "jb": cf, "jae": not cf,
                      "je": zf, "jne": not zf, "jbe": cf or zf,
                      "ja": not cf and not zf, "js": sf, "jns": not sf,
                      "jp": pf, "jnp": not pf, "jl": sf != of,
                      "jge": sf == of, "jle": zf or sf != of,
                      "jg": not zf and sf == of, "jecxz": ecx == 0}
        if name in conditions:
            return conditions[name]
        if name in {"loop", "loope", "loopne"}:
            nonzero = ((ecx - 1) & 0xffffffff) != 0
            return nonzero and (name == "loop" or (zf if name == "loope" else not zf))
        raise Hold("unsupported conditional transfer")

    def plan(self, context, thread_id):
        self.event("step", thread_id)
        if self.stopped_at_frontier:
            raise Hold("frontier entry is stopped; no function body is authorized")
        if self.pending is not None:
            raise Hold("previous instruction has no observed result")
        if self.steps >= self.max_steps:
            raise Hold("capture instruction budget exhausted")
        pc, sp = self.reg(context, "eip"), self.reg(context, "esp")
        if not self.stack.start <= sp <= self.stack.start + self.stack.size:
            raise Hold("live stack pointer outside observed stack")
        if pc not in self.code:
            if not self.application_image.contains(pc, 1):
                raise Hold("external instruction requires a proved service boundary")
            return self.unclassified(context,pc,None,None,"unclassified instruction entry")
        owner, raw = self.code[pc]
        for segment in ("cs", "ss"):
            if segment not in self.flat_segments or (self.reg(context, segment) & 0xffff) != self.flat_segments[segment]:
                raise Hold("code/stack segment differs from observed flat profile")
        if self.read(pc, len(raw)) != raw:
            raise Hold("relocated instruction bytes changed")
        decoded = list(self.decoder.disasm(raw, pc, count=1))
        if len(decoded) != 1 or decoded[0].size != len(raw):
            raise Hold("invalid byte-bound instruction descriptor")
        instruction = decoded[0]
        name, operands = instruction.mnemonic, instruction.operands
        # Capstone 5.0.7 labels FF /3 and /5 as ordinary call/jmp with
        # DWORD operands. Their real operands/frames are FAR; never trust
        # that near-call presentation when deciding stack or pointer reads.
        if instruction.opcode[0] == 0xff and (instruction.modrm >> 3) & 7 in {3, 5}:
            raise Hold("unapproved indirect far/service transfer")
        end = pc + instruction.size
        if end > 0xffffffff:
            raise Hold("instruction address overflow")
        repeated = self.startup and raw in {b"\xf3\xab", b"\xf3\xaa"}
        word_mov = (self.startup and name == "mov" and 0x66 in instruction.prefix and
                    len(operands) == 2 and all(op.size == 2 for op in operands))
        if (any(p in {0x67, 0xf0, 0xf2} for p in instruction.prefix) or
                (0x66 in instruction.prefix and not word_mov) or
                (0xf3 in instruction.prefix and not repeated)):
            raise Hold("unproved size/locking/repeated instruction")
        if repeated:
            return self.plan_zero(context, owner, raw, end, sp)
        if self.startup and raw == b"\xac":
            return self.plan_lodsb(context, owner, end, sp)
        if name.startswith(("int", "iret", "sys", "rep")) or name in {
                "popf", "popfd", "pushf", "pushfd", "cli", "sti", "hlt",
                "lcall", "ljmp", "leave", "xlatb"}:
            raise Hold("unapproved trap, far/service or TF/implicit-stack operation")
        linear = {"mov", "movzx", "movsx", "lea", "add", "sub", "adc", "sbb",
                  "and", "or", "xor", "test", "cmp", "inc", "dec", "not", "neg",
                  "shl", "shr", "sar", "sal", "rol", "ror", "rcl", "rcr",
                  "imul", "mul", "idiv", "div", "cdq", "cwd", "cwde", "cbw",
                  "xchg", "nop", "clc", "stc", "cmc", "cld", "std", "lahf",
                  "sahf", "push", "pop", "enter"}
        if name not in linear | {"call", "ret", "retf", "jmp"} and not name.startswith(("j", "loop", "set", "cmov")):
            raise Hold("unsupported instruction semantics")
        # All currently supported data segments are explicitly flat. A transport
        # must establish this fact; FS/GS and segment-register writes stay closed.
        _, written = instruction.regs_access()
        written_names = {instruction.reg_name(r) for r in written}
        segment_pop = (self.startup and raw == b"\x07")
        if written_names & {"cs", "ss", "ds", "es", "fs", "gs"} and not segment_pop:
            raise Hold("segment change can invalidate flat memory or suppress TF")
        for op in operands:
            if op.type == X86_OP_REG and instruction.reg_name(op.reg) in {"cs", "ss", "ds", "es", "fs", "gs"}:
                segment = instruction.reg_name(op.reg)
                if not self.startup or segment not in {"ds", "es", "ss"}:
                    raise Hold("unproved segment-register operation")
                if self.reg(context, segment) != self.flat_segments[segment]:
                    raise Hold("segment selector differs from observed flat descriptor")
        writes=[]
        for operand in operands:
            if operand.type != X86_OP_MEM or name == "lea":
                continue
            if not operand.access or not operand.size:
                raise Hold("unknown memory access semantics")
            address = self.address(instruction, operand, context)
            self.read(address, operand.size, bool(operand.access & CS_AC_WRITE))
            if operand.access & CS_AC_WRITE:
                writes.append((address,operand.size))

        next_pc, next_sp = end, sp
        frame, pop_frame, transfer = None, False, "linear"
        register_checks = ()
        if name == "call":
            transfer = "call"
            if len(operands) != 1:
                raise Hold("unsupported call form")
            next_pc = self.operand(instruction, operands[0], context)
            next_sp = sp - 4
            self.stack_access(next_sp, 4, write=True)
            writes.append((next_sp,4))
            if next_pc not in self.entries:
                return self.unclassified(context,pc,next_pc,next_sp,"unclassified call target",
                                         transfer="call",call_return=end,writes=writes)
            if self.entries[next_pc].cleanup is None:
                raise Hold("CALL target has no proved return ABI")
            frame = Frame(end, next_sp, self.entries[next_pc].cleanup)
        elif name in {"ret", "retf"}:
            transfer = "return"
            if not self.frames:
                raise Hold("return without a live call frame")
            expected = self.frames[-1]
            cleanup = operands[0].imm if operands else 0
            far = expected.selector is not None
            size = 8 if far else 4
            if (name == "retf") != far or sp != expected.stack_pointer or cleanup != expected.cleanup:
                raise Hold("return shape, live slot or cleanup mismatch")
            self.stack_access(sp, size + cleanup)
            data = self.read(sp, size)
            next_pc = struct.unpack_from("<I", data)[0]
            if next_pc != expected.return_pc:
                raise Hold("foreign return target")
            if next_pc not in self.code:
                raise Hold("return continuation is not proved code")
            if far and struct.unpack_from("<H", data, 4)[0] != expected.selector:
                raise Hold("foreign far return selector")
            next_sp, pop_frame = sp + size + cleanup, True
        elif name == "jmp" or name.startswith("j") or name.startswith("loop"):
            transfer = "branch"
            if len(operands) != 1:
                raise Hold("unsupported branch form")
            taken = name == "jmp" or self.condition(name, self.reg(context, "eflags"), self.reg(context, "ecx"))
            next_pc = self.operand(instruction, operands[0], context) if taken else end
        elif name == "enter":
            # Only the reached 32-bit, zero-nesting frame form. Every reserved
            # byte must remain within the observed writable thread stack.
            if len(raw) != 4 or raw[0] != 0xc8 or raw[3] != 0:
                raise Hold("unproved ENTER nesting or width")
            if end not in self.code:
                raise Hold("ENTER continuation must have an owned instruction")
            reservation = struct.unpack_from("<H", raw, 1)[0]
            saved_bp = sp - 4
            next_sp = saved_bp - reservation
            self.stack_access(next_sp, reservation + 4, write=True)
            writes.append((saved_bp, 4))
            register_checks = (("ebp", saved_bp),
                               ("stack_dword", saved_bp, self.reg(context, "ebp")),
                               ("flags_unchanged", self.reg(context, "eflags") & ~0x100))
        elif name == "push":
            segment_push = self.startup and raw in {b"\x06", b"\x1e"}
            if len(operands) != 1 or (operands[0].size != 4 and not segment_push):
                raise Hold("unproved push width")
            next_sp = sp - 4
            self.stack_access(next_sp, 4, write=True)
            writes.append((next_sp,2 if segment_push else 4))
            if segment_push:
                register_checks = (("stack_selector", next_sp, self.reg(context, instruction.reg_name(operands[0].reg))),)
        elif name == "pop":
            if len(operands) != 1 or operands[0].type != X86_OP_REG or (operands[0].size != 4 and not segment_pop):
                raise Hold("unsupported pop form")
            if instruction.reg_name(operands[0].reg) == "esp":
                raise Hold("unproved stack pivot")
            self.stack_access(sp, 4)
            self.read(sp, 4)
            if segment_pop:
                selector = struct.unpack("<H", self.read(sp, 2))[0]
                if selector != self.flat_segments["es"]:
                    raise Hold("POP ES would change the proved flat selector")
                register_checks = (("es", selector),)
            next_sp = sp + 4
        elif "esp" in written_names:
            if (name not in {"add", "sub"} or len(operands) != 2 or
                    operands[0].type != X86_OP_REG or instruction.reg_name(operands[0].reg) != "esp" or
                    operands[1].type != X86_OP_IMM or not 0 <= operands[1].imm <= 65536):
                raise Hold("unproved stack-pointer write")
            change = operands[1].imm * (1 if name == "add" else -1)
            next_sp = sp + change
            if not self.stack.start <= next_sp <= self.stack.start + self.stack.size:
                raise Hold("stack adjustment leaves owned stack")
        if next_pc not in self.code:
            return self.unclassified(context,pc,next_pc,next_sp,"unclassified branch/fallthrough target",
                                     transfer=transfer,writes=writes)
        if not 0 <= next_sp <= 0xffffffff:
            raise Hold("stack-pointer overflow")
        next_cs = self.frames[-1].selector if pop_frame and self.frames[-1].selector is not None else self.reg(context, "cs") & 0xffff
        decision = Decision("step", pc, next_pc, next_sp, owner.name, next_cs=next_cs,
                            transfer=transfer,frame=frame,pop_frame=pop_frame,writes=tuple(writes),
                            register_checks=register_checks)
        self.pending = decision
        return decision

    def plan_lodsb(self, context, owner, end, sp):
        if context["eflags"] & 0x400 or context["ds"] != self.flat_segments["ds"]:
            raise Hold("LODSB requires observed flat DS and forward direction")
        address = context["esi"]
        value = self.read(address, 1)[0]
        if address == 0xffffffff:
            raise Hold("LODSB pointer overflow")
        result = Decision("step", context["eip"], end, sp, owner.name,
                          next_cs=context["cs"], register_checks=(
                              ("esi", address + 1), ("eax", (context["eax"] & 0xffffff00) | value)))
        self.pending = result
        return result

    def plan_zero(self, context, owner, raw, end, sp):
        width = 4 if raw == b"\xf3\xab" else 1
        count, address = context["ecx"], context["edi"]
        if (context["eflags"] & 0x400 or context["es"] != self.flat_segments["es"] or
                context["eax"] != 0 or self.zero_span is None or count > 65536):
            raise Hold("REP zeroing lacks bounded forward zero/selector contract")
        if count and not self.zero_span.contains(address, count * width):
            raise Hold("REP zeroing exceeds exact declared BSS span")
        writes = ()
        if count:
            self.read(address, width, write=True)
            writes = ((address, width),)
        # x86 TF stops after each REP iteration, retaining EIP until ECX is zero.
        remaining = max(0, count - 1)
        result = Decision("step", context["eip"], context["eip"] if remaining else end,
                          sp, owner.name, next_cs=context["cs"], writes=writes,
                          register_checks=(("ecx", remaining), ("edi", address + (width if count else 0))))
        self.pending = result
        return result

    def observe(self, context, thread_id, event="step"):
        self.event(event, thread_id)
        decision = self.pending
        if decision is None or decision.action != "step":
            raise Hold("unexpected post-instruction event")
        if (self.reg(context, "eip") != decision.next_pc or
                self.reg(context, "esp") != decision.next_sp or
                self.reg(context, "cs") & 0xffff != decision.next_cs):
            raise Hold("observed instruction transfer differs from plan")
        for check in decision.register_checks:
            if check[0] == "stack_selector":
                if struct.unpack("<H", self.read(check[1], 2))[0] != check[2]:
                    raise Hold("actual pushed selector low word differs")
            elif check[0] == "stack_dword":
                if struct.unpack("<I", self.read(check[1], 4))[0] != check[2]:
                    raise Hold("actual saved ENTER frame pointer differs")
            elif check[0] == "flags_unchanged":
                if self.reg(context, "eflags") & ~0x100 != check[1]:
                    raise Hold("ENTER changed flags")
            elif self.reg(context, check[0]) != check[1]:
                raise Hold("observed startup register differs: " + check[0])
        for segment in ("ds", "es", "ss"):
            if self.startup and self.reg(context, segment) != self.flat_segments[segment]:
                raise Hold("startup instruction changed a proved segment")
        if decision.pop_frame:
            self.frames.pop()
        if decision.frame is not None:
            actual_return = struct.unpack("<I", self.read(decision.frame.stack_pointer, 4))[0]
            if actual_return != decision.frame.return_pc:
                raise Hold("actual pushed call return slot differs from plan")
            self.frames.append(decision.frame)
        self.steps += 1
        self.pending = None

    def arm_frontier(self, decision, context, thread_id, snapshot_byte, before_byte,
                     write_ok, bytes_written, flushed):
        """Validate a real transport-installed target trap, then permit ONE step.

        snapshot_byte comes from the separately hash-bound relocated-image
        verifier. before_byte/write results are actual transport observations.
        This method neither installs a trap nor executes/forges a CALL.
        """
        self.event("step",thread_id)
        if self.pending is not None or self.frontier != (decision,context):
            raise Hold("stale or unexpected frontier transition")
        if (decision.next_pc is None or decision.transfer not in {"call","branch","linear"} or
                not self.application_image.contains(decision.next_pc,1)):
            raise Hold("frontier has no approved one-instruction transition")
        if (len(snapshot_byte) != 1 or snapshot_byte == b"\xcc" or before_byte != snapshot_byte or
                write_ok is not True or bytes_written != 1 or flushed is not True):
            raise Hold("target trap lacks verified prebytes/write/flush proof")
        if any(address <= decision.next_pc < address+size for address,size in decision.writes):
            raise Hold("transition instruction would overwrite its target trap")
        # Bounded inspection of an unclassified instruction entry is distinct
        # from authorizing an application read of an unowned data object.
        if self.inspect_entry_byte(decision.next_pc) != b"\xcc":
            raise Hold("target trap was not actually installed")
        if self.clock() >= self.deadline:
            raise Hold("deadline expired while arming frontier")
        self.pending = decision
        return {"action":"single-step-transfer-only","target":decision.next_pc,
                "original_byte":before_byte.hex(),"must_enable_tf":True}

    def observe_frontier(self, context, thread_id, event="step"):
        """Record the actual callee/branch entry; never a pre-CALL fabrication."""
        self.event(event,thread_id)
        decision=self.pending
        if decision is None or decision.action != "frontier":
            raise Hold("unexpected frontier observation")
        if (self.reg(context,"eip") != decision.next_pc or self.reg(context,"esp") != decision.next_sp or
                self.reg(context,"cs") & 0xffff != decision.next_cs):
            raise Hold("actual frontier target/SP/CS differs from guarded transfer")
        if self.inspect_entry_byte(decision.next_pc) != b"\xcc":
            raise Hold("frontier target trap disappeared")
        if decision.transfer == "call":
            self.stack_access(decision.next_sp,4)
            actual=struct.unpack("<I",self.read(decision.next_sp,4))[0]
            if actual != decision.call_return:
                raise Hold("actual frontier CALL return slot differs")
        self.steps += 1
        self.pending=self.frontier=None
        self.stopped_at_frontier=True
        return {"kind":"actual-guarded-frontier-entry","context":dict(context),
                "transfer":decision.transfer,"return_pc":decision.call_return,
                "target_trap_still_installed":True,"application_acceptance":False}

    def inspect_entry_byte(self,address):
        if not self.application_image.contains(address,1) or self.clock() >= self.deadline:
            raise Hold("entry inspection outside image/deadline")
        try:
            value=bytes(self.memory(address,1))
        except Exception as error:
            raise Hold("transport entry-byte inspection failed") from error
        if len(value)!=1 or self.clock() >= self.deadline:
            raise Hold("short or late entry-byte inspection")
        return value
