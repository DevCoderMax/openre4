#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <dolphin/mtx.h>
#include <charPipeline/texPalette.h>
#include "gx.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ucontext.h>

extern "C" {

void OSReport(const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
    fflush(stdout);
}

void OSPanic(const char* file, int line, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "[OpenRE4 OS PANIC] %s:%d: ", file, line);
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

#undef PSMTXIdentity
#undef PSMTXCopy
#undef PSMTXConcat
#undef PSMTXTranspose
#undef PSMTXInverse
#undef PSMTXMultVec
#undef PSMTXMultVecArray
#undef PSMTXMultVecSR
#undef PSMTXTrans
#undef PSMTXScale
#undef PSMTXRotRad
#undef PSMTXRotTrig
#undef PSMTXRotAxisRad
#undef PSMTXTransApply
#undef PSMTXScaleApply
#undef PSMTXReorder
#undef PSMTXQuat
#undef PSMTX44MultVec
#undef PSVECAdd
#undef PSVECSubtract
#undef PSVECScale
#undef PSVECDotProduct
#undef PSVECCrossProduct

void PSVECAdd(const Vec* a, const Vec* b, Vec* ab) { C_VECAdd(a, b, ab); }
void PSVECSubtract(const Vec* a, const Vec* b, Vec* a_b) { C_VECSubtract(a, b, a_b); }
void PSVECScale(const Vec* src, Vec* dst, f32 scale) { C_VECScale(src, dst, scale); }
f32 PSVECDotProduct(const Vec* a, const Vec* b) { return C_VECDotProduct(a, b); }
void PSVECCrossProduct(const Vec* a, const Vec* b, Vec* axb) { C_VECCrossProduct(a, b, axb); }

void PSMTXIdentity(Mtx m) { C_MTXIdentity(m); }
void PSMTXCopy(const Mtx src, Mtx dst) { C_MTXCopy(src, dst); }
void PSMTXConcat(const Mtx a, const Mtx b, Mtx ab) { C_MTXConcat(a, b, ab); }
void PSMTXTranspose(const Mtx src, Mtx xPose) { C_MTXTranspose(src, xPose); }
u32 PSMTXInverse(const Mtx src, Mtx inv) { return C_MTXInverse(src, inv); }
void PSMTXMultVec(const Mtx m, const Vec* src, Vec* dst) { C_MTXMultVec(m, src, dst); }
void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count) { C_MTXMultVecArray(m, srcBase, dstBase, count); }
void PSMTXMultVecSR(const Mtx m, const Vec* src, Vec* dst) { C_MTXMultVecSR(m, src, dst); }
void PSMTXTrans(Mtx m, f32 xT, f32 yT, f32 zT) { C_MTXTrans(m, xT, yT, zT); }
void PSMTXScale(Mtx m, f32 xS, f32 yS, f32 zS) { C_MTXScale(m, xS, yS, zS); }
void PSMTXRotRad(Mtx m, char axis, f32 rad) { C_MTXRotRad(m, axis, rad); }
void PSMTXRotTrig(Mtx m, char axis, f32 sinA, f32 cosA) { C_MTXRotTrig(m, axis, sinA, cosA); }
void PSMTXRotAxisRad(Mtx m, const Vec* axis, f32 rad) { C_MTXRotAxisRad(m, axis, rad); }
void PSMTXTransApply(const Mtx src, Mtx dst, f32 xT, f32 yT, f32 zT) { C_MTXTransApply(src, dst, xT, yT, zT); }
void PSMTXScaleApply(const Mtx src, Mtx dst, f32 xS, f32 yS, f32 zS) { C_MTXScaleApply(src, dst, xS, yS, zS); }
void PSMTXReorder(Mtx src, f32 dst[4][3]) { C_MTXReorder(src, dst); }


BOOL DBIsDebuggerPresent(void) {
    return FALSE;
}

BOOL OSGetResetButtonState(void) {
    return FALSE;
}

void VIWaitForRetrace(void) {
    // Retrace e vsync são sincronizados via Mailbox/FIFO no WebGPU swapchain
}

void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu) {
    printf("[OpenRE4 OS] OSResetSystem chamado (reset=%d, code=%u). Encerrando...\n", reset, resetCode);
    exit(0);
}

void PPCSync(void) {
    // Sincronização de barramento PowerPC (no-op em x86_64/ARM64)
}

void ADXGC_SetupDvdFs(int mode) {
    // Configuração de sistema de arquivos CRI ADX
}

u32 OSGetProgressiveMode(void) {
    return 1; // 480p progressivo habilitado por padrão
}

void OSSetProgressiveMode(u32 on) {
    (void)on;
}

u32 VIGetDTVStatus(void) {
    return 1; // Component video / DTV detectado
}

void VISetBlack(BOOL black) {
    (void)black;
}

void AXFXSetHooks(void* alloc, void* free) {
    (void)alloc; (void)free;
}

void PSMTX44MultVec(const Mtx44 m, const Vec* src, Vec* dst) {
    Vec vTmp;
    f32 w;
    vTmp.x = m[0][0] * src->x + m[0][1] * src->y + m[0][2] * src->z + m[0][3];
    vTmp.y = m[1][0] * src->x + m[1][1] * src->y + m[1][2] * src->z + m[1][3];
    vTmp.z = m[2][0] * src->x + m[2][1] * src->y + m[2][2] * src->z + m[2][3];
    w = m[3][0] * src->x + m[3][1] * src->y + m[3][2] * src->z + m[3][3];
    if (w != 0.0f) {
        w = 1.0f / w;
        dst->x = vTmp.x * w;
        dst->y = vTmp.y * w;
        dst->z = vTmp.z * w;
    } else {
        *dst = vTmp;
    }
}

void PSMTXQuat(Mtx m, const Quaternion* q) {
    C_MTXQuat(m, q);
}

TEXDescriptorPtr TEXGet(TEXPalettePtr pal, u32 id) {
    if (!pal || !pal->descriptorArray) return NULL;
    return &pal->descriptorArray[id];
}

typedef void (*GXDrawSyncCallback)(u16 token);
static GXDrawSyncCallback s_drawSyncCallback = NULL;

GXDrawSyncCallback GXSetDrawSyncCallback(GXDrawSyncCallback cb) {
    GXDrawSyncCallback old = s_drawSyncCallback;
    s_drawSyncCallback = cb;
    return old;
}

void GXSetDrawSync(u16 token) {
    if (s_drawSyncCallback) {
        s_drawSyncCallback(token);
    }
}

// Write Gather Pipe FIFO buffer
extern "C++" WGPipe GXWGFifo[1];

// Memory Allocation / Deallocation
void* __builtin_new(size_t size) { return malloc(size); }
void __builtin_delete(void* ptr) { free(ptr); }
void* __builtin_vec_new(size_t size) { return malloc(size); }
void __builtin_vec_delete(void* ptr) { free(ptr); }

// PPC Hardware Performance Registers (no-op on PC)
u32 PPCMfpmc1(void) { return 0; }
void PPCMtmsr(u32 v) { (void)v; }
void PPCMtpmc1(u32 v) { (void)v; }
void PPCMtpmc2(u32 v) { (void)v; }
void PPCMtpmc3(u32 v) { (void)v; }
void PPCMtpmc4(u32 v) { (void)v; }
void PPCMtmmcr0(u32 v) { (void)v; }
void PPCMtmmcr1(u32 v) { (void)v; }

// GX / VI / ARAM
void GXSetCurrentGXThread(void) {}
void GXClearBoundingBox(void) {}
void GXPeekZ(u16 x, u16 y, u32* z) { (void)x; (void)y; if (z) *z = 0; }
void __GXSetIndirectMask(u32 mask) { (void)mask; }
void VISetNextFrameBuffer(void* fb) { (void)fb; }
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb) { (void)cb; return NULL; }
u32 VIGetNextField(void) { return 0; }

// ProDG Host PC Debugger File I/O
int PCinit(void) { return 0; }
int PCopen(const char* name, int flags, int mode) { (void)name; (void)flags; (void)mode; return -1; }
int PCread(int fd, void* buf, int len) { (void)fd; (void)buf; (void)len; return 0; }
int PCwrite(int fd, const void* buf, int len) { (void)fd; (void)buf; return len; }
int PCclose(int fd) { (void)fd; return 0; }
int PClseek(int fd, int offset, int origin) { (void)fd; (void)offset; (void)origin; return 0; }
int PCcreat(const char* name, int mode) { (void)name; (void)mode; return -1; }

// OS Threading / Synchronization
BOOL OSEnableInterrupts(void) { return TRUE; }
BOOL OSDisableInterrupts(void) { return FALSE; }
BOOL OSRestoreInterrupts(BOOL level) { return level; }
s32 OSEnableScheduler(void) { return 0; }
struct NativeFiberInfo {
    ucontext_t ctx;
    void* (*func)(void*);
    void* param;
    void* allocatedStack;
    bool active;
    bool exited;
};

static ucontext_t s_mainContext;
static OSThread s_mainThread;
static OSThread* s_currentThread = &s_mainThread;

static void FiberTrampoline(uint32_t lo, uint32_t hi) {
    uintptr_t ptr = ((uintptr_t)hi << 32) | lo;
    NativeFiberInfo* info = (NativeFiberInfo*) ptr;
    if (info && info->func) {
        info->func(info->param);
    }
    if (info) {
        info->exited = true;
        info->active = false;
    }
    s_currentThread = &s_mainThread;
    setcontext(&s_mainContext);
}

void OSCancelThread(OSThread* thread) {
    if (thread && thread != &s_mainThread) {
        NativeFiberInfo* info = (NativeFiberInfo*) thread->specific[0];
        if (info) {
            info->active = false;
            info->exited = true;
        }
    }
}

BOOL OSCreateThread(OSThread* thread, void* (*func)(void*), void* param, void* stack, u32 stackSize, OSPriority priority, u16 attr) {
    (void)priority; (void)attr;
    if (stack && stackSize) {
        *(u32*)((u8*)stack - stackSize) = OS_THREAD_STACK_MAGIC;
    }

    NativeFiberInfo* info = (NativeFiberInfo*) calloc(1, sizeof(NativeFiberInfo));
    info->func = func;
    info->param = param;
    info->active = true;
    info->exited = false;
    info->allocatedStack = malloc(512 * 1024);

    getcontext(&info->ctx);
    info->ctx.uc_stack.ss_sp = info->allocatedStack;
    info->ctx.uc_stack.ss_size = 512 * 1024;
    info->ctx.uc_link = &s_mainContext;

    uintptr_t ptr = (uintptr_t) info;
    makecontext(&info->ctx, (void(*)())FiberTrampoline, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));

    thread->specific[0] = info;
    thread->state = OS_THREAD_STATE_READY;
    return TRUE;
}

void OSExitThread(void* val) {
    (void)val;
    if (s_currentThread && s_currentThread != &s_mainThread) {
        NativeFiberInfo* info = (NativeFiberInfo*) s_currentThread->specific[0];
        if (info) {
            info->exited = true;
            info->active = false;
        }
        s_currentThread = &s_mainThread;
        swapcontext(&info->ctx, &s_mainContext);
    }
}

OSThread* OSGetCurrentThread(void) {
    return s_currentThread;
}

s32 OSResumeThread(OSThread* thread) {
    if (!thread) return 0;
    if (thread == &s_mainThread) {
        if (s_currentThread != &s_mainThread) {
            NativeFiberInfo* cur_info = (NativeFiberInfo*) s_currentThread->specific[0];
            s_currentThread = &s_mainThread;
            swapcontext(&cur_info->ctx, &s_mainContext);
        }
        return 0;
    }

    NativeFiberInfo* info = (NativeFiberInfo*) thread->specific[0];
    if (info && info->active && !info->exited) {
        s_currentThread = thread;
        swapcontext(&s_mainContext, &info->ctx);
    }
    return 0;
}

void OSSleepThread(OSThreadQueue* queue) {
    (void)queue;
    if (s_currentThread && s_currentThread != &s_mainThread) {
        NativeFiberInfo* info = (NativeFiberInfo*) s_currentThread->specific[0];
        s_currentThread = &s_mainThread;
        swapcontext(&info->ctx, &s_mainContext);
    }
}

s32 OSSuspendThread(OSThread* thread) {
    if (thread == &s_mainThread && s_currentThread == &s_mainThread) {
        return 0;
    }
    if (thread == s_currentThread && s_currentThread != &s_mainThread) {
        NativeFiberInfo* info = (NativeFiberInfo*) s_currentThread->specific[0];
        s_currentThread = &s_mainThread;
        swapcontext(&info->ctx, &s_mainContext);
    }
    return 0;
}

void OSWakeupThread(OSThreadQueue* queue) { (void)queue; }
void OSInitThreadQueue(OSThreadQueue* queue) { (void)queue; }
void OSInitSemaphore(OSSemaphore* sem, s32 count) { (void)sem; (void)count; }
s32 OSWaitSemaphore(OSSemaphore* sem) { (void)sem; return 0; }
s32 OSSignalSemaphore(OSSemaphore* sem) { (void)sem; return 0; }
void OSInitAlarm(void) {}
BOOL OSInitFont(OSFontHeader* font) { (void)font; return TRUE; }
u16 OSGetFontEncode(void) { return 0; }
char* OSGetFontTexture(const char* string, void** image, s32* x, s32* y, s32* width) {
    (void)string; (void)image; (void)x; (void)y; (void)width;
    return NULL;
}
u32 OSGetConsoleSimulatedMemSize(void) { return 64 * 1024 * 1024; }
u32 OSGetConsoleType(void) { return 0x10000002; }
OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler) { (void)error; (void)handler; return NULL; }
void OSSetSaveRegion(void* start, void* end) { (void)start; (void)end; }
u32 OSGetSoundMode(void) { return 1; }
void OSSetSoundMode(u32 mode) { (void)mode; }
BOOL OSLink(OSModuleInfo* newModule, void* bss) { (void)newModule; (void)bss; return TRUE; }
BOOL OSUnlink(OSModuleInfo* oldModule) { (void)oldModule; return TRUE; }

// AX / MIX / Audio Driver
void AXInitEx(u32 mode) { (void)mode; }
void AXQuit(void) {}
void* AXAcquireVoice(u32 prio, void* cb, u32 user) { (void)prio; (void)cb; (void)user; return NULL; }
void AXFreeVoice(void* voice) { (void)voice; }
void AXSetVoiceState(void* voice, u16 state) { (void)voice; (void)state; }
void AXSetVoiceType(void* voice, u16 type) { (void)voice; (void)type; }
void AXSetVoicePriority(void* voice, u32 prio) { (void)voice; (void)prio; }
void AXSetVoiceAddr(void* voice, void* addr) { (void)voice; (void)addr; }
void AXSetVoiceLoop(void* voice, u16 loop) { (void)voice; (void)loop; }
void AXSetVoiceLoopAddr(void* voice, u32 addr) { (void)voice; (void)addr; }
void AXSetVoiceEndAddr(void* voice, u32 addr) { (void)voice; (void)addr; }
void AXSetVoiceAdpcm(void* voice, void* adpcm) { (void)voice; (void)adpcm; }
void AXSetVoiceAdpcmLoop(void* voice, void* loop) { (void)voice; (void)loop; }
void AXSetVoiceSrc(void* voice, void* src) { (void)voice; (void)src; }
void AXSetVoiceSrcRatio(void* voice, f32 ratio) { (void)voice; (void)ratio; }
void AXSetVoiceSrcType(void* voice, u32 type) { (void)voice; (void)type; }
void AXSetVoiceLpf(void* voice, void* lpf) { (void)voice; (void)lpf; }
void AXSetVoiceLpfCoefs(void* voice, u16 a1, u16 a2, u16 b1, u16 b2) { (void)voice; (void)a1; (void)a2; (void)b1; (void)b2; }
void AXSetMode(u32 mode) { (void)mode; }
void AXRegisterCallback(void* cb) { (void)cb; }
void AXRegisterAuxACallback(void* cb, void* user) { (void)cb; (void)user; }
void AXRegisterAuxBCallback(void* cb, void* user) { (void)cb; (void)user; }
u32 AXGetDspCycles(void) { return 0; }
u32 AXGetMaxDspCycles(void) { return 0; }
void AXSetCompressor(u32 switch_) { (void)switch_; }

void AXARTInit(void) {}
void AXARTQuit(void) {}
void AXARTServiceSounds(void) {}

void AXFXChorusInit(void* p) { (void)p; }
void AXFXChorusShutdown(void* p) { (void)p; }
void AXFXChorusSettings(void* p) { (void)p; }
void AXFXChorusCallback(void* ch, void* p) { (void)ch; (void)p; }

void AXFXDelayInit(void* p) { (void)p; }
void AXFXDelayShutdown(void* p) { (void)p; }
void AXFXDelaySettings(void* p) { (void)p; }
void AXFXDelayCallback(void* ch, void* p) { (void)ch; (void)p; }

void AXFXReverbStdInit(void* p) { (void)p; }
void AXFXReverbStdShutdown(void* p) { (void)p; }
void AXFXReverbStdSettings(void* p) { (void)p; }
void AXFXReverbStdCallback(void* ch, void* p) { (void)ch; (void)p; }

void AXFXReverbHiInit(void* p) { (void)p; }
void AXFXReverbHiInitDpl2(void* p) { (void)p; }
void AXFXReverbHiShutdown(void* p) { (void)p; }
void AXFXReverbHiShutdownDpl2(void* p) { (void)p; }
void AXFXReverbHiSettings(void* p) { (void)p; }
void AXFXReverbHiSettingsDpl2(void* p) { (void)p; }
void AXFXReverbHiCallback(void* ch, void* p) { (void)ch; (void)p; }
void AXFXReverbHiCallbackDpl2(void* ch, void* p) { (void)ch; (void)p; }

void MIXInit(void) {}
void MIXQuit(void) {}
void MIXInitChannel(void* chan, u16 in, u16 auxA, u16 auxB, u16 pan, u16 sPan, f32 fPan) {
    (void)chan; (void)in; (void)auxA; (void)auxB; (void)pan; (void)sPan; (void)fPan;
}
void MIXReleaseChannel(void* chan) { (void)chan; }
void MIXSetInput(void* chan, f32 in) { (void)chan; (void)in; }
void MIXSetPan(void* chan, u16 pan) { (void)chan; (void)pan; }
void MIXSetSPan(void* chan, u16 sPan) { (void)chan; (void)sPan; }
void MIXSetAuxA(void* chan, f32 auxA) { (void)chan; (void)auxA; }
void MIXSetAuxB(void* chan, f32 auxB) { (void)chan; (void)auxB; }
void MIXSetSoundMode(u32 mode) { (void)mode; }
void MIXUpdateSettings(void) {}

void SEQInit(void) {}
void SEQQuit(void) {}
void SEQRunAudioFrame(void) {}

void SYNInit(void) {}
void SYNInitSynth(void) {}
void SYNQuit(void) {}
void SYNQuitSynth(void) {}
void SYNRunAudioFrame(void) {}
void SYNMidiInput(void* midi) { (void)midi; }
void SYNSetMasterVolume(s32 vol) { (void)vol; }
u32 SYNGetActiveNotes(void) { return 0; }

void AIInit(u8* stack) { (void)stack; }
void AIReset(void) {}

void ADXM_ExecMain(void) {}
void ADXM_SetCbErr(void* cb) { (void)cb; }
void ADXT_SetOutputMono(int sw) { (void)sw; }

void* mwPlyCreateSofdec(void* p) { (void)p; return NULL; }
void mwPlyInitSfdFx(void* p) { (void)p; }
u32 mwPlyCalcWorkCprmSfd(void* p) { (void)p; return 0; }
void mwPlyFxSetOutBufSize(void* p, u32 w, u32 h) { (void)p; (void)w; (void)h; }
void mwPlyFxSetOutBufPitchHeight(void* p, u32 pitch, u32 h) { (void)p; (void)pitch; (void)h; }
void mwPlyFxCnvFrmARGB8888(void* p) { (void)p; }
void mwPlyFxCnvFrmY84C44(void* p) { (void)p; }
void* mwPlyGetCurFrm(void* p) { (void)p; return NULL; }
void mwPlyRelCurFrm(void* p) { (void)p; }
void mwPlyGetHdrInf(void* p, void* h) { (void)p; (void)h; }
u32 mwPlyGetNumSkipDec(void* p) { (void)p; return 0; }
u32 mwPlyGetNumSkipDisp(void* p) { (void)p; return 0; }

void (*_ctors[])(void) = { nullptr };
void (*_dtors[])(void) = { nullptr };

}


