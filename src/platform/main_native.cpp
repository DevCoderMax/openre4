#include <aurora/aurora.h>
#include <aurora/event.h>
#include <aurora/main.h>
#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <dolphin/pad.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

static void openre4_log_callback(AuroraLogLevel level, const char* module, const char* message, unsigned int len) {
    const char* levelStr = "INFO";
    FILE* out = stdout;
    switch (level) {
    case LOG_DEBUG:   levelStr = "DEBUG"; break;
    case LOG_INFO:    levelStr = "INFO";  break;
    case LOG_WARNING: levelStr = "WARN";  break;
    case LOG_ERROR:   levelStr = "ERROR"; out = stderr; break;
    case LOG_FATAL:   levelStr = "FATAL"; out = stderr; break;
    }
    fprintf(out, "[OpenRE4 %s][%s] %s\n", levelStr, module, message);
    if (level == LOG_FATAL) {
        fflush(out);
        abort();
    }
}

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("   OpenRE4 — Native Engine Starting...  \n");
    printf("   Platform: %s (Native x86_64)         \n",
#if defined(__linux__)
           "Linux"
#elif defined(_WIN32)
           "Windows"
#elif defined(__APPLE__)
           "macOS"
#else
           "Unknown"
#endif
    );
    printf("========================================\n");

    const AuroraConfig config = {
        .appName = "OpenRE4 (Native Port)",
        .logCallback = &openre4_log_callback,
    };
    AuroraInfo initInfo = aurora_initialize(argc, argv, &config);

    bool exiting = false;
    bool paused = false;

    // Loop principal da plataforma
    while (!exiting) {
        const AuroraEvent* event = aurora_update();
        while (event != NULL && event->type != AURORA_NONE) {
            switch (event->type) {
            case AURORA_EXIT:
                exiting = true;
                break;
            case AURORA_PAUSED:
                paused = true;
                break;
            case AURORA_UNPAUSED:
                paused = false;
                break;
            case AURORA_WINDOW_RESIZED:
                initInfo.windowSize = event->windowSize;
                break;
            default:
                break;
            }
            ++event;
        }

        if (exiting || paused || !aurora_begin_frame()) {
            continue;
        }

        static float timer = 0.0f;
        static uint32_t frameCount = 0;
        timer += 0.03f;
        frameCount++;

        if (frameCount % 60 == 0) {
            printf("[OpenRE4 Core] Rodando a 60 FPS nativo! Frame: %u\n", frameCount);
            fflush(stdout);
        }

        // Vermelho "Biohazard / Resident Evil" pulsando suavemente para dar feedback visual claro
        uint8_t redIntensity = (uint8_t)((sinf(timer) * 0.5f + 0.5f) * 150.0f + 50.0f);
        GXColor re4PulsingRed = { redIntensity, 15, 20, 255 };
        GXSetCopyClear(re4PulsingRed, GX_MAX_Z24);

        aurora_end_frame();
    }

    printf("OpenRE4 encerrando com sucesso.\n");
    aurora_shutdown();
    return 0;
}
