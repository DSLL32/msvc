"""Strict client for the unproved Win32 debug broker, never a fixture runner.

No process launch, decoding, policy bypass, target register seeding or writes.
Only explicit RPCs are exposed; gate.py owns instruction decisions.
"""
import struct

from ipc import ProtocolError

CONTEXT_NAMES=("eip","eax","ecx","edx","ebx","esp","ebp","esi","edi",
               "eflags","cs","ss","ds","es","fs","gs")


def words(values):
    if any(not isinstance(value,int) or not 0<=value<=0xffffffff for value in values):
        raise ProtocolError("broker DWORD outside bounds")
    return struct.pack("<%dI"%len(values),*values)


class Broker:
    def __init__(self,client):
        self.client=client
        self.events=[]

    def fixed(self,operation,payload,size):
        response=self.client.request(operation,payload)
        if len(response)!=size:
            self.client.failed=True
            raise ProtocolError("broker response length mismatch")
        return response

    def event(self):
        packet=struct.unpack("<9I",self.fixed(10,b"",36))
        result=dict(zip(("code","pid","tid","base","entry","teb","exception",
                         "exception_address","first_chance"),packet))
        self.events.append(result)
        return result

    def context(self):
        return dict(zip(CONTEXT_NAMES,struct.unpack("<16I",self.fixed(11,b"",64))))

    def read(self,address,size):
        if not isinstance(size,int) or not 1<=size<=512 or address+size>0x100000000:
            raise ProtocolError("broker read outside bounds")
        return self.fixed(12,words((address,size)),size)

    def trap(self,address,expected):
        if not isinstance(expected,int) or not 0<=expected<=255:
            raise ProtocolError("broker expected opcode outside byte bounds")
        return struct.unpack("<I",self.fixed(13,words((address,expected)),4))[0]

    def consume(self,index):
        self.fixed(14,words((index,)),0)

    def arm(self,before,decision):
        packet=tuple(before[name] for name in CONTEXT_NAMES)+(decision.next_pc,
                   decision.next_sp,decision.next_cs)
        self.fixed(15,words(packet),0)
        armed=self.context()
        expected=dict(before);expected["eflags"]|=0x100
        if armed!=expected:
            self.client.failed=True
            raise ProtocolError("ARM did not preserve exact raw context plus TF")
        return armed

    def step(self):
        self.fixed(16,b"",0)
        event=self.event()
        if event["code"]!=1 or event["exception"]!=0x80000004 or not event["first_chance"]:
            raise ProtocolError("missing owned first-chance debug step")
        after=self.context()
        if event["exception_address"]!=after["eip"]:
            raise ProtocolError("debug exception/context PC mismatch")
        return event,after

    def loader(self):
        self.fixed(17,b"",0)

    def stop(self):
        self.fixed(18,b"",0)

    def wcc_prefix(self):
        """One fixed loader prefix, ending before its CF launch CALL.

        The broker permanently denies resume/trap/step operations afterward.
        Caller must pin the original loader and verify loaded code/data first.
        """
        index=struct.unpack("<I",self.fixed(21,b"",4))[0]
        event=self.event()
        if (event["code"]!=1 or event["exception"]!=0x80000003 or
                not event["first_chance"] or
                event["exception_address"]!=event["base"]+0x10763):
            raise ProtocolError("missing stopped WCC loader launch")
        actual=self.context()
        if actual["eip"]!=event["base"]+0x10764:
            raise ProtocolError("WCC launch breakpoint/context mismatch")
        return index,event,actual

    def wcc_prepare_entry(self):
        return struct.unpack("<I",self.fixed(22,b"",4))[0]

    def wcc_entry_trap(self,before):
        self.fixed(23,b"",0)
        event=self.event();actual=self.context()
        expected=dict(before);expected["eip"]+=1
        if (event["code"]!=1 or event["exception"]!=0x80000003 or
                not event["first_chance"] or event["exception_address"]!=before["eip"] or
                actual!=expected):
            raise ProtocolError("actual CF entry INT3/context mismatch")
        return event,actual

    def arena(self,base,size):
        self.fixed(24,words((base,size)),0)

    def frontier_trap(self,index,before):
        self.fixed(25,words((index,)),0)
        event=self.event();actual=self.context();expected=dict(before);expected['eip']+=1
        if (event['code']!=1 or event['exception']!=0x80000003 or not event['first_chance'] or
                event['exception_address']!=before['eip'] or actual!=expected):
            raise ProtocolError('actual arena frontier INT3/context mismatch')
        return event,actual

    def authored_setup(self,ready_rva,expected,signature_rva):
        index=struct.unpack('<I',self.fixed(26,words((ready_rva,expected,signature_rva)),4))[0]
        event=self.event();actual=self.context()
        if (event['code']!=1 or event['exception']!=0x80000003 or not event['first_chance'] or
                event['exception_address']!=event['base']+ready_rva or
                actual['eip']!=event['base']+ready_rva+1):
            raise ProtocolError('authored setup did not stop at owned ready trap')
        return index,event,actual

    def flat_selectors(self,actual):
        packet=struct.unpack("<12I",self.fixed(19,b"",48));result={};proof=[]
        for index,name in enumerate(("cs","ss","ds","es")):
            selector,low,high=packet[index*3:index*3+3]
            base=(low>>16)|((high&255)<<16)|(high&0xff000000)
            limit=(low&65535)|(((high>>16)&15)<<16)
            if high&(1<<23):limit=(limit<<12)|4095
            if (selector!=actual[name] or selector>65535 or base or limit!=0xffffffff or
                    not high&(1<<15) or not high&(1<<22)):
                raise ProtocolError("unsupported/nonflat actual segment descriptor")
            result[name]=selector
            proof.append({"name":name,"selector":selector,"raw_low":low,"raw_high":high,
                          "base":base,"limit":limit})
        return result,proof

    def region(self,address):
        packet=struct.unpack("<7I",self.fixed(20,words((address,)),28))
        result=dict(zip(("base","allocation_base","allocation_protect","size","state",
                         "protect","type"),packet))
        if (not result["size"] or result["base"]+result["size"]>0x100000000 or
                not result["base"]<=address<result["base"]+result["size"]):
            raise ProtocolError("invalid observed memory region")
        return result

    def entry(self,rva,expected):
        first=self.event()
        if first["code"]!=3 or first["entry"]!=first["base"]+rva:
            raise ProtocolError("unexpected initial process/entry event")
        index=self.trap(first["entry"],expected)
        for _ in range(100):
            self.loader();event=self.event()
            if event["pid"]!=first["pid"] or event["tid"]!=first["tid"]:
                raise ProtocolError("foreign startup process/thread")
            if event["code"]==1 and event["exception_address"]==first["entry"]:
                if event["exception"]!=0x80000003 or not event["first_chance"]:
                    raise ProtocolError("unexpected entry exception")
                before=self.context()
                if before["eip"]!=first["entry"]+1:
                    raise ProtocolError("entry trap context mismatch")
                self.consume(index);after=self.context()
                expected_context=dict(before);expected_context["eip"]=first["entry"]
                if after!=expected_context:
                    raise ProtocolError("owned trap rewind changed unrelated raw context")
                return first,after
        raise ProtocolError("startup event budget exhausted")
