#include "xavrx/cartridge.h"

#include <stdlib.h>
#include <string.h>

void xavrx_cartridge_unload(xavrx_cartridge_t *cart) {
    if (cart == NULL) return;
    free(cart->data);
    cart->data = NULL;
    cart->size = 0u;
    cart->source_order = XAVRX_ROM_ORDER_Z64;
}

bool xavrx_cartridge_load_memory(xavrx_cartridge_t *cart,
                                 const uint8_t *image, size_t image_size) {
    xavrx_rom_order_t order;
    uint8_t *normalized;

    if (cart == NULL || image == NULL || image_size < 4u ||
        image_size > XAVRX_ROM_MAX_SIZE || (image_size & 3u) != 0u) {
        return false;
    }

    if (image[0] == 0x80u && image[1] == 0x37u &&
        image[2] == 0x12u && image[3] == 0x40u) {
        order = XAVRX_ROM_ORDER_Z64;
    } else if (image[0] == 0x37u && image[1] == 0x80u &&
               image[2] == 0x40u && image[3] == 0x12u) {
        order = XAVRX_ROM_ORDER_V64;
    } else if (image[0] == 0x40u && image[1] == 0x12u &&
               image[2] == 0x37u && image[3] == 0x80u) {
        order = XAVRX_ROM_ORDER_N64;
    } else {
        return false;
    }

    normalized = (uint8_t *)malloc(image_size);
    if (normalized == NULL) return false;

    if (order == XAVRX_ROM_ORDER_Z64) {
        memcpy(normalized, image, image_size);
    } else if (order == XAVRX_ROM_ORDER_V64) {
        for (size_t i = 0; i < image_size; i += 2u) {
            normalized[i] = image[i + 1u];
            normalized[i + 1u] = image[i];
        }
    } else {
        for (size_t i = 0; i < image_size; i += 4u) {
            normalized[i] = image[i + 3u];
            normalized[i + 1u] = image[i + 2u];
            normalized[i + 2u] = image[i + 1u];
            normalized[i + 3u] = image[i];
        }
    }

    xavrx_cartridge_unload(cart);
    cart->data = normalized;
    cart->size = image_size;
    cart->source_order = order;
    return true;
}
