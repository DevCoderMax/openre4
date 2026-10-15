#include <aurora/aurora.h>
#include <aurora/event.h>
#include <aurora/main.h>
#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <dolphin/pad.h>
#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <SDL3/SDL_scancode.h>

#include "main.h"
#include "scheduler.h"
#include "title.h"
#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#endif

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
        .appName = "Resident Evil 4 (Native Port)",
        .logCallback = &openre4_log_callback,
        .mem1Size = 64 * 1024 * 1024,
        .mem2Size = ARAM_DEFAULT_SIZE,
    };
    AuroraInfo initInfo = aurora_initialize(argc, argv, &config);
    aurora_set_pause_on_focus_lost(false);
    aurora_set_background_input(true);

#if defined(__linux__) || defined(__APPLE__)
    void* lockedCache = mmap((void*) 0xE0000000, 64 * 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (lockedCache != MAP_FAILED) {
        printf("[OpenRE4 Core] Mapped Locked Cache at 0xE0000000 (64 KB)\n");
    } else {
        printf("[OpenRE4 Core] Warning: Failed to map Locked Cache at 0xE0000000\n");
    }
#endif

    printf("[OpenRE4 FS] Montando imagem de disco original...\n");
    if (aurora_dvd_open("orig/G4BE08/re4_disc1.gcm")) {
        DVDInit();
        DVDDiskID* diskID = DVDGetCurrentDiskID();
        if (diskID) {
            printf("[OpenRE4 FS] Disco montado com sucesso! GameID: %.4s, Company: %.2s, DiscNo: %u\n",
                   diskID->gameName, diskID->company, diskID->diskNumber);
        }
    } else {
        printf("[OpenRE4 FS] Falha ao abrir orig/G4BE08/re4_disc1.gcm\n");
    }

    printf("[OpenRE4 Core] Inicializando subsistemas do Resident Evil 4...\n");
    systemStartInit();
    systemRestartInit();
    if (pG != NULL) {
        pG->CardStatus |= 0x80000000;
    }

    printf("[OpenRE4 Core] Configurando mapeamento de teclado e controles nativos...\n");
    PADKeyButtonBinding buttons[PAD_BUTTON_COUNT] = {
        {SDL_SCANCODE_SPACE,  PAD_BUTTON_A},
        {SDL_SCANCODE_X,      PAD_BUTTON_B},
        {SDL_SCANCODE_C,      PAD_BUTTON_X},
        {SDL_SCANCODE_V,      PAD_BUTTON_Y},
        {SDL_SCANCODE_RETURN, PAD_BUTTON_START},
        {SDL_SCANCODE_TAB,    PAD_TRIGGER_Z},
        {SDL_SCANCODE_Q,      PAD_TRIGGER_L},
        {SDL_SCANCODE_E,      PAD_TRIGGER_R},
        {SDL_SCANCODE_UP,     PAD_BUTTON_UP},
        {SDL_SCANCODE_DOWN,   PAD_BUTTON_DOWN},
        {SDL_SCANCODE_LEFT,   PAD_BUTTON_LEFT},
        {SDL_SCANCODE_RIGHT,  PAD_BUTTON_RIGHT},
    };
    PADKeyAxisBinding axes[PAD_AXIS_COUNT] = {
        {SDL_SCANCODE_D, PAD_AXIS_LEFT_X_POS, 1},
        {SDL_SCANCODE_A, PAD_AXIS_LEFT_X_NEG, 1},
        {SDL_SCANCODE_W, PAD_AXIS_LEFT_Y_POS, 1},
        {SDL_SCANCODE_S, PAD_AXIS_LEFT_Y_NEG, 1},
        {SDL_SCANCODE_L, PAD_AXIS_RIGHT_X_POS, 1},
        {SDL_SCANCODE_J, PAD_AXIS_RIGHT_X_NEG, 1},
        {SDL_SCANCODE_I, PAD_AXIS_RIGHT_Y_POS, 1},
        {SDL_SCANCODE_K, PAD_AXIS_RIGHT_Y_NEG, 1},
        {SDL_SCANCODE_Q, PAD_AXIS_TRIGGER_L, 0},
        {SDL_SCANCODE_E, PAD_AXIS_TRIGGER_R, 0},
    };
    PADSetKeyButtonBindings(0, buttons);
    PADSetKeyAxisBindings(0, axes);
    PADSetKeyboardActive(0, TRUE);

    bool showLogos = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--show-logos") == 0) {
            showLogos = true;
        } else if (strcmp(argv[i], "--village") == 0 || strcmp(argv[i], "--pueblo") == 0) {
            g_autoStartVillage = true;
            printf("[OpenRE4 Core] Modo Auto-Village ativo: pulando direto para Pueblo (r100)!\n");
        }
    }
    if (!showLogos && pRK != NULL) {
        pRK->logo_skip_enable = 1;
        printf("[OpenRE4 Core] Logo skip ativo: abrindo tela de título imediatamente!\n");
    }

    printf("===============================================================\n");
    printf("   OpenRE4 - Mapeamento de Teclado PC Nativo Ativo:            \n");
    printf("   - ENTER / ESPAÇO : START / Confirma / Ação (Botão A)        \n");
    printf("   - X / ESC        : Cancelar / Correr (Botão B)              \n");
    printf("   - C / V          : Botão X / Y (Inventário / Status)        \n");
    printf("   - W, A, S, D     : Movimentação (Analógico Principal)       \n");
    printf("   - SETAS          : D-Pad (Navegação de Menus)               \n");
    printf("   - Q / E          : Mirar / Atirar / Facada (Gatilhos L / R) \n");
    printf("   - TAB            : Mapa (Gatilho Z)                         \n");
    printf("   - I, J, K, L     : Câmera (Analógico Direito C-Stick)       \n");
    printf("===============================================================\n");

    printf("[OpenRE4 Core] Enfileirando Title_task (Tela de Título / Abertura)...\n");
    TaskExec(0, Title_task, 0);

    bool exiting = false;
    bool paused = false;
    uint32_t frameCount = 0;

    printf("[OpenRE4 Core] Iniciando loop de quadros nativo...\n");
    while (!exiting) {
        const AuroraEvent* event = aurora_update();
        while (event != NULL && event->type != AURORA_NONE) {
            switch (event->type) {
            case AURORA_EXIT:
                printf("[OpenRE4 Debug] Event AURORA_EXIT recebido!\n");
                exiting = true;
                break;
            case AURORA_PAUSED:
                printf("[OpenRE4 Debug] Event AURORA_PAUSED recebido!\n");
                paused = true;
                break;
            case AURORA_UNPAUSED:
                printf("[OpenRE4 Debug] Event AURORA_UNPAUSED recebido!\n");
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

        if (exiting) break;
        if (paused) {
            continue;
        }
        if (!aurora_begin_frame()) {
            static int begin_fail_count = 0;
            if (begin_fail_count++ < 5) {
                printf("[OpenRE4 Debug] aurora_begin_frame() falhou! (count=%d)\n", begin_fail_count);
            }
            continue;
        }

        bio4_frame();
        frameCount++;

        if (frameCount <= 10 || frameCount % 60 == 0) {
            printf("[OpenRE4 Core] Rodando a 60 FPS nativo! Frame: %u\n", frameCount);
            fflush(stdout);
        }

        aurora_end_frame();
    }

    printf("OpenRE4 encerrando com sucesso.\n");
    aurora_shutdown();
    return 0;
}
