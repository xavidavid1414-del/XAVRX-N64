#include <3ds.h>
#include "xavrx/cartridge.h"
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static PrintConsole top;
static PrintConsole bottom;

#define MAX_ITEMS 96
#define PATH_SIZE 512

typedef enum {
    SCREEN_LIBRARY,
    SCREEN_SETTINGS,
    SCREEN_SD
} Screen;

typedef struct {
    char name[256];
    bool directory;
} BrowserItem;

static Screen screen = SCREEN_LIBRARY;
static char game_folder[PATH_SIZE] = "sdmc:/roms/n64";
static BrowserItem items[MAX_ITEMS];
static int item_count = 0;
static int selected = 0;
static int scroll = 0;
static bool show_fps = true;
static bool folder_loaded = false;
static char status[96] = "Choose Settings > SD to select your games folder.";
static xavrx_cartridge_t cartridge = {0};
static char loaded_rom[256] = "";

static bool is_rom(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return false;
    return strcasecmp(dot, ".z64") == 0 ||
           strcasecmp(dot, ".n64") == 0 ||
           strcasecmp(dot, ".v64") == 0;
}

static void load_folder(void) {
    DIR *dir = opendir(game_folder);
    item_count = 0;
    selected = 0;
    scroll = 0;
    folder_loaded = true;

    if (!dir) {
        snprintf(status, sizeof(status), "Cannot open folder: %.55s", game_folder);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && item_count < MAX_ITEMS) {
        if (entry->d_name[0] == '.') continue;
        if (entry->d_type == DT_DIR || is_rom(entry->d_name)) {
            snprintf(items[item_count].name, sizeof(items[item_count].name),
                     "%s", entry->d_name);
            items[item_count].directory = entry->d_type == DT_DIR;
            item_count++;
        }
    }
    closedir(dir);
    snprintf(status, sizeof(status), "%d N64 item(s) found", item_count);
}

static void parent_folder(void) {
    size_t len = strlen(game_folder);
    while (len > 0 && game_folder[len - 1] == '/') game_folder[--len] = '\0';
    char *slash = strrchr(game_folder, '/');
    if (slash && slash > game_folder + 5) *slash = '\0';
    else snprintf(game_folder, sizeof(game_folder), "sdmc:/");
    load_folder();
}

static void enter_selected(void) {
    if (selected < 0 || selected >= item_count) return;
    if (items[selected].directory) {
        size_t used = strlen(game_folder);
        if (used + strlen(items[selected].name) + 2 >= sizeof(game_folder)) return;
        if (used && game_folder[used - 1] != '/')
            strncat(game_folder, "/", sizeof(game_folder) - strlen(game_folder) - 1);
        strncat(game_folder, items[selected].name,
                sizeof(game_folder) - strlen(game_folder) - 1);
        load_folder();
    } else {
        char path[PATH_SIZE + 256];
        FILE *file;
        long file_size;
        uint8_t *image;
        size_t bytes_read;

        if (snprintf(path, sizeof(path), "%s/%s", game_folder,
                     items[selected].name) >= (int)sizeof(path)) {
            snprintf(status, sizeof(status), "ROM path is too long");
            return;
        }
        file = fopen(path, "rb");
        if (!file) {
            snprintf(status, sizeof(status), "Cannot open selected ROM");
            return;
        }
        if (fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) < 4 ||
            (unsigned long)file_size > XAVRX_ROM_MAX_SIZE ||
            fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            snprintf(status, sizeof(status), "Invalid ROM file size");
            return;
        }
        image = (uint8_t *)malloc((size_t)file_size);
        if (!image) {
            fclose(file);
            snprintf(status, sizeof(status), "Not enough memory for ROM");
            return;
        }
        bytes_read = fread(image, 1u, (size_t)file_size, file);
        fclose(file);
        if (bytes_read != (size_t)file_size ||
            !xavrx_cartridge_load_memory(&cartridge, image, bytes_read)) {
            free(image);
            snprintf(status, sizeof(status), "Unsupported or damaged N64 ROM");
            return;
        }
        free(image);
        snprintf(loaded_rom, sizeof(loaded_rom), "%.255s", items[selected].name);
        snprintf(status, sizeof(status), "ROM validated: %.65s", loaded_rom);
    }
}

static void draw_library(void) {
    consoleSelect(&top);
    consoleClear();
    printf("XAVRX N64  |  GAME LIBRARY\n");
    printf("----------------------------------------\n");
    printf("Folder: %.70s\n\n", game_folder);
    if (!folder_loaded) load_folder();
    if (item_count == 0) {
        printf("No N64 games found in this folder.\n");
        printf("Open SETTINGS on the lower screen,\n");
        printf("then SD to choose your games folder.\n");
    } else {
        int end = scroll + 10;
        if (end > item_count) end = item_count;
        for (int i = scroll; i < end; i++) {
            printf("%s %s %s\n", i == selected ? ">" : " ",
                   items[i].directory ? "[DIR]" : "[N64]",
                   items[i].name);
        }
        printf("\nA: select   UP/DOWN: move\n");
        printf("Directories can be opened from SD settings.\n");
    }
    printf("\n%s\n", status);
    if (cartridge.data != NULL) {
        printf("\nLoaded ROM: %.60s\n", loaded_rom);
        printf("Normalized format: big-endian z64\n");
        printf("ROM bytes: %lu\n", (unsigned long)cartridge.size);
        printf("Game execution is not connected yet.");
    } else {
        printf("\nROM selection reads and validates the image.\n");
        printf("CPU, RSP and RDP execution is still in progress.");
    }

    consoleSelect(&bottom);
    consoleClear();
    printf("\n\n\n");
    printf("          [*] SETTINGS\n");
    printf("\n");
    printf("       Tap here to open settings");
}

static void draw_settings(void) {
    consoleSelect(&top);
    consoleClear();
    printf("XAVRX N64  |  SETTINGS\n");
    printf("----------------------------------------\n\n");
    printf("%s FPS counter\n", show_fps ? "[ON ]" : "[OFF]");
    printf("[   ] Button mapping\n");
    printf("[   ] Controls\n");
    printf("[   ] Performance\n");
    printf("[   ] Audio\n");
    printf("[   ] Display\n");
    printf("[   ] Other settings\n\n");
    printf("Use UP/DOWN and A to change an option.\n");
    printf("B: return to game library");

    consoleSelect(&bottom);
    consoleClear();
    printf("\n\n       SETTINGS MENU\n\n");
    printf("      [ SD / GAME FOLDER ]\n\n");
    printf("      [ BACK TO LIBRARY ]\n");
    printf("\nTouch SD / GAME FOLDER to browse\n");
    printf("the microSD card and choose a folder.");
}

static void draw_sd(void) {
    consoleSelect(&top);
    consoleClear();
    printf("XAVRX N64  |  SD GAME FOLDER\n");
    printf("----------------------------------------\n");
    printf("Current folder:\n%.100s\n\n", game_folder);
    printf("Select a directory and press A to open it.\n");
    printf("Press B to go to the parent folder.\n");
    printf("Press X to use this folder for the library.\n\n");
    int end = scroll + 10;
    if (end > item_count) end = item_count;
    for (int i = scroll; i < end; i++) {
        if (items[i].directory)
            printf("%s [DIR] %s\n", i == selected ? ">" : " ", items[i].name);
        else
            printf("%s [ROM] %s\n", i == selected ? ">" : " ", items[i].name);
    }
    printf("\n%s", status);

    consoleSelect(&bottom);
    consoleClear();
    printf("\n\n       SD BROWSER\n\n");
    printf("      [ USE THIS FOLDER ]\n\n");
    printf("      [ PARENT FOLDER ]\n\n");
    printf("      B: parent  X: choose folder");
}

int main(void) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    consoleInit(GFX_BOTTOM, NULL);

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        touchPosition touch;
        hidTouchRead(&touch);

        if (screen == SCREEN_LIBRARY) {
            if (down & KEY_UP) {
                if (selected > 0) selected--;
                if (selected < scroll) scroll = selected;
            }
            if (down & KEY_DOWN) {
                if (selected + 1 < item_count) selected++;
                if (selected >= scroll + 10) scroll = selected - 9;
            }
            if (down & KEY_A) enter_selected();
            if (down & KEY_TOUCH) {
                if (touch.px >= 180 && touch.py >= 150) screen = SCREEN_SETTINGS;
            }
        } else if (screen == SCREEN_SETTINGS) {
            if (down & KEY_UP || down & KEY_DOWN || down & KEY_A)
                show_fps = !show_fps;
            if (down & KEY_B) screen = SCREEN_LIBRARY;
            if (down & KEY_TOUCH) {
                if (touch.px < 280 && touch.py < 125) {
                    screen = SCREEN_SD;
                    load_folder();
                } else {
                    screen = SCREEN_LIBRARY;
                }
            }
        } else {
            if (down & KEY_UP) {
                if (selected > 0) selected--;
                if (selected < scroll) scroll = selected;
            }
            if (down & KEY_DOWN) {
                if (selected + 1 < item_count) selected++;
                if (selected >= scroll + 10) scroll = selected - 9;
            }
            if (down & KEY_A) enter_selected();
            if (down & KEY_B) parent_folder();
            if (down & KEY_X) {
                load_folder();
                screen = SCREEN_LIBRARY;
            }
            if (down & KEY_TOUCH) {
                if (touch.py < 120) {
                    load_folder();
                    screen = SCREEN_LIBRARY;
                } else {
                    parent_folder();
                }
            }
        }

        if (screen == SCREEN_LIBRARY) draw_library();
        else if (screen == SCREEN_SETTINGS) draw_settings();
        else draw_sd();

        if (show_fps) {
            consoleSelect(&bottom);
            printf("\nFPS counter: enabled (emulation FPS pending)");
        }
        gspWaitForVBlank();
    }

    xavrx_cartridge_unload(&cartridge);
    gfxExit();
    return 0;
}
