#include "xavrx/bus.h"
#include "xavrx/cpu.h"
#include "xavrx/cartridge.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_big_endian_bus(void) {
    xavrx_bus_t bus = {0};
    uint32_t word = 0;
    uint64_t wide = 0;
    assert(xavrx_bus_init(&bus, 64u));
    assert(xavrx_bus_write32(&bus, 0u, 0x12345678u));
    assert(bus.rdram[0] == 0x12u && bus.rdram[3] == 0x78u);
    assert(xavrx_bus_read32(&bus, 0u, &word) && word == 0x12345678u);
    assert(xavrx_bus_write64(&bus, 8u, 0x0123456789abcdefull));
    assert(xavrx_bus_read64(&bus, 8u, &wide) && wide == 0x0123456789abcdefull);
    assert(!xavrx_bus_read32(&bus, 62u, &word));
    assert(!xavrx_bus_read32(&bus, 1u, &word));
    xavrx_bus_destroy(&bus);
}

static void test_cpu_addiu_and_logic(void) {
    xavrx_bus_t bus = {0};
    xavrx_cpu_t cpu;
    assert(xavrx_bus_init(&bus, 4096u));
    assert(xavrx_bus_write32(&bus, 0u, 0x24080007u));
    assert(xavrx_bus_write32(&bus, 4u, 0x35090120u));
    assert(xavrx_bus_write32(&bus, 8u, 0x01095021u));
    xavrx_cpu_reset(&cpu, 0u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[8] == 7u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[9] == 0x127u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[10] == 0x12eu);
    assert(cpu.gpr[0] == 0u);
    xavrx_bus_destroy(&bus);
}

static void test_cpu_branch_delay_slot(void) {
    xavrx_bus_t bus = {0};
    xavrx_cpu_t cpu;
    assert(xavrx_bus_init(&bus, 4096u));
    assert(xavrx_bus_write32(&bus, 0u, 0x10000002u));
    assert(xavrx_bus_write32(&bus, 4u, 0x24080001u));
    assert(xavrx_bus_write32(&bus, 12u, 0x24080002u));
    xavrx_cpu_reset(&cpu, 0u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.pc == 4u && cpu.next_pc == 12u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[8] == 1u && cpu.pc == 12u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[8] == 2u);
    xavrx_bus_destroy(&bus);
}

static void test_cpu_jump_and_link(void) {
    xavrx_bus_t bus = {0};
    xavrx_cpu_t cpu;
    assert(xavrx_bus_init(&bus, 4096u));
    assert(xavrx_bus_write32(&bus, 0u, 0x0c000004u));
    assert(xavrx_bus_write32(&bus, 4u, 0x24080003u));
    xavrx_cpu_reset(&cpu, 0u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[31] == 8u);
    assert(cpu.pc == 4u && cpu.next_pc == 16u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[8] == 3u && cpu.pc == 16u);
    xavrx_bus_destroy(&bus);
}

static void test_cartridge_byte_orders(void) {
    const uint8_t z64[] = {0x80u, 0x37u, 0x12u, 0x40u, 0x11u, 0x22u, 0x33u, 0x44u};
    const uint8_t v64[] = {0x37u, 0x80u, 0x40u, 0x12u, 0x22u, 0x11u, 0x44u, 0x33u};
    const uint8_t n64[] = {0x40u, 0x12u, 0x37u, 0x80u, 0x44u, 0x33u, 0x22u, 0x11u};
    xavrx_cartridge_t cart = {0};

    assert(xavrx_cartridge_load_memory(&cart, z64, sizeof(z64)));
    assert(cart.source_order == XAVRX_ROM_ORDER_Z64);
    assert(memcmp(cart.data, z64, sizeof(z64)) == 0);
    xavrx_cartridge_unload(&cart);

    assert(xavrx_cartridge_load_memory(&cart, v64, sizeof(v64)));
    assert(cart.source_order == XAVRX_ROM_ORDER_V64);
    assert(memcmp(cart.data, z64, sizeof(z64)) == 0);
    xavrx_cartridge_unload(&cart);

    assert(xavrx_cartridge_load_memory(&cart, n64, sizeof(n64)));
    assert(cart.source_order == XAVRX_ROM_ORDER_N64);
    assert(memcmp(cart.data, z64, sizeof(z64)) == 0);
    xavrx_cartridge_unload(&cart);

    assert(!xavrx_cartridge_load_memory(&cart, (const uint8_t *)"NOPE", 4u));
    assert(!xavrx_cartridge_load_memory(&cart, z64, 7u));
}


static uint32_t encode_r(unsigned rs, unsigned rt, unsigned rd, unsigned shift, unsigned function) {
    return (rs << 21) | (rt << 16) | (rd << 11) | (shift << 6) | function;
}

static void test_cpu_multiply_divide_and_64bit(void) {
    xavrx_bus_t bus = {0};
    xavrx_cpu_t cpu;
    uint64_t stored = 0u;
    assert(xavrx_bus_init(&bus, 4096u));
    assert(xavrx_bus_write32(&bus, 0u, 0x2408ffebu)); /* addiu r8,zero,-21 */
    assert(xavrx_bus_write32(&bus, 4u, 0x24090004u)); /* addiu r9,zero,4 */
    assert(xavrx_bus_write32(&bus, 8u, encode_r(8u, 9u, 0u, 0u, 0x18u))); /* mult */
    assert(xavrx_bus_write32(&bus, 12u, encode_r(8u, 9u, 0u, 0u, 0x1au))); /* div */
    assert(xavrx_bus_write32(&bus, 16u, encode_r(9u, 0u, 10u, 4u, 0x38u))); /* dsll */
    assert(xavrx_bus_write32(&bus, 20u, 0xfd0c0000u)); /* sd r12,0(r8) */
    xavrx_cpu_reset(&cpu, 0u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert((int64_t)cpu.lo == -84 && cpu.hi == 0u);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert((int64_t)cpu.lo == -5 && (int64_t)cpu.hi == -1);
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(cpu.gpr[10] == 64u);
    cpu.gpr[8] = 32u;
    cpu.gpr[12] = 0x1122334455667788ull;
    assert(xavrx_cpu_step(&cpu, &bus) == XAVRX_CPU_OK);
    assert(xavrx_bus_read64(&bus, 32u, &stored));
    assert(stored == 0x1122334455667788ull);
    xavrx_bus_destroy(&bus);
}

int main(void) {
    test_big_endian_bus();
    test_cartridge_byte_orders();
    test_cpu_multiply_divide_and_64bit();
    test_cpu_addiu_and_logic();
    test_cpu_branch_delay_slot();
    test_cpu_jump_and_link();
    puts("XAVRX N64 core tests passed.");
    return 0;
}
