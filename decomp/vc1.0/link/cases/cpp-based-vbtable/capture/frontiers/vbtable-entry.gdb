set pagination off
set confirm off
run -dump /symbols /undecorate names.obj > /root/.ai-tmp/vc1.0-link/cpp-based-vbtable/vbtable-entry-native.stdout 2> /root/.ai-tmp/vc1.0-link/cpp-based-vbtable/vbtable-entry-native.stderr
bt 8
info args
python
try:
 section=gdb.parse_and_eval("section")
 if "Section *" not in str(section.type): raise gdb.error("not section pointer")
 s=section.cast(gdb.lookup_type("unsigned int").pointer())
 for i in range(18): gdb.write("SECTION %02d %08x\n" % (i,int(s[i])))
 gdb.write("SECTION_NAME %s\n" % gdb.parse_and_eval("section->name").string())
except gdb.error: pass
end
quit
