#ifndef XAVRX_CARTRIDGE_H
#define XAVRX_CARTRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define XAVRX_ROM_MAX_SIZE (64u * 1024u * 1024u)

typedef enum {
    XAVRX_ROM_ORDER_Z64 = 0,
    XAVRX_ROM_ORDER_V64,
    XAVRX_ROM_ORDER_N64
} xavrx_rom_order_t;

typedef struct {
    uint8_t *data;
    size_t size;
    xavrx_rom_order_t source_order;
} xavrx_cartridge_t;

/* Loads a ROM image from memory and normalizes it to big-endian .z64 order. */
bool xavrx_cartridge_load_memory(xavrx_cartridge_t *cart,
                                 const uint8_t *image, size_t image_size);
void xavrx_cartridge_unload(xavrx_cartridge_t *cart);

#endif
