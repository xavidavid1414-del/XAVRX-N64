#include "xavrx/bus.h"

#include <stdlib.h>
#include <string.h>

static bool range_ok(const xavrx_bus_t *bus, uint32_t address, size_t width) {
    if (bus == NULL || bus->rdram == NULL || width == 0u) return false;
    if ((size_t)address > bus->rdram_size) return false;
    return width <= bus->rdram_size - (size_t)address;
}

bool xavrx_bus_init(xavrx_bus_t *bus, size_t rdram_size) {
    if (bus == NULL || rdram_size == 0u) return false;
    bus->rdram = (uint8_t *)calloc(rdram_size, 1u);
    if (bus->rdram == NULL) {
        bus->rdram_size = 0u;
        return false;
    }
    bus->rdram_size = rdram_size;
    return true;
}

void xavrx_bus_destroy(xavrx_bus_t *bus) {
    if (bus == NULL) return;
    free(bus->rdram);
    bus->rdram = NULL;
    bus->rdram_size = 0u;
}

bool xavrx_bus_read8(const xavrx_bus_t *bus, uint32_t address, uint8_t *value) {
    if (value == NULL || !range_ok(bus, address, 1u)) return false;
    *value = bus->rdram[address];
    return true;
}

bool xavrx_bus_read16(const xavrx_bus_t *bus, uint32_t address, uint16_t *value) {
    if (value == NULL || (address & 1u) != 0u || !range_ok(bus, address, 2u)) return false;
    *value = (uint16_t)(((uint16_t)bus->rdram[address] << 8) |
                        (uint16_t)bus->rdram[address + 1u]);
    return true;
}

bool xavrx_bus_read32(const xavrx_bus_t *bus, uint32_t address, uint32_t *value) {
    if (value == NULL || (address & 3u) != 0u || !range_ok(bus, address, 4u)) return false;
    *value = ((uint32_t)bus->rdram[address] << 24) |
             ((uint32_t)bus->rdram[address + 1u] << 16) |
             ((uint32_t)bus->rdram[address + 2u] << 8) |
             (uint32_t)bus->rdram[address + 3u];
    return true;
}

bool xavrx_bus_read64(const xavrx_bus_t *bus, uint32_t address, uint64_t *value) {
    uint32_t high, low;
    if (value == NULL || (address & 7u) != 0u) return false;
    if (!xavrx_bus_read32(bus, address, &high) ||
        !xavrx_bus_read32(bus, address + 4u, &low)) return false;
    *value = ((uint64_t)high << 32) | low;
    return true;
}

bool xavrx_bus_write8(xavrx_bus_t *bus, uint32_t address, uint8_t value) {
    if (!range_ok(bus, address, 1u)) return false;
    bus->rdram[address] = value;
    return true;
}

bool xavrx_bus_write16(xavrx_bus_t *bus, uint32_t address, uint16_t value) {
    if ((address & 1u) != 0u || !range_ok(bus, address, 2u)) return false;
    bus->rdram[address] = (uint8_t)(value >> 8);
    bus->rdram[address + 1u] = (uint8_t)value;
    return true;
}

bool xavrx_bus_write32(xavrx_bus_t *bus, uint32_t address, uint32_t value) {
    if ((address & 3u) != 0u || !range_ok(bus, address, 4u)) return false;
    bus->rdram[address] = (uint8_t)(value >> 24);
    bus->rdram[address + 1u] = (uint8_t)(value >> 16);
    bus->rdram[address + 2u] = (uint8_t)(value >> 8);
    bus->rdram[address + 3u] = (uint8_t)value;
    return true;
}

bool xavrx_bus_write64(xavrx_bus_t *bus, uint32_t address, uint64_t value) {
    return xavrx_bus_write32(bus, address, (uint32_t)(value >> 32)) &&
           xavrx_bus_write32(bus, address + 4u, (uint32_t)value);
}
