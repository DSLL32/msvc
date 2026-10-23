#include "widen.h"
char object_size_must_be_4[sizeof(WidenTi) == 4 ? 1 : -1];
char description_size_must_be_16[sizeof(WidenOutput) == 16 ? 1 : -1];
extern "C" int probe_factory(WidenTi **out) { return WidenTi::fCreate(*out, 256, 0); }
extern "C" int probe_query(WidenTi *object, WidenOutput *out, const void *input, unsigned size) { return object->Query(out, input, size, 0); }
extern "C" int probe_emit(WidenTi *object, WidenOutput *out, const void *input, unsigned size) { return object->Emit(out, input, size, 0); }
extern "C" void probe_destroy(WidenTi *object) { object->Release(); }
