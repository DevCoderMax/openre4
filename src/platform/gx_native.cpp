#include <dolphin/gx.h>

extern "C" void GXSetArraySized(int attr, const void* base_ptr, u32 size, u8 stride) {
    GXSetArray(static_cast<GXAttr>(attr), base_ptr, size, stride, false);
}
