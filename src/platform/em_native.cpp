#include "types.h"
#include <stdio.h>
#include "em.h"
#include "em_mod.h"
#include "em10.h"

extern void Em12Init(cEm* em);
extern void Em12Set(cEm10* em);

void NativeEmProlog(u32 em_id) {
    printf("[OpenRE4 Core] NativeEmProlog: vinculando modulo nativo do inimigo %u...\n", em_id);
    fflush(stdout);
    switch (em_id) {
    case 18: // 0x12: Ganado Village (em12)
    default:
        EmInitFunc = Em12Init;
        Em10SetFunc = Em12Set;
        printf("[OpenRE4 Core] NativeEmProlog: Em12Init e Em10SetFunc vinculados com sucesso!\n");
        fflush(stdout);
        break;
    }
}
