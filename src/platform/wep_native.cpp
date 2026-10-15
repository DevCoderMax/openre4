#include "types.h"
#include <stdio.h>

extern void Wep02_prolog();

void NativeWepProlog(u32 wep_id) {
    printf("[OpenRE4 Core] NativeWepProlog: vinculando modulo nativo da arma %u...\n", wep_id);
    fflush(stdout);
    switch (wep_id) {
    case 2: // Punisher / Red9 (Handgun)
    default:
        Wep02_prolog();
        break;
    }
}
