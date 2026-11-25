#include <3ds.h>
#include <stdio.h>

int main(void) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("XAVRX N64\n");
    printf("Nintendo 64 emulator for 3DS\n\n");
    printf("Phase 1: MIPS core foundation\n");
    printf("Build status: development shell\n\n");
    printf("The CPU and memory core are being\n");
    printf("implemented and tested on host first.\n");
    printf("Commercial ROM boot is not available yet.\n\n");
    printf("Press START to exit.\n");

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
