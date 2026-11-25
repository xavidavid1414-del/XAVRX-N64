#ifndef XAVRX_BUS_H
#define XAVRX_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define XAVRX_RDRAM_SIZE (8u * 1024u * 1024u)

typedef struct {
    uint8_t *rdram;
    size_t rdram_size;
} xavrx_bus_t;

bool xavrx_bus_init(xavrx_bus_t *bus, size_t rdram_size);
void xavrx_bus_destroy(xavrx_bus_t *bus);
bool xavrx_bus_read8(const xavrx_bus_t *bus, uint32_t address, uint8_t *value);
bool xavrx_bus_read16(const xavrx_bus_t *bus, uint32_t address, uint16_t *value);
bool xavrx_bus_read32(const xavrx_bus_t *bus, uint32_t address, uint32_t *value);
bool xavrx_bus_read64(const xavrx_bus_t *bus, uint32_t address, uint64_t *value);
bool xavrx_bus_write8(xavrx_bus_t *bus, uint32_t address, uint8_t value);
bool xavrx_bus_write16(xavrx_bus_t *bus, uint32_t address, uint16_t value);
bool xavrx_bus_write32(xavrx_bus_t *bus, uint32_t address, uint32_t value);
bool xavrx_bus_write64(xavrx_bus_t *bus, uint32_t address, uint64_t value);

#endif
