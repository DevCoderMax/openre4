#include "types.h"
#include <stdio.h>
#include "em.h"
#include "em_mod.h"
#include "em10.h"

extern void Em12Init(cEm* em);
extern void Em12Set(cEm10* em);
extern void Em15Init(cEm* em);
extern void Em15Set(cEm10* em);
extern void Em26Init(cEm* em);

static void EmSafeFallbackInit(cEm* em) {
    new (em) cEm();
    em->r_no_0 = 0xFF;
}

void NativeEmProlog(u32 em_id) {
    printf("[OpenRE4 Core] NativeEmProlog: vinculando modulo nativo do inimigo %u...\n", em_id);
    fflush(stdout);
    switch (em_id) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 16:
    case 18: // 0x12: Ganado Village (em12)
        EmInitFunc = Em12Init;
        Em10SetFunc = Em12Set;
        printf("[OpenRE4 Core] NativeEmProlog: Em12Init e Em10SetFunc vinculados com sucesso!\n");
        break;
    case 21: // 0x15: Ganado Chainsaw Dr. Salvador (em15)
        EmInitFunc = Em15Init;
        Em10SetFunc = Em15Set;
        printf("[OpenRE4 Core] NativeEmProlog: Em15Init e Em15Set vinculados com sucesso!\n");
        break;
    case 38: // 0x26: Vaca da Vila Central (em26)
        EmInitFunc = Em26Init;
        Em10SetFunc = NULL;
        printf("[OpenRE4 Core] NativeEmProlog: Em26Init vinculado com sucesso!\n");
        break;
    default:
        EmInitFunc = EmSafeFallbackInit;
        Em10SetFunc = NULL;
        printf("[OpenRE4 Core] NativeEmProlog: Inimigo %u sem modulo compilado, usando stub seguro.\n", em_id);
        break;
    }
    fflush(stdout);
}
