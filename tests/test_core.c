#include "xavrx/bus.h"
#include "xavrx/cpu.h"

#include <assert.h>
#include <stdio.h>

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
    /* addiu t0, zero, 7; ori t1, t0, 0x120; addu t2, t0, t1 */
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
    /* beq zero, zero, +2; addiu t0, zero, 1 (delay slot); addiu t0, zero, 2 */
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

int main(void) {
    test_big_endian_bus();
    test_cpu_addiu_and_logic();
    test_cpu_branch_delay_slot();
    puts("XAVRX N64 core tests passed.");
    return 0;
}
